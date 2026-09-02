# MolSimWorkbench — Explainer Series

> You know the LJ equation. This series walks you through how this repo actually implements it — from raw math to a running MD simulation you can drive from Python.

## Documents in this folder

| File | What it covers |
|------|---------------|
| [00_cpp_primer.md](00_cpp_primer.md) | C++ syntax, taught by walking one 48-line file (`lj_potential.h`) top to bottom — every keyword explained where it first appears |
| [01_architecture.md](01_architecture.md) | The big picture — what the layers are and how they connect |
| [02_lj_implementation.md](02_lj_implementation.md) | How the LJ equation lives in the code — energy, forces, cutoff, PBC |
| [03_integrator_and_md.md](03_integrator_and_md.md) | Velocity Verlet, Maxwell-Boltzmann init, the MD loop |
| [04_builders_and_config.md](04_builders_and_config.md) | How a simulation is set up — geometry builders and YAML config |
| [05_python_interface.md](05_python_interface.md) | Running everything from Python via pybind11 |

**New to C++?** Start with [00_cpp_primer.md](00_cpp_primer.md), then [01_architecture.md](01_architecture.md).
**Comfortable with C++?** Start with [01_architecture.md](01_architecture.md).

### Suggested 90-minute reading path

| Time | Read | With this file open beside it |
|------|------|-------------------------------|
| 0:00–0:25 | `00_cpp_primer.md` | `src/engine/potentials/lj_potential.h` |
| 0:25–0:35 | `01_architecture.md` | — |
| 0:35–1:00 | `02_lj_implementation.md` | `src/engine/potentials/lj_potential.cpp` |
| 1:00–1:20 | `03_integrator_and_md.md` | `src/engine/molecular_dynamics.cpp` |
| 1:20–1:35 | `04_builders_and_config.md` | `data/lj_bulk_simulation.yaml` |
| 1:35–1:50 | `05_python_interface.md` | `src/bindings/molsim_bindings.cpp` |
