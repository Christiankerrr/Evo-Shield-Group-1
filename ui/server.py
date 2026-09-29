#!/usr/bin/env python3
"""HyperNEAT IDS Studio - local web UI for the intrusion-detection trainer.

Drop a CSV in the browser, pick the label column and features, and train the
HyperNEAT trainer (running in the Colima/Docker container `hn`) with live charts.

Standard library only. Run from anywhere:
    python3 ui/server.py            # then open http://127.0.0.1:8765
"""
import csv
import io
import json
import os
import random
import re
import subprocess
import threading
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

PORT = int(os.environ.get("IDS_STUDIO_PORT", "8765"))
UI_DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(UI_DIR)
NE_DIR = os.path.join(REPO, "HyperNEAT", "NE", "HyperNEAT")
RUN_DIR = os.path.join(NE_DIR, "run")
UPLOAD_DIR = os.path.join(RUN_DIR, "uploads")
JOBS_DIR = os.path.join(RUN_DIR, "jobs")
BASE_PARAMS = os.path.join(NE_DIR, "out", "data", "AIDSExploitExperiment.dat")
CONTAINER = "hn"
BIN = "/work/HyperNEAT/NE/HyperNEAT/out/Hypercube_NEAT"
BUILD_DIR = "/work/HyperNEAT/NE/HyperNEAT/build"
MAX_UPLOAD = 300 * 1024 * 1024
MAX_ONEHOT = 200  # refuse categorical features wider than this after sampling

DOCKER_ENV = dict(os.environ)
DOCKER_ENV["PATH"] = "/opt/homebrew/bin:/usr/local/bin:" + DOCKER_ENV.get("PATH", "")
# macOS uses Colima's socket; elsewhere (Docker Desktop via WSL, Linux) use Docker's default.
_COLIMA_SOCK = os.path.expanduser("~/.colima/default/docker.sock")
if "DOCKER_HOST" not in DOCKER_ENV and os.path.exists(_COLIMA_SOCK):
    DOCKER_ENV["DOCKER_HOST"] = "unix://" + _COLIMA_SOCK

# NSL-KDD ships without a header: 41 features + label (+ difficulty score).
NSL_KDD_COLUMNS = [
    "duration", "protocol_type", "service", "flag", "src_bytes", "dst_bytes", "land",
    "wrong_fragment", "urgent", "hot", "num_failed_logins", "logged_in", "num_compromised",
    "root_shell", "su_attempted", "num_root", "num_file_creations", "num_shells",
    "num_access_files", "num_outbound_cmds", "is_host_login", "is_guest_login", "count",
    "srv_count", "serror_rate", "srv_serror_rate", "rerror_rate", "srv_rerror_rate",
    "same_srv_rate", "diff_srv_rate", "srv_diff_host_rate", "dst_host_count",
    "dst_host_srv_count", "dst_host_same_srv_rate", "dst_host_diff_srv_rate",
    "dst_host_same_src_port_rate", "dst_host_srv_diff_host_rate", "dst_host_serror_rate",
    "dst_host_srv_serror_rate", "dst_host_rerror_rate", "dst_host_srv_rerror_rate",
    "label", "difficulty",
]
BENIGN_WORDS = {"normal", "benign", "0", "legit", "legitimate"}

datasets = {}   # id -> {"path", "header", "rows", "analysis"}
jobs = {}       # id -> job dict
lock = threading.Lock()


# ----------------------------------------------------------------- helpers
def docker(args, timeout=60):
    return subprocess.run(["docker"] + args, env=DOCKER_ENV, capture_output=True,
                          text=True, timeout=timeout)


def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


def normalize_label(v):
    return v.strip().rstrip(".")


def is_benign_default(v):
    return normalize_label(v).lower() in BENIGN_WORDS


def container_path(host_path):
    return "/work" + host_path[len(REPO):]


def read_csv(text):
    rows = [r for r in csv.reader(io.StringIO(text)) if r and any(c.strip() for c in r)]
    if not rows:
        raise ValueError("The file has no rows.")
    width = max(len(r) for r in rows)
    first = rows[0]
    has_header = all(not is_number(c.strip()) for c in first if c.strip())
    nsl = False
    if has_header:
        header = [c.strip() or "col_%d" % (i + 1) for i, c in enumerate(first)]
        rows = rows[1:]
    elif width in (42, 43):
        header, nsl = NSL_KDD_COLUMNS[:width], True
    else:
        header = ["col_%d" % (i + 1) for i in range(width)]
    rows = [[c.strip() for c in r] for r in rows if len(r) == len(header)]
    if not rows:
        raise ValueError("No data rows match the header width (%d columns)." % len(header))
    return header, rows, has_header, nsl


