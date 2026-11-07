#!/usr/bin/env bash
# Compiles the engine into a Python module that Pyodide loads in a web browser,
# and copies it to the website. Run from the top folder of the repository, after
# a normal build (it reuses the pybind11 and nlohmann/json that CMake fetched).
#
# One-time setup, about 1.6 GB, all inside the git-ignored build-wasm/:
#   python3.12 -m venv build-wasm/venv
#   build-wasm/venv/bin/pip install pyodide-build
#   export PYODIDE_XBUILDENV_PATH=$PWD/build-wasm/xbuildenv
#   build-wasm/venv/bin/pyodide xbuildenv install 0.27.8
#   build-wasm/venv/bin/pyodide xbuildenv install-emscripten
set -euo pipefail

ROOT=$PWD
VERSION=0.27.8
XBUILD=$ROOT/build-wasm/xbuildenv/$VERSION
PYINC=$XBUILD/xbuildenv/pyodide-root/cpython/installs/python-3.12.7/include/python3.12
OUT=$ROOT/build-wasm/out
MODULE=_molsim.cpython-312-wasm32-emscripten.so

source "$XBUILD/emsdk/emsdk_env.sh" >/dev/null 2>&1
mkdir -p "$OUT"

# The sources of BuilderLib, EngineLib and _molsim in CMakeLists.txt.
SOURCES=(
  src/builder/geometry.cpp
  src/builder/molecule_builder.cpp
  src/builder/lj_particle_box_builder.cpp
  src/utils/yaml_parser.cpp
  src/utils/trajectory_writer.cpp
  src/engine/force_calculator.cpp
  src/engine/molecular_dynamics.cpp
  src/engine/potentials/lj_potential.cpp
  src/engine/potentials/lj_kernels.cpp
  src/engine/potentials/dft_potential.cpp
  src/bindings/molsim_bindings.cpp
)

em++ -std=c++17 -O3 -fPIC -fexceptions -fvisibility=hidden \
  -Isrc -Ibuild/_deps/json-src/include -Ibuild/_deps/pybind11-src/include -I"$PYINC" \
  "${SOURCES[@]}" \
  -sSIDE_MODULE=1 -sWASM_BIGINT -fexceptions \
  -o "$OUT/$MODULE"

mkdir -p website/_static/engine
cp "$OUT/$MODULE" website/_static/engine/_molsim.wasm
ls -la website/_static/engine/_molsim.wasm
