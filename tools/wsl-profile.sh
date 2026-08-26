#!/usr/bin/env bash
# Profiles grRaytracing with Linux `perf`, run from inside WSL.
#
# One-time setup (run yourself in a WSL terminal, needs your sudo password):
#   sudo apt update && sudo apt install -y linux-tools-common linux-tools-generic
#
# Usage (from WSL):
#   tools/wsl-profile.sh [frame_count]
#
# frame_count (default 6) is how many orbit-sequence frames main() renders
# before exiting, via the GR_PROFILE_FRAMES env var main.cpp reads. A handful
# of frames is already thousands of geodesics and plenty to sample - no need
# to run the full 360-frame sequence just to profile.
#
# Notes:
# - WSL2's kernel is virtualized and has no access to hardware performance
#   counters, so this uses the software `task-clock` event instead of the
#   default `cycles` (which fails with "not supported").
# - perf.data is recorded onto WSL's native ext4 filesystem (/tmp), not the
#   /mnt/c-mounted project directory: writing perf's mmap'd trace buffer
#   through the DrvFs 9p bridge fails with "failed to write perf data, error:
#   Bad address". The executable itself still runs from the project's build
#   directory on /mnt/c, so its relative paths (../background.exr, ../seq)
#   keep working. Only the final report + flamegraph get copied back.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/cmake-build-wsl-profile"
SCRATCH_DIR="/tmp/gr-profile"
FRAMES="${1:-6}"
export DEBUGINFOD_URLS=

if ! command -v perf >/dev/null 2>&1; then
    echo "perf not found. Install it first (needs your sudo password):" >&2
    echo "  sudo apt update && sudo apt install -y linux-tools-common linux-tools-generic" >&2
    exit 1
fi

echo "== Configuring ($BUILD_DIR) =="
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo

echo "== Building =="
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "== Recording ($FRAMES frames) =="
mkdir -p "$SCRATCH_DIR"
cd "$BUILD_DIR"
GR_PROFILE_FRAMES="$FRAMES" perf record -e task-clock -F 999 -g --call-graph fp -o "$SCRATCH_DIR/perf.data" -- ./grRaytracing

echo "== Text report =="
perf report -i "$SCRATCH_DIR/perf.data" --stdio --sort=overhead,symbol > "$SCRATCH_DIR/perf-report.txt"

echo "== Flamegraph =="
FLAMEGRAPH_DIR="$PROJECT_DIR/tools/FlameGraph"
if [ ! -d "$FLAMEGRAPH_DIR" ]; then
    mkdir -p "$FLAMEGRAPH_DIR"
    curl -sL https://raw.githubusercontent.com/brendangregg/FlameGraph/master/stackcollapse-perf.pl -o "$FLAMEGRAPH_DIR/stackcollapse-perf.pl"
    curl -sL https://raw.githubusercontent.com/brendangregg/FlameGraph/master/flamegraph.pl -o "$FLAMEGRAPH_DIR/flamegraph.pl"
fi
perf script -i "$SCRATCH_DIR/perf.data" \
    | perl "$FLAMEGRAPH_DIR/stackcollapse-perf.pl" \
    | perl "$FLAMEGRAPH_DIR/flamegraph.pl" > "$SCRATCH_DIR/flamegraph.svg"

echo "== Copying results into $BUILD_DIR =="
cp "$SCRATCH_DIR/perf-report.txt" "$SCRATCH_DIR/flamegraph.svg" "$BUILD_DIR/"

echo
echo "Done:"
echo "  Top functions : $BUILD_DIR/perf-report.txt"
echo "  Flamegraph    : $BUILD_DIR/flamegraph.svg   (open in a browser)"
echo "  Raw data      : $SCRATCH_DIR/perf.data       (perf report -i $SCRATCH_DIR/perf.data for the interactive TUI; kept on native fs, not copied back)"