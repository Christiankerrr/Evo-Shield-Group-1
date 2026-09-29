# Evo Shield: Neuroevolution for Intrusion Detection

Evo Shield is an **anomaly-based intrusion detection system** built on
[HyperNEAT](http://eplex.cs.ucf.edu/hyperNEATpage/). Instead of training a neural network with
gradient descent, a **genetic algorithm evolves** it over many generations. The network learns to
label each network connection as **benign** or **attack**. It was developed with the
[NSL-KDD](https://www.unb.ca/cic/datasets/nsl.html) dataset in mind.

The project has three parts:

| Part | What it is |
|---|---|
| `HyperNEAT/` | The HyperNEAT C++ library plus our experiment, `AIDSExploit` (`HyperNEAT/NE/HyperNEAT/Hypercube_NEAT/src/Experiments/`) |
| `ui/` | **IDS Studio**, a local web app: drop a CSV, choose the label and features, and watch training live |
| `dataset_formater/` | A small standalone tool that one-hot encodes text columns in a CSV |

Design documents (SRS, system design, test plans) are in `Docs/`.

---

## How it runs

The original code targets x86 Windows and Visual Studio. The easiest way to run it anywhere is
inside an **x86_64 Linux container**. Evo Shield's scripts handle that for you:

- **macOS:** Colima runs an x86_64 Linux VM, emulated with QEMU on Apple Silicon.
- **Windows:** Docker Desktop, with commands run from **WSL** (Ubuntu).

The first build compiles everything. It takes about 10–15 minutes on Apple Silicon because of
emulation, and less on Intel Macs or Windows. After that, training runs start in seconds.

---

## Install on macOS

1. **Install Homebrew** if you don't have it:
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```
2. **Install the container tools:**
   ```bash
   brew install colima docker qemu lima-additional-guestagents
   ```
3. **Clone the repo:**
   ```bash
   git clone https://github.com/Christiankerrr/Evo-Shield-Group-1.git
   cd Evo-Shield-Group-1
   ```
4. **Build and run a first training job** (this also starts the Colima VM):
   ```bash
   ./run-hyperneat.sh
   ```

## Install on Windows

Everything runs inside **WSL**, so the Linux commands are the same as on a Mac.

1. **Install WSL with Ubuntu.** Open PowerShell as Administrator, run the command below, then
   restart when asked:
   ```powershell
   wsl --install -d Ubuntu
   ```
2. **Install [Docker Desktop](https://www.docker.com/products/docker-desktop/).** In
   *Settings → Resources → WSL integration*, turn on integration for **Ubuntu**. Keep Docker
   Desktop running while you use Evo Shield.
3. **Open the Ubuntu app** and install the basics:
   ```bash
   sudo apt update && sudo apt install -y git python3
   ```
4. **Clone the repo inside WSL** (in your Linux home folder, not under `/mnt/c`). This keeps
   line endings and file permissions correct:
   ```bash
   cd ~
   git clone https://github.com/Christiankerrr/Evo-Shield-Group-1.git
   cd Evo-Shield-Group-1
   ```
5. **Build and run a first training job:**
   ```bash
   ./run-hyperneat.sh
   ```

> The repo is private, so `git clone` asks you to sign in. Use a GitHub
> [personal access token](https://github.com/settings/tokens) as the password, or install the
> GitHub CLI and run `gh auth login` first.

---

## Use it

### Option A: IDS Studio (web UI)

From the repo folder (Terminal on a Mac, the Ubuntu/WSL window on Windows):

```bash
python3 ui/server.py
```

Open **http://127.0.0.1:8765** in your browser (on Windows, any Windows browser works) and:

1. **Load data.** Drop a CSV, or click **Try the sample dataset**. Header rows are detected
   automatically. Raw NSL-KDD files (`KDDTrain+.txt`, `KDDTest+.txt`) have no header, so their 43
   column names are filled in for you.
2. **Pick the label.** Choose the label column and tick the values that mean *benign* (for
   example `normal`). Everything else counts as an attack. A bar shows the class balance.
3. **Pick features.** Choose which columns to use. The table shows each column's type, range and
   how many input nodes it adds. Text columns are one-hot encoded and numbers are scaled to 0–1
   automatically.
4. **Train.** Set the sample size, population size and number of generations, then press
   **Start training**.
5. **Watch the results live:**
   - the fitness curve
   - accuracy, detection rate and false-positive rate per generation
   - a confusion matrix for the best network
   - the trainer log

   You can download the evolved population (`output.xml`) and the prepared CSV.

Uploads and runs are saved under `HyperNEAT/NE/HyperNEAT/run/uploads/` and `…/run/jobs/`.

### Option B: command line

```bash
./run-hyperneat.sh                              # trains on HyperNEAT/NE/HyperNEAT/run/
./run-hyperneat.sh path/to/folder/in/the/repo   # folder containing subset.csv + params.dat
```

The folder you pass must contain:

- **`subset.csv`** with a header row and a column named `labels`. Columns before it are inputs.
  All values must be numeric (use `dataset_formater/` or the web UI to encode text), and the file
  must have **no trailing newline**. Label `0` is treated as the class to detect (attack).
- **`params.dat`**, the genetic algorithm settings. Start from
  `HyperNEAT/NE/HyperNEAT/out/data/AIDSExploitExperiment.dat` and adjust `PopulationSize` and
  `MaxGenerations`. `ExperimentType 0` is the AIDS experiment (the Explore and Balanced variants
  are unfinished stubs, and `3` is the XOR demo).

The result is written to `output.xml` in the same folder.

### Getting NSL-KDD

Download the dataset from the [UNB NSL-KDD page](https://www.unb.ca/cic/datasets/nsl.html) and
drop `KDDTrain+.txt` into IDS Studio. For a quick first run, untick the wide `service` column
(about 70 inputs) and use a sample of a few hundred rows.

---

## Everyday commands

```bash
colima start          # macOS: start the VM after a reboot (Windows: start Docker Desktop)
docker start hn       # restart the build container if it is stopped
./run-hyperneat.sh    # train again
colima stop           # macOS: free up memory when you're done
```

After you change C++ code, IDS Studio recompiles the trainer before each run. From the command
line you can rebuild the same way:

```bash
docker exec hn bash -lc 'cd /work/HyperNEAT/NE/HyperNEAT/build && make -j4'
```

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `failed to connect to the docker API` (Mac) | Run `colima start`, then `export DOCKER_HOST="unix://$HOME/.colima/default/docker.sock"` for manual `docker` commands. The scripts set this automatically. |
| `Docker isn't reachable` (Windows) | Start Docker Desktop and check that WSL integration is on for Ubuntu. |
| `qemu-img not found` / `guest agent binary could not be found` | Run `brew install qemu lima-additional-guestagents`, then `colima delete -f && colima start --arch x86_64 --cpu 4 --memory 8 --disk 20`. |
| `bash\r: No such file or directory` (Windows) | The repo was cloned on the Windows side. Clone it again inside WSL (step 4). |
| The first build seems stuck | It's compiling under emulation, which is normal on Apple Silicon. Check with `docker exec hn ps -eo etime,comm \| grep cc1plus`. |
| Training uses 100% CPU but never logs a generation | Make sure `subset.csv` is numeric, has a `labels` column, and has no trailing newline. |

---

## Known issue: the model doesn't learn yet

The pipeline runs end to end, but on current settings the best network usually flags **every**
row as an attack (about 50% accuracy, 100% detection, 100% false positives, fitness 75). Two
things cause this:

1. **Saturated output.** The output node sums 200 hidden nodes whose weights are close to ±1, so
   it ends up on the same side for almost every input.
2. **The fitness formula.** Fitness is `(accuracy + detection rate) / 2`. That gives "flag
   everything" a score of 75 on balanced data, and evolution gets stuck there. Using balanced
   accuracy, `(detection rate + true-negative rate) / 2`, would score that strategy 50.

Other ways to improve this: scale down the hidden-to-output weights, bring back HyperNEAT's
weight threshold (commented out in `populateSubstrate`), or use fewer hidden nodes than the fixed
200. IDS Studio's confusion matrix and per-generation metrics are the quickest way to see
whether a change helps.

---

## Changes made to build outside Visual Studio

These are already applied in this repo.

1. Removed `-stdlib=libstdc++` from the CMake files for `zlib`, `tinyxmldll`, `Board` and
   `NE/HyperNEAT`. The flag is invalid under gcc and missing on modern macOS. The originals are
   kept as `*.orig`.
2. Fixed the Boost link names in `Hypercube_NEAT/CMakeLists.txt`. They were Visual Studio names
   such as `boost_thread-vc143-mt-x64-1_86`.
3. Fixed an infinite loop in `getCSVRowCount()`. It was called twice on a stream left in a failed
   state, so `while(!f.eof())` never finished.
4. Stopped `evaluateFitness()` from flipping the stored labels on every call. Before, networks
   were scored against labels that alternated between correct and inverted.
5. Fixed the `NORMALIZE` option (a syntax error and a divide-by-zero) and turned it on. This
   scales CPPN coordinates to [-1, 1].
6. Added one `EVAL tp=… tn=… fp=… fn=… fit=…` log line per evaluation for IDS Studio.

The C++ changes (3–6) are commented in
`HyperNEAT/NE/HyperNEAT/Hypercube_NEAT/src/Experiments/HCUBE_AIDSExploitExperiment.cpp`.

## Native Windows build (Visual Studio, original method)

The original team built natively with Visual Studio 2022 on Windows 11, and the GUI version needs
this path. The bundled Boost and wxWidgets sources are **not** in this repo because of their size.
Download them and unpack them into `HyperNEAT/`:

- Boost 1.86.0: https://archives.boost.io/release/1.86.0/source/boost_1_86_0.zip → `HyperNEAT/boost_1_86_0/`
- wxWidgets 3.2.6: https://github.com/wxWidgets/wxWidgets/releases/tag/v3.2.6 → `HyperNEAT/wxWidgets-3.2.6/`

Then follow the step-by-step guide in [`HyperNEAT/README`](HyperNEAT/README).

---

## Credits

The original project was developed by Regan Bossie, Tyler Donay, Megan Krajewski and Richard
Pepe, for Dr. M. Ilhan Akbas (see `Docs/`). The HyperNEAT implementation is © 2008 Jason Gauci,
under the BSD-style license in `HyperNEAT/LICENSE`.
