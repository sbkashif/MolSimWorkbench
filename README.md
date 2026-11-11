# MolSimWorkbench

A small but real molecular dynamics engine, and a course built around it.

Molecular simulations are a routine part of materials discovery, but they are
often used as a black box. This project takes one apart: the engine is written
in C++17, exposed to Python through a pybind11 module (`_molsim`), and the
[`website/`](website/) course walks through it piece by piece, from the physics
to the C++, the Python binding, parallel loops and an AI agent that drives a
simulation workflow. The same engine, compiled to WebAssembly, runs in the
reader's browser.

Version 0.7.2 ([changelog](CHANGELOG.md)) · MIT license ([`LICENSE`](LICENSE))

## Features

**Engine**
- **Lennard-Jones MD**: NVE/NVT molecular dynamics with periodic boundary conditions, minimum-image convention, configurable cutoff, geometric or Lorentz–Berthelot (`arithmetic`) mixing, and an opt-in tail correction
- **LJ bulk box**: cubic-lattice initialization from a target density, no geometry file needed
- **Swappable integrator and thermostat**: velocity Verlet (default) or leapfrog; velocity rescale (default), Berendsen, Langevin or none, chosen by name in the YAML. Maxwell–Boltzmann velocity initialization with an optional fixed seed
- **Energy minimization** before MD: steepest descent (conjugate gradient and L-BFGS are named in the config but fall back to it)
- **Parallel LJ kernels** (`lj_kernels.cpp`): serial gather by default, with opt-in `std::thread` and OpenMP backends, each tested against a reference loop
- **Pluggable potentials**: every energy function implements the `Potential` interface, and a `ForceCalculator` sums several of them. `LJPotential` is the one implemented today; `DFTPotential` is a placeholder that raises a not-implemented error when built, and `MachineLearningPotential` is an unbuilt stub that does not yet implement the interface
- **Molecule builder**: explicit molecules (e.g. water) from YAML geometry files
- **YAML configuration**: a simulation file points at separate potential and geometry files
- **Python API**: high-level `System.from_yaml()` and the low-level object API

**Around the engine**
- **Learning website** ([`website/`](website/), Sphinx): seven modules: molecular simulation, C++, pybind11, the engine in code, parallel programming on CPU cores, GPU programming, and agentic AI in a simulation workflow
- **In-browser engine**: `_molsim` compiled to WebAssembly and run through Pyodide, so the simulator on the website is this C++ engine ([`scripts/wasm/`](scripts/wasm/))
- **MLIP training agent** ([`agent/`](agent/README.md)): an LLM chooses the next step of a machine-learned-potential training loop; tools do the work and a harness enforces the limits
- **Parallelization course** with exercises ([`exercises/`](exercises/)), a final project ([`project/`](project/README.md)) and benchmarks ([`benchmarks/`](benchmarks/))
- **Tests and CI**: GoogleTest suite through CTest, Python API test, and a GitHub Actions matrix on `ubuntu-latest` and `macos-latest` that also builds the website with warnings as errors

## Quick Start

### Build

```bash
git clone https://github.com/sbkashif/MolSimWorkbench.git
cd MolSimWorkbench
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)          # macOS: make -j$(sysctl -n hw.ncpu)
cd ..
```

You need a C++17 compiler, CMake 3.15 or newer, and Python 3 (CI uses 3.11). The first
`cmake` run downloads the other dependencies, so it needs network access. The
engine reads its YAML files through Python, so install PyYAML
(`python -m pip install pyyaml`). Python scripts are run from the top folder of
the repository; the project itself is not `pip install`ed.

This produces:
- `build/run_simulation` — CLI binary
- `build/_molsim.cpython-*.so` — Python extension module

The parallel LJ backends need OpenMP. GCC on Linux has it built in. Apple clang
does not: install Homebrew's runtime with `brew install libomp` before configuring,
and CMake finds it on its own. Without it the build still succeeds, and the OpenMP
backends fall back to serial code.

### Run a simulation from Python (YAML-driven)

```python
from molsimworkbench import System

system = System.from_yaml("data/lj_bulk_simulation.yaml",
                          trajectory_output="output/trajectory.xyz")
final_energy = system.run()
print(f"Final energy: {final_energy:.4f}")
```

### Run a simulation from Python (programmatic)

