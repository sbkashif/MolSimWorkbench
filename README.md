# MolSimWorkbench

A molecular simulation workbench for educational purposes that enables experimentation with different potential types.

The engine is written in C++17 and exposed to Python through a pybind11 native module (`_molsim`). Simulations are configured via YAML files or driven fully programmatically from Python.

## Features

- **Lennard-Jones MD**: Full NVE/NVT molecular dynamics with periodic boundary conditions, minimum-image convention, configurable cutoff, and geometric/arithmetic mixing rules
- **LJ Bulk Box**: Automatic cubic-lattice initialization from density — no geometry file needed
- **Swappable integrator and thermostat**: Velocity Verlet (default) or leapfrog; velocity rescale (default), Berendsen, Langevin or none. Chosen by name in the YAML. Maxwell-Boltzmann velocity initialization
- **Energy Minimization**: Steepest descent, conjugate gradient, and L-BFGS minimizers for pre-MD relaxation
- **Pluggable Potentials**: every energy function implements the `Potential` interface, and a `ForceCalculator` sums several of them. `LJPotential` is the one implemented today; `DFTPotential` is a placeholder that raises a not-implemented error when built, and `MachineLearningPotential` is an unbuilt stub that does not yet implement the interface
- **Molecule Builder**: Build explicit molecules (e.g. water) from YAML geometry files
- **YAML Configuration**: One file drives geometry, potential parameters, integrator settings, and output
- **Python API**: High-level `System.from_yaml()` and low-level direct object API
- **C++ Unit Tests**: GoogleTest suite with CTest integration
- **Cross-platform CI**: GitHub Actions matrix on `ubuntu-latest` and `macos-latest`

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

You need a C++17 compiler, CMake 3.15 or newer, and Python 3.7 or newer. The first
`cmake` run downloads the other dependencies, so it needs network access. Python
scripts are run from the top folder of the repository; nothing needs to be
`pip install`ed.

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
geo.addAtom("LJ", [1.2, 0.0, 0.0])
geo.setBoxDimensions([5.0, 5.0, 5.0])

lj = _molsim.LJPotential()
lj.addAtomParameters("LJ", 1.0, 1.0)   # epsilon, sigma (reduced units)
lj.setCutoff(2.5)

fc = _molsim.ForceCalculator()
fc.addPotential(lj)

params = _molsim.SimulationParameters()
params.timeStep = 0.001
params.temperature = 1.0
params.usePBC = True

md = _molsim.MolecularDynamics()
md.setGeometry(geo)
md.setForceCalculator(fc)
md.setSimulationParameters(params)
md.setTrajectoryOutput("traj.xyz", _molsim.TrajectoryFormat.XYZ, 10)
md.initialize()
md.runSimulation()
```

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
│   │   │   ├── lj_potential.h/.cpp          # LJ energy + forces
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
├── docs/
│   ├── explainer/                           # layer-by-layer code walkthrough
│   └── ai_assistance/                       # implementation logs
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

Output XYZ trajectory files can be loaded in [VMD](https://www.ks.uiuc.edu/Research/vmd/) using the provided VMD script:

```bash
vmd -e examples/output/load_trajectory.vmd
```

---

## Requirements

- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.15+
- Python 3.7+
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
