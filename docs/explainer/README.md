# MolSimWorkbench — Explainer Series

> You know the LJ equation. This series walks you through how this repo actually implements it — from raw math to a running MD simulation you can drive from Python.

## Where this fits in the learning path

This series is the middle, narrow part of the path:

1. **[docs/learn-cpp/](../learn-cpp/)** — C++ basics, practised on a small program (`mini_lj`), ending with
   [10_mini_lj_to_the_repo.md](../learn-cpp/10_mini_lj_to_the_repo.md), which builds and runs this repo.
2. **This explainer, 01–04** — how *this* program is put together: layers, the LJ code, the MD loop, and setup.
3. **[docs/learn-parallel/](../learn-parallel/START_HERE.md)** — making the LJ loop fast on many cores.

Two documents sit outside that main line:

- **[00_cpp_primer.md](00_cpp_primer.md) is a review** if you did learn-cpp 01–07. Skip it, or skim it as a recap.
  If you came here without learn-cpp, read it first.
- **[05_python_interface.md](05_python_interface.md) is a "widen later" read (deep dive later).** You do not need it
  before learn-parallel. Come back to it when you want to drive the engine from Python.

## Documents in this folder

| File | What it covers |
|------|---------------|
| [00_cpp_primer.md](00_cpp_primer.md) | C++ syntax, taught by walking one 48-line file (`lj_potential.h`) top to bottom — every keyword explained where it first appears |
| [01_architecture.md](01_architecture.md) | The big picture — what the layers are and how they connect |
| [02_lj_implementation.md](02_lj_implementation.md) | How the LJ equation lives in the code — energy, forces, cutoff, PBC |
| [03_integrator_and_md.md](03_integrator_and_md.md) | Velocity Verlet, Maxwell-Boltzmann init, the MD loop |
| [04_builders_and_config.md](04_builders_and_config.md) | How a simulation is set up — geometry builders and YAML config |
| [05_python_interface.md](05_python_interface.md) | Running everything from Python via pybind11 |

**New to C++?** Start with [00_cpp_primer.md](00_cpp_primer.md) (or better, [docs/learn-cpp/](../learn-cpp/)), then [01_architecture.md](01_architecture.md).
**Comfortable with C++, or finished learn-cpp?** Start with [01_architecture.md](01_architecture.md). After 04, go on to [learn-parallel/START_HERE.md](../learn-parallel/START_HERE.md).

### Suggested 90-minute reading path

| Time | Read | With this file open beside it |
|------|------|-------------------------------|
| 0:00–0:25 | `00_cpp_primer.md` | `src/engine/potentials/lj_potential.h` |
| 0:25–0:35 | `01_architecture.md` | — |
| 0:35–1:00 | `02_lj_implementation.md` | `src/engine/potentials/lj_potential.cpp` |
| 1:00–1:20 | `03_integrator_and_md.md` | `src/engine/molecular_dynamics.cpp` |
| 1:20–1:35 | `04_builders_and_config.md` | `data/lj_bulk_simulation.yaml` |
| 1:35–1:50 | `05_python_interface.md` (deep dive later; optional before learn-parallel) | `src/bindings/molsim_bindings.cpp` |