```python
import _molsim

geo = _molsim.Geometry()
geo.addAtom("LJ", [0.0, 0.0, 0.0])
geo.addAtom("LJ", [1.1, 0.0, 0.0])
geo.setBoxDimensions([5.0, 5.0, 5.0])

lj = _molsim.LJPotential()
lj.addAtomParameters("LJ", 1.0, 1.0)   # epsilon, sigma (reduced units)
lj.setCutoff(2.5)

fc = _molsim.ForceCalculator()
fc.addPotential(lj)

params = _molsim.SimulationParameters()
params.timeStep = 0.001
params.temperature = 0.1        # low enough that the pair stays bound
params.totalTime = 1.0
params.usePBC = True
box = _molsim.SimulationBox()
box.a = box.b = box.c = 5.0
params.box = box
params.useMinimization = False  # on by default; skip it to see raw dynamics
params.randomSeed = 42          # same velocities every run

md = _molsim.MolecularDynamics()
md.setGeometry(geo)
md.setForceCalculator(fc)
md.setSimulationParameters(params)
md.setTrajectoryOutput("traj.xyz", _molsim.TrajectoryFormat.XYZ, 10)
md.initialize()
md.runSimulation()
```

The pair vibrates around the LJ minimum while the thermostat holds the kinetic
energy at 0.3:

```
Step 0, Potential Energy: -0.983345, Kinetic Energy: 0.3, Total Energy: -0.683345, Temperature: 0.1
Step 100, Potential Energy: -0.999179, Kinetic Energy: 0.3, Total Energy: -0.699179, Temperature: 0.1
Step 300, Potential Energy: -0.947841, Kinetic Energy: 0.3, Total Energy: -0.647841, Temperature: 0.1
```

Run it from the repository root with `PYTHONPATH=build`, or add `build/` to
`sys.path` first. [`docs/explainer/05_python_interface.md`](docs/explainer/05_python_interface.md)
explains each setting.

### Run the CLI

```bash
build/run_simulation data/lj_bulk_simulation.yaml
```

### Run Examples

```bash
# LJ water simulation
python examples/lj/water_lj_sim.py

# LJ bulk box (125 particles, rho=0.8, NVE)
python examples/lj_bulk_box/run_lj_bulk_sim.py
```

### Run Tests

```bash
# C++ unit tests
cd build && ctest --verbose

# Python API tests
python tests/test_python_api.py
```

### Full CI validation (configure + build + all tests)

```bash
bash scripts/ci/run_all.sh --clean
```

---

## Repository Structure

```
MolSimWorkbench/
├── src/
│   ├── engine/
│   │   ├── potentials/
│   │   │   ├── lj_potential.h/.cpp          # LJ energy + forces; reference loops
│   │   │   ├── lj_kernels.h/.cpp            # pair-loop kernels: serial, threads, OpenMP
│   │   │   ├── dft_potential.h/.cpp         # DFT placeholder (not implemented)
│   │   │   └── machine_learning_potential.h/.cpp
│   │   ├── integrators/
│   │   │   ├── integrator.h/.cpp            # Integrator base + makeIntegrator()
│   │   │   ├── velocity_verlet_integrator.h/.cpp  # default
│   │   │   └── leapfrog_integrator.h/.cpp
│   │   ├── thermostats/
│   │   │   ├── thermostat.h/.cpp            # Thermostat base + makeThermostat()
│   │   │   ├── velocity_rescale_thermostat.h/.cpp  # default; Berendsen with tau
│   │   │   └── langevin_thermostat.h/.cpp
│   │   ├── md_state.h/.cpp                  # view the integrator/thermostat work on
│   │   ├── force_calculator.h/.cpp          # potential aggregator
│   │   └── molecular_dynamics.h/.cpp        # top-level MD driver
│   ├── builder/
│   │   ├── geometry.h/.cpp                  # atom list + box
│   │   ├── lj_particle_box_builder.h/.cpp   # cubic lattice init
│   │   └── molecule_builder.h/.cpp          # molecule from YAML
│   ├── bindings/
│   │   └── molsim_bindings.cpp              # pybind11 module
│   ├── tools/
│   │   └── run_simulation.cpp               # CLI main()
│   └── utils/
│       ├── yaml_parser.h/.cpp
│       └── trajectory_writer.h/.cpp
├── data/
│   ├── lj_bulk_simulation.yaml              # LJ bulk run config
│   ├── simulation_config.yaml               # water MD config
│   ├── potentials/
│   │   ├── lj_bulk.yaml                     # LJ params (reduced units)
│   │   ├── lj_only.yaml
│   │   ├── classical.yaml
│   │   ├── dft.yaml
│   │   └── ml_potential.yaml
│   └── geometry/
│       └── water.yaml
├── examples/
│   ├── lj/                                  # LJ water example
│   ├── lj_bulk_box/                         # LJ bulk box example
│   └── dft/                                 # DFT example (raises not-implemented)
├── tests/
│   ├── unit/                                # C++ GoogleTest suite
│   └── test_python_api.py
├── website/                                 # the course (Sphinx); _static/engine holds the wasm
├── docs/                                    # full notes: explainer, learn-cpp, learn-parallel
├── exercises/                               # parallelization course exercises 01–09
├── project/                                 # parallelization course final project
├── benchmarks/                              # LJ kernel benchmarks
├── agent/                                   # MLIP training agent
├── scripts/                                 # ci/run_all.sh, wasm/build_pyodide.sh, website/build.sh
├── molsimworkbench.py                       # high-level Python API
└── CMakeLists.txt
```

