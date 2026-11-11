#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

CLEAN=0
if [[ "${1:-}" == "--clean" ]]; then
  CLEAN=1
fi

PYTHON_BIN="${PYTHON_BIN:-python3}"
if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
  PYTHON_BIN="python"
fi

if [[ "$CLEAN" -eq 1 ]]; then
  rm -rf build
fi

mkdir -p build

CMAKE_ARGS=(
  -B build
  -S .
  -DPython_EXECUTABLE="$(command -v "$PYTHON_BIN")"
)

OS_NAME="$(uname -s)"
if [[ "$OS_NAME" == "Darwin" ]]; then
  export CC=/usr/bin/clang
  export CXX=/usr/bin/clang++
  SDK_PATH="$(xcrun --show-sdk-path)"
  CMAKE_ARGS+=( -DCMAKE_OSX_SYSROOT="$SDK_PATH" )
fi

cmake "${CMAKE_ARGS[@]}"
cmake --build build --parallel

(
  cd build
  ctest --output-on-failure
)

"$PYTHON_BIN" examples/lj/water_lj_sim.py

echo "All checks passed on ${OS_NAME}."
