# MolSimWorkbench

A molecular simulation workbench for educational purposes that enables experimentation with different potential types.

The engine is written in C++17 and exposed to Python through a pybind11 native module (`_molsim`). Simulations are configured via YAML files or driven fully programmatically from Python.

## Features

- **Lennard-Jones MD**: Full NVE/NVT molecular dynamics with periodic boundary conditions, minimum-image convention, configurable cutoff, and geometric/arithmetic mixing rules
- **LJ Bulk Box**: Automatic cubic-lattice initialization from density — no geometry file needed
- **Velocity Verlet Integrator**: Symplectic integrator with Maxwell-Boltzmann velocity initialization and optional Berendsen thermostat
- **Energy Minimization**: Steepest descent, conjugate gradient, and L-BFGS minimizers for pre-MD relaxation
- **Pluggable Potentials**: `LJPotential`, `DFTPotential`, and `MachineLearningPotential` all share the same `Potential` interface; multiple potentials can be stacked in one `ForceCalculator`
- **Molecule Builder**: Build explicit molecules (e.g. water) from YAML geometry files
- **YAML Configuration**: One file drives geometry, potential parameters, integrator settings, and output
- **Python API**: High-level `System.from_yaml()` and low-level direct object API
- **C++ Unit Tests**: GoogleTest suite with CTest integration
- **Cross-platform CI**: GitHub Actions matrix on `ubuntu-latest` and `macos-latest`

## Quick Start

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

This produces:
- `build/run_simulation` — CLI binary
- `build/_molsim.cpython-*.so` — Python extension module

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

# DFT water example
python examples/dft/run_dft_example.py
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
│   │   │   ├── dft_potential.h/.cpp         # DFT interface
│   │   │   └── machine_learning_potential.h/.cpp
│   │   ├── integrators/
│   │   │   ├── velocity_verlet_integrator.h # VV step()
│   │   │   ├── md_integrator.h              # base class + MB init
│   │   │   └── md_parameters.h
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
│   └── dft/                                 # DFT example
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
| `DFTPotential` | Integration stub | Interface wired; DFT backend pluggable |
| `MachineLearningPotential` | Stub | Interface placeholder for ML force fields |

## Current Limitations / Planned Work

- Pair loop is O(N^2) — neighbor list not yet implemented (config stubs exist)
- Potential is not shifted to zero at cutoff (`shift_potential: false`)
- Long-range tail corrections not applied (`tail_correction: false`)
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

---

## Documentation

Layer-by-layer explanations of the codebase are in [`docs/explainer/`](docs/explainer/README.md):

1. [Architecture overview](docs/explainer/01_architecture.md)
2. [LJ implementation — equation to code](docs/explainer/02_lj_implementation.md)
3. [Velocity Verlet integrator and MD loop](docs/explainer/03_integrator_and_md.md)
4. [Builders and YAML configuration](docs/explainer/04_builders_and_config.md)
5. [Python interface via pybind11](docs/explainer/05_python_interface.md)

---

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass (`ctest` + `test_python_api.py`)
5. Submit a pull request