def analyze(header, rows):
    cols = []
    for j, name in enumerate(header):
        counts, numeric, lo, hi, missing = {}, True, None, None, 0
        for r in rows:
            v = r[j]
            if v == "":
                missing += 1
                continue
            if len(counts) <= 1000 or v in counts:
                counts[v] = counts.get(v, 0) + 1
            if numeric:
                try:
                    x = float(v)
                    lo = x if lo is None else min(lo, x)
                    hi = x if hi is None else max(hi, x)
                except ValueError:
                    numeric = False
        top = sorted(counts.items(), key=lambda kv: -kv[1])[:50]
        cols.append({
            "name": name, "kind": "numeric" if numeric else "categorical",
            "unique": len(counts), "uniqueCapped": len(counts) > 1000,
            "min": lo if numeric else None, "max": hi if numeric else None,
            "missing": missing, "top": top,
        })
    lower = [h.lower() for h in header]
    label_idx = next((i for i, h in enumerate(lower) if h in ("label", "labels", "class", "attack", "target")), None)
    if label_idx is None:
        label_idx = next((i for i, h in enumerate(lower) if "label" in h or "class" in h), len(header) - 1)
    drop = [h for h in header if h.lower() == "difficulty"]
    return {"columns": cols, "suggestedLabel": header[label_idx], "suggestedDrop": drop}


def safe_name(name, used):
    n = re.sub(r"[^A-Za-z0-9_.=:-]+", "_", name).replace("labels", "lbls")[:60] or "f"
    base, k = n, 2
    while n in used:
        n = "%s_%d" % (base, k)
        k += 1
    used.add(n)
    return n


