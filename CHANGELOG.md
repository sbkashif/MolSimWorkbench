# Changelog

Changes to the engine, its Python interface and its build. Website and
documentation edits are not listed here. Versions follow [Semantic Versioning](https://semver.org/);
before 1.0.0 a minor version may change the API. Dates are commit dates.
Versions before 0.7.0 were numbered afterwards, from the history.

## 0.7.2 — 2026-11-11

### Changed
- `DFTPotential` now raises a not-implemented error when built
  (`std::runtime_error`, a `RuntimeError` from Python). It performed no DFT
  calculation: the functional name picked one of three tables of per-atom
  energies, and results were not physical. The code stays as a placeholder for
  a real backend. The DFT example is no longer run by `scripts/ci/run_all.sh`.

### Added
- `LJPotential.setTailCorrection` / `getTailCorrection` in the Python module.

### Fixed
- The `tail_correction:` key in a Lennard-Jones potential YAML file is now read
  (it was ignored, so the correction could only be turned on from C++). `data/potentials/lj_only.yaml`, used by the water example, set it to
  `true`; it is now `false`, because the correction assumes a uniform fluid
  beyond the cutoff and that file describes one molecule in an empty box. No
  shipped run changes.

## 0.7.1 — 2026-11-10

### Fixed
- The `"arithmetic"` LJ mixing rule is now Lorentz–Berthelot: σ is averaged
  and ε is `sqrt(ε_i ε_j)`. It used to average ε as well. No shipped config uses
  it; `"geometric"` (the default) is unchanged.
- `lj_particle_box` places every requested particle. The lattice used
  round(N^(1/3)) sites per side, so N such as 128, 256 or 400 got fewer atoms
  (256 gave 216). It now uses the smallest n with n³ ≥ N. Perfect cubes and N
  that already fit are placed exactly as before.

## 0.7.0 — 2026-11-08

### Added
- **Integrator plug.** `Integrator` interface (`src/engine/integrators/`) with
  velocity Verlet (default) and leapfrog. Chosen with `integrator:` under
  `simulation:` in the YAML, or `MolecularDynamics.setIntegrator` from Python.
- **Thermostat plug.** `Thermostat` interface (`src/engine/thermostats/`) with
  velocity rescale (default), Berendsen (needs `tau` > 0), Langevin (`friction`)
  and `none`. Chosen with `thermostat:` under `simulation:`, either as a name or
  as a block such as `thermostat: { type: "langevin", friction: 2.0 }`, or with
  `MolecularDynamics.setThermostat` from Python.
- `MDState`, the view of the system that the plugs work on.
- Example config `data/lj_langevin_simulation.yaml` (leapfrog + Langevin).
- `agent/`: an LLM agent that trains a MACE potential on rMD17 ethanol
  (separate from the engine; uses OpenMM for its MD check).

### Changed
- The engine prints reduced-unit values without K/Å/ps labels.
- Runs that set neither key behave as before: their output is byte-identical.

### Removed
- `src/engine/integrators/md_integrator.h` and `md_parameters.h` (unused).
- `MolecularDynamics::integrateVelocityVerlet`, `applyTemperatureControl` and
  `applyPeriodicBoundaryConditions`; their code now lives in the plugs.

## 0.6.0 — 2026-11-07

### Added
- Equilibration and NVE production phases: `equilibration_steps`,
  `production_steps` and `energy_drift_tolerance` under `simulation:`. The
  temperature is held only during equilibration, and the production energy
  drift is reported. Files without these keys run as before. Example:
  `data/lj_nve_simulation.yaml`.
- A Pyodide (WebAssembly) build of the engine, so it can run in a web browser.

## 0.5.0 — 2026-10-21 – 2026-10-22

### Fixed
- Linux build: position-independent code for `_molsim`, `stdc++fs` on older
  GCC, a missing `<cmath>`.
- Python runs use periodic boundaries; `random_seed` fixes the initial
  velocities.

### Changed
- CMake builds Release by default.
- nlohmann/json bumped to 3.11.3, for CMake 4.
- LJ forces and energy are computed in one pass over the pairs
  (`calculateForcesAndEnergy`), instead of two. Trajectories are unchanged; one
  step at N=4096 goes from 37.5 ms to 18.4 ms.

## 0.4.0 — 2026-09-02

### Added
- An opt-in parallel LJ backend using OpenMP.

## 0.3.0 — 2026-07-01

### Added
- `LJParticleBoxBuilder`: `lj_particle_box` mode builds an LJ box from a
  target density, with no geometry file.
- Minimum-image convention in the LJ potential, for periodic boundaries.
- The pybind11 module `_molsim` (`src/bindings/molsim_bindings.cpp`), which
  `molsimworkbench.py` now calls.
- GitHub Actions CI on Ubuntu and macOS.

### Fixed
- The LJ bulk Python path, and a trajectory file extension that was added twice.
- The DFT example finds its YAML file.

## 0.2.0 — 2026-01-23

### Added
- A basic DFT example and placeholder DFT modules.

### Changed
- The Python module `molsimworkbench.py` replaces `main.cpp` as the way to run
  a simulation.

## 0.1.0 — 2025-04-24

- First working version: a single water molecule with an LJ potential, in C++.
  Also includes steepest-descent energy minimization (the conjugate-gradient
  and L-BFGS options fall back to it) and stub DFT and ML potentials.