---

## Potentials

| Potential | Status | Notes |
|---|---|---|
| `LJPotential` | **Active** | Full energy + forces, PBC, cutoff, mixing rules |
| `DFTPotential` | Not implemented | Placeholder on the `Potential` interface; constructing it raises an error. No DFT calculation is performed |
| `MachineLearningPotential` | Stub | Placeholder for ML force fields; not built, does not derive from `Potential` |

## Current Limitations / Planned Work

- Pair loop is O(N^2) — neighbor list not yet implemented (config stubs exist)
- Potential is not shifted to zero at cutoff (`shift_potential: false`)
- Long-range tail correction implemented but off by default (`tail_correction: false`)
- Particle mass is fixed at 1.0 (reduced units); per-atom masses not yet supported

---

## Trajectory Visualization

The LJ examples write an XYZ trajectory and a [VMD](https://www.ks.uiuc.edu/Research/vmd/) script that loads it into their `output/` folder. After running `examples/lj/water_lj_sim.py`:

```bash
vmd -e examples/lj/output/load_trajectory.vmd
```

---

## Requirements

- C++17 compiler: GCC or Clang. CI builds on Ubuntu and macOS; Windows is not tested
- CMake 3.15+
- Python 3 (CI uses 3.11; the browser engine uses Pyodide's 3.12) with PyYAML
- Dependencies fetched automatically by CMake: pybind11, nlohmann/json, GoogleTest
- Optional: OpenMP for the parallel LJ backends (macOS: `brew install libomp`)

---

## Documentation

The learning material comes in two forms:

- **[`website/`](website/)** — the course: a shorter, guided version of the
  notes, with figures, quizzes and runs of the real engine. Build it with
  `python -m sphinx -E -b html website website/_build/html` (requirements in
  `website/requirements.txt`) and open `website/_build/html/index.html`.
- **[`docs/`](docs/README.md)** — the full notes: every chapter in detail, the
  parallelization course with its exercises, and a layer-by-layer walk through
  the engine's code. Read these on GitHub when you want every line, or before
  changing the engine.

**Learning C++ and the parallel kernel from scratch?** Start at
[`docs/LEARNING_PATH.md`](docs/LEARNING_PATH.md): C++ basics
([`docs/learn-cpp/`](docs/learn-cpp/README.md)) → this codebase → the
parallelization course ([`docs/learn-parallel/`](docs/learn-parallel/START_HERE.md)).
Implementation notes for the parallel LJ kernel are in
[`docs/implementation/parallel_lj.md`](docs/implementation/parallel_lj.md).

Layer-by-layer explanations of the codebase are in [`docs/explainer/`](docs/explainer/README.md):

1. [Architecture overview](docs/explainer/01_architecture.md)
2. [LJ implementation — equation to code](docs/explainer/02_lj_implementation.md)
3. [Velocity Verlet integrator and MD loop](docs/explainer/03_integrator_and_md.md)
4. [Builders and YAML configuration](docs/explainer/04_builders_and_config.md)
5. [Python interface via pybind11](docs/explainer/05_python_interface.md)

Changes to the engine, its Python interface and its build are listed in [`CHANGELOG.md`](CHANGELOG.md).

---

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass (`ctest` + `test_python_api.py`)
5. Submit a pull request