def prepare(ds, cfg):
    """Turn the uploaded rows into the trainer's subset.csv format."""
    header, rows = ds["header"], ds["rows"]
    label = cfg["label"]
    if label not in header:
        raise ValueError("Unknown label column: %s" % label)
    li = header.index(label)
    benign = set(cfg.get("benignValues") or [])
    feats = [h for h in cfg.get("features") or [] if h in header and h != label]
    if not feats:
        raise ValueError("Select at least one feature column.")

    rng = random.Random(cfg.get("seed", 1))
    pos = [r for r in rows if r[li] in benign]
    neg = [r for r in rows if r[li] not in benign]
    if not pos or not neg:
        raise ValueError("Both classes are needed: mark some label values as benign and leave some as attack.")
    n = max(10, min(int(cfg.get("sampleRows", 400)), len(rows)))
    if cfg.get("balance", True):
        nb = min(len(pos), n // 2)
        na = min(len(neg), n - nb)
    else:
        nb = max(1, min(len(pos), round(n * len(pos) / len(rows))))
        na = max(1, min(len(neg), n - nb))
    sample = rng.sample(pos, nb) + rng.sample(neg, na)
    rng.shuffle(sample)

    used, out_header, encoders, dropped = set(), [], [], []
    for h in feats:
        j = header.index(h)
        vals = [r[j] for r in sample]
        if all(v == "" or is_number(v) for v in vals):
            xs = [float(v) if v else 0.0 for v in vals]
            lo, hi = min(xs), max(xs)
            if hi == lo:
                dropped.append(h)
                continue
            encoders.append(("num", j, lo, hi))
            out_header.append(safe_name(h, used))
        else:
            cats = sorted(set(vals))
            if len(cats) > MAX_ONEHOT:
                raise ValueError("'%s' has %d distinct values in the sample - too many to one-hot encode. Untick it." % (h, len(cats)))
            if len(cats) == 1:
                dropped.append(h)
                continue
            encoders.append(("cat", j, cats))
            out_header.extend(safe_name("%s=%s" % (h, c), used) for c in cats)
    if not encoders:
        raise ValueError("Every selected feature is constant in the sample.")

    lines = [",".join(out_header + ["labels"])]
    for r in sample:
        cells = []
        for enc in encoders:
            if enc[0] == "num":
                _, j, lo, hi = enc
                x = float(r[j]) if r[j] else 0.0
                # min-max to [0,1]: the trainer rejects '-' and 'e' as "strings"
                cells.append("%.6f" % ((x - lo) / (hi - lo)))
            else:
                _, j, cats = enc
                cells.extend("1" if r[j] == c else "0" for c in cats)
        # trainer scores label 0 as the positive (detected) class -> attack = 0
        cells.append("1" if r[li] in benign else "0")
        lines.append(",".join(cells))
    summary = {"rows": len(sample), "benign": nb, "attack": na, "inputs": len(out_header),
               "features": len(encoders), "dropped": dropped}
    return "\n".join(lines), summary  # no trailing newline: the row counter is off by one otherwise


def write_params(path, population, generations):
    with open(BASE_PARAMS) as f:
        text = f.read()
    for key, val in (("PopulationSize", population), ("MaxGenerations", generations),
                     ("GenerationDumpModulo", max(1, generations))):
        text = re.sub(r"(?m)^%s\s.*$" % key, "%s %.1f" % (key, val), text)
    with open(path, "w") as f:
        f.write(text)


# ----------------------------------------------------------------- training
EVAL_RE = re.compile(r"EVAL tp=([\d.]+) tn=([\d.]+) fp=([\d.]+) fn=([\d.]+) fit=([-\d.eE+]+|nan)")
GEN_RE = re.compile(r"Generation (\d+): overall_average")


def finish_generation(job, gen_index):
    evals = job["_evals"]
    job["_evals"] = []
    if not evals:
        return
    best = max(evals, key=lambda e: e["fit"])
    job["generations"].append({
        "gen": gen_index, "n": len(evals),
        "avg": sum(e["fit"] for e in evals) / len(evals),
        "best": best,
    })


def log(job, line):
    job["log"].append(line)
    if len(job["log"]) > 400:
        del job["log"][:100]


def run_job(job):
    jdir, cdir = job["dir"], container_path(job["dir"])
    try:
        job["state"], job["phase"] = "building", "Checking the trainer is up to date"
        docker(["start", CONTAINER], timeout=120)
        mk = docker(["exec", CONTAINER, "bash", "-lc", "cd %s && make -j4 2>&1 | tail -20" % BUILD_DIR], timeout=1800)
        for l in mk.stdout.splitlines():
            log(job, l)
        if docker(["exec", CONTAINER, "test", "-x", BIN]).returncode != 0:
            raise RuntimeError("Trainer binary missing. Run ./run-hyperneat.sh once to build it.")

        job["state"], job["phase"] = "training", "Training"
        cmd = ("cd %s && (stdbuf -oL %s -I params.dat -O output.xml 2>&1 & echo $! > trainer.pid; wait $!)"
               % (cdir, BIN))
        proc = subprocess.Popen(["docker", "exec", CONTAINER, "bash", "-lc", cmd], env=DOCKER_ENV,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        job["_proc"] = proc
        with open(os.path.join(jdir, "trainer.log"), "w") as logf:
            for line in proc.stdout:
                logf.write(line)
                line = line.rstrip("\n")
                found = EVAL_RE.findall(line)
                for tp, tn, fp, fn, fit in found:
                    job["_evals"].append({"tp": float(tp), "tn": float(tn), "fp": float(fp),
                                          "fn": float(fn), "fit": float(fit) if fit != "nan" else 0.0})
                m = GEN_RE.search(line)
                if m:
                    finish_generation(job, int(m.group(1)))
                if not found and "Fitness reward" not in line and line.strip():
                    log(job, line)
        proc.wait()
        if job["_evals"]:  # the last generation is evaluated but not announced
            finish_generation(job, len(job["generations"]))
        if job.get("stopRequested"):
            job["state"], job["phase"] = "stopped", "Stopped"
        elif proc.returncode == 0 and os.path.exists(os.path.join(jdir, "output.xml")):
            job["state"], job["phase"] = "done", "Finished"
        else:
            raise RuntimeError("Trainer exited with code %s" % proc.returncode)
    except Exception as e:  # surfaced to the UI
        job["state"], job["phase"], job["error"] = "error", "Error", str(e)
    finally:
        job["finishedAt"] = time.time()
        job.pop("_proc", None)


def public_job(job):
    out = {k: v for k, v in job.items() if not k.startswith("_") and k != "dir"}
    out["current"] = {"gen": len(job["generations"]), "evaluated": len(job["_evals"])}
    out["hasOutput"] = os.path.exists(os.path.join(job["dir"], "output.xml"))
    return out


def health():
    info = {"docker": False, "container": False, "binary": False}
    try:
        r = docker(["inspect", "-f", "{{.State.Running}}", CONTAINER], timeout=10)
        info["docker"] = r.returncode == 0 or "No such object" in r.stderr
        info["container"] = r.stdout.strip() == "true"
        if info["container"]:
            info["binary"] = docker(["exec", CONTAINER, "test", "-x", BIN], timeout=20).returncode == 0
    except Exception as e:
        info["error"] = str(e)
    return info


# ----------------------------------------------------------------- http
class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass

    def send_json(self, obj, code=200):
        body = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_file(self, path, ctype, download=None):
        if not os.path.exists(path):
            return self.send_json({"error": "not found"}, 404)
        with open(path, "rb") as f:
            body = f.read()
        self.send_response(200)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        if download:
            self.send_header("Content-Disposition", 'attachment; filename="%s"' % download)
        self.end_headers()
        self.wfile.write(body)

    def body(self):
        n = int(self.headers.get("Content-Length") or 0)
        if n > MAX_UPLOAD:
            raise ValueError("File too large (max %d MB)." % (MAX_UPLOAD // 1024 // 1024))
        return self.rfile.read(n)

    def do_GET(self):
        path = urlparse(self.path).path
        if path in ("/", "/index.html"):
            return self.send_file(os.path.join(UI_DIR, "index.html"), "text/html; charset=utf-8")
        if path == "/sample_traffic.csv":
            return self.send_file(os.path.join(UI_DIR, "sample_traffic.csv"), "text/csv")
        if path == "/api/health":
            return self.send_json(health())
        if path == "/api/jobs/latest":
            if not jobs:
                return self.send_json({"error": "no jobs yet"}, 404)
            return self.send_json(public_job(max(jobs.values(), key=lambda j: j["startedAt"])))
        m = re.match(r"^/api/jobs/([\w-]+)(?:/(output\.xml|subset\.csv|trainer\.log))?$", path)
        if m:
            job = jobs.get(m.group(1))
            if not job:
                return self.send_json({"error": "no such job"}, 404)
            if m.group(2):
                ctype = "application/xml" if m.group(2).endswith("xml") else "text/plain; charset=utf-8"
                return self.send_file(os.path.join(job["dir"], m.group(2)), ctype, m.group(2))
            return self.send_json(public_job(job))
        self.send_json({"error": "not found"}, 404)

    def do_POST(self):
        path = urlparse(self.path).path
        try:
            if path == "/api/datasets":
                raw = self.body()
                text = raw.decode("utf-8-sig", errors="replace")
                header, rows, has_header, nsl = read_csv(text)
                did = uuid.uuid4().hex[:10]
                os.makedirs(UPLOAD_DIR, exist_ok=True)
                with open(os.path.join(UPLOAD_DIR, did + ".csv"), "w") as f:
                    f.write(text)
                a = analyze(header, rows)
                name = self.headers.get("X-Filename", "upload.csv")
                datasets[did] = {"header": header, "rows": rows}
                return self.send_json({"id": did, "name": name, "rows": len(rows), "header": header,
                                       "hasHeader": has_header, "nslKdd": nsl,
                                       "preview": rows[:12], "benignWords": sorted(BENIGN_WORDS), **a})
            if path == "/api/train":
                cfg = json.loads(self.body() or b"{}")
                with lock:
                    if any(j["state"] in ("building", "training") for j in jobs.values()):
                        return self.send_json({"error": "A training job is already running."}, 409)
                    ds = datasets.get(cfg.get("datasetId"))
                    if not ds:
                        return self.send_json({"error": "Upload a dataset first."}, 400)
                    subset, summary = prepare(ds, cfg)
                    jid = time.strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:4]
                    jdir = os.path.join(JOBS_DIR, jid)
                    os.makedirs(jdir, exist_ok=True)
                    with open(os.path.join(jdir, "subset.csv"), "w") as f:
                        f.write(subset)
                    pop = max(2, min(int(cfg.get("population", 20)), 500))
                    gens = max(1, min(int(cfg.get("generations", 10)), 1000))
                    write_params(os.path.join(jdir, "params.dat"), pop, gens)
                    job = {"id": jid, "dir": jdir, "state": "queued", "phase": "Queued",
                           "config": {"population": pop, "generations": gens, "label": cfg["label"]},
                           "summary": summary, "generations": [], "log": [], "_evals": [],
                           "startedAt": time.time(), "finishedAt": None, "error": None}
                    jobs[jid] = job
                threading.Thread(target=run_job, args=(job,), daemon=True).start()
                return self.send_json(public_job(job))
            m = re.match(r"^/api/jobs/([\w-]+)/stop$", path)
            if m and m.group(1) in jobs:
                job = jobs[m.group(1)]
                job["stopRequested"] = True
                docker(["exec", CONTAINER, "bash", "-lc",
                        "kill -9 $(cat %s/trainer.pid) 2>/dev/null" % container_path(job["dir"])])
                return self.send_json(public_job(job))
            self.send_json({"error": "not found"}, 404)
        except (ValueError, KeyError) as e:
            self.send_json({"error": str(e)}, 400)


if __name__ == "__main__":
    os.makedirs(JOBS_DIR, exist_ok=True)
    print("HyperNEAT IDS Studio -> http://127.0.0.1:%d" % PORT)
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
