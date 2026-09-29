#!/usr/bin/env bash
# Build & run the HyperNEAT intrusion-detection trainer inside an x86_64 Linux
# container: Colima + QEMU on macOS, Docker Desktop on Windows (run this from WSL),
# or plain Docker on Linux. Self-healing: starts Colima on macOS, creates + builds
# the container if needed, then trains.
#
# Usage:
#   ./run-hyperneat.sh                    # trains on HyperNEAT/NE/HyperNEAT/run/{subset.csv,params.dat}
#   ./run-hyperneat.sh <datadir>          # datadir (inside this repo) with subset.csv + params.dat
#
# Output (output.xml) is written back into that data directory (on your Mac).
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATADIR="${1:-$REPO/HyperNEAT/NE/HyperNEAT/run}"
BIN=/work/HyperNEAT/NE/HyperNEAT/out/Hypercube_NEAT

# 1) Make sure a Docker daemon is reachable.
#    macOS: an x86_64 Colima VM. Windows (inside WSL) / Linux: Docker Desktop or dockerd.
if [ "$(uname -s)" = "Darwin" ]; then
  export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
  if ! command -v colima >/dev/null 2>&1; then
    echo "!! colima not found. Install it with: brew install colima docker qemu lima-additional-guestagents"; exit 1
  fi
  if ! colima status >/dev/null 2>&1; then
    echo ">> starting Colima x86_64 VM (first time downloads a VM image)..."
    colima start --arch x86_64 --cpu 4 --memory 8 --disk 20
  fi
  export DOCKER_HOST="unix://${HOME}/.colima/default/docker.sock"
elif ! docker info >/dev/null 2>&1; then
  echo "!! Docker isn't reachable. Start Docker Desktop (with WSL integration enabled for this distro) and try again."; exit 1
fi

# 2) Ensure the build container exists (ubuntu + repo mounted at /work)
if ! docker inspect hn >/dev/null 2>&1; then
  echo ">> creating build container 'hn'..."
  docker run -d --platform linux/amd64 --name hn -v "$REPO":/work -w /work ubuntu:22.04 sleep infinity >/dev/null
elif [ "$(docker inspect -f '{{.State.Running}}' hn)" != "true" ]; then
  docker start hn >/dev/null
fi

# 3) Ensure toolchain + libraries are installed in the container (idempotent)
if ! docker exec hn bash -lc 'command -v g++ >/dev/null && dpkg -s libboost-thread-dev >/dev/null 2>&1'; then
  echo ">> installing build toolchain (gcc, cmake, boost, zlib)..."
  docker exec hn bash -lc 'export DEBIAN_FRONTEND=noninteractive; apt-get update -qq && \
    apt-get install -y -qq build-essential cmake zlib1g-dev \
      libboost-thread-dev libboost-filesystem-dev libboost-system-dev libboost-date-time-dev'
fi

# 4) Build the trainer if the binary is missing
if ! docker exec hn test -x "$BIN"; then
  echo ">> building HyperNEAT (headless). This is slow the first time under emulation..."
  docker exec hn bash -lc '
    set -e; export CC=gcc CXX=g++; H=/work/HyperNEAT
    # bundled leaf libs (configure twice: their CMakeLists force Debug on first run)
    cd $H/zlib      && rm -rf build && mkdir build && cd build && { cmake .. >/dev/null 2>&1 || true; } && cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null && make -j"$(nproc)" zlib z
    cd $H/tinyxmldll && rm -rf build && mkdir build && cd build && { cmake -DZLIB_INCLUDE=$H/zlib -DZLIB_LIB=$H/zlib/out .. >/dev/null 2>&1 || true; } && cmake -DCMAKE_BUILD_TYPE=Release -DZLIB_INCLUDE=$H/zlib -DZLIB_LIB=$H/zlib/out .. >/dev/null && make -j"$(nproc)" tinyxmlpluslib
    cd $H/Board     && rm -rf build && mkdir build && cd build && { cmake .. >/dev/null 2>&1 || true; } && cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null && make -j"$(nproc)" board
    # NEAT core + trainer (headless). NOTE: only remove build/, NOT out/ (out/data holds the .dat params)
    cd $H/NE/HyperNEAT && rm -rf build && mkdir build && cd build
    COMMON="-DUSE_GUI=OFF -DBUILD_MPI=OFF -DBUILD_GPU=OFF -DBUILD_PYTHON=OFF \
      -DBOOST_ROOT=/usr/include -DBOOST_STAGE=/usr/lib/x86_64-linux-gnu \
      -DTINYXMLDLL_INCLUDE=$H/tinyxmldll/include -DTINYXMLDLL_LIB=$H/tinyxmldll/out \
      -DZLIB_INCLUDE=$H/zlib -DZLIB_LIB=$H/zlib/out \
      -DBOARD_INCLUDE=$H/Board/include -DBOARD_LIB=$H/Board/out"
    { cmake $COMMON .. >/dev/null 2>&1 || true; }
    cmake -DCMAKE_BUILD_TYPE=Release $COMMON .. >/dev/null
    make -j"$(nproc)"
  '
fi

# 5) Locate the data dir inside the container (under the mounted /work) and train
case "$DATADIR" in
  "$REPO"*) CDIR="/work${DATADIR#$REPO}" ;;
  *) echo "!! data dir must be inside the repo ($REPO)"; exit 1 ;;
esac
echo ">> training on: $DATADIR"
docker exec hn bash -lc "cd '$CDIR' && test -f subset.csv && test -f params.dat && stdbuf -oL '$BIN' -I params.dat -O output.xml 2>&1 | grep -vi 'fitness reward'"
echo ">> done. Output: $DATADIR/output.xml"
