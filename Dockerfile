# HyperNEAT intrusion-detection build for Apple Silicon (arm64) Macs.
#
# The original codebase targets x86 + Windows/Visual Studio. This image builds the
# HEADLESS (no GUI/MPI/GPU) command-line trainer inside x86_64 Linux, which runs on
# an M-series Mac via Docker/Colima emulation.
#
# Build:  docker build --platform linux/amd64 -t hyperneat-ids .
# Run:    see run-hyperneat.sh  (or the commands in README.md)
FROM --platform=linux/amd64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y -qq \
        build-essential cmake \
        zlib1g-dev \
        libboost-thread-dev libboost-filesystem-dev libboost-system-dev libboost-date-time-dev \
    && rm -rf /var/lib/apt/lists/*

ENV CC=gcc CXX=g++
WORKDIR /work
COPY . /work

# 1) bundled leaf libraries (configure twice: the CMakeLists force Debug on first run)
RUN set -e; cd /work/HyperNEAT/zlib && rm -rf build out && mkdir build && cd build \
    && cmake .. >/dev/null 2>&1 || true \
    && cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null \
    && make -j"$(nproc)" zlib z

RUN set -e; H=/work/HyperNEAT; cd $H/tinyxmldll && rm -rf build out && mkdir build && cd build \
    && cmake -DZLIB_INCLUDE=$H/zlib -DZLIB_LIB=$H/zlib/out .. >/dev/null 2>&1 || true \
    && cmake -DCMAKE_BUILD_TYPE=Release -DZLIB_INCLUDE=$H/zlib -DZLIB_LIB=$H/zlib/out .. >/dev/null \
    && make -j"$(nproc)" tinyxmlpluslib

RUN set -e; cd /work/HyperNEAT/Board && rm -rf build out && mkdir build && cd build \
    && cmake .. >/dev/null 2>&1 || true \
    && cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null \
    && make -j"$(nproc)" board

# 2) NEAT core + the Hypercube_NEAT trainer (headless), using the OS Boost
RUN set -e; H=/work/HyperNEAT; cd $H/NE/HyperNEAT && rm -rf build out && mkdir build && cd build; \
    COMMON="-DUSE_GUI=OFF -DBUILD_MPI=OFF -DBUILD_GPU=OFF -DBUILD_PYTHON=OFF \
      -DBOOST_ROOT=/usr/include -DBOOST_STAGE=/usr/lib/x86_64-linux-gnu \
      -DTINYXMLDLL_INCLUDE=$H/tinyxmldll/include -DTINYXMLDLL_LIB=$H/tinyxmldll/out \
      -DZLIB_INCLUDE=$H/zlib -DZLIB_LIB=$H/zlib/out \
      -DBOARD_INCLUDE=$H/Board/include -DBOARD_LIB=$H/Board/out"; \
    cmake $COMMON .. >/dev/null 2>&1 || true; \
    cmake -DCMAKE_BUILD_TYPE=Release $COMMON .. >/dev/null; \
    make -j"$(nproc)"

# The trainer expects a ./subset.csv and a params .dat in the working directory.
# Mount your data dir to /data and run from there.
WORKDIR /data
ENV PATH="/work/HyperNEAT/NE/HyperNEAT/out:${PATH}"
CMD ["bash", "-lc", "echo 'HyperNEAT built. Binary: Hypercube_NEAT'; echo 'Example: Hypercube_NEAT -I params.dat -O output.xml'; Hypercube_NEAT"]
