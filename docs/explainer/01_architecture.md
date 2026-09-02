# 01 — Architecture: The Big Picture

## What this repo is

MolSimWorkbench is a C++ molecular simulation engine with a Python interface bolted on through pybind11. You write a YAML config file (or a few lines of Python), and it runs Lennard-Jones (or DFT, or ML-potential) molecular dynamics for you.

The code is layered. Each layer has one job and talks to the layer below it through clean interfaces.

```
┌─────────────────────────────────────────────────────┐
│                 Python / YAML config                │   ← you are here
├─────────────────────────────────────────────────────┤
│            Python bindings  (_molsim)               │   pybind11 wrapper
├─────────────────────────────────────────────────────┤
│         MolecularDynamics  (top-level driver)       │
│    setup → minimize → integrate → write trajectory  │
├──────────────────────┬──────────────────────────────┤
│  VV integration      │   Force Calculator           │
│  (method on MD, not  │   (sums up all potentials)   │
│   a separate class)  │                              │
├──────────────────────┴──────────────────────────────┤
│                  Potentials                         │
│      LJPotential  |  DFTPotential  |  MLPotential   │
├─────────────────────────────────────────────────────┤
│                   Geometry                          │
│       atoms + positions + box dimensions            │
├─────────────────────────────────────────────────────┤
│                   Builders                          │
│   LJParticleBoxBuilder  |  MoleculeBuilder (YAML)   │
└─────────────────────────────────────────────────────┘
         utils: YAMLParser, TrajectoryWriter
```

---

## The layers, one sentence each

**Geometry** (`src/builder/geometry.h`)  
The dumbest possible data class: a list of atom types, a list of 3D coordinates, and an optional periodic box. Everything else reads and writes through it.

**Builders** (`src/builder/`)  
Two ways to produce a `Geometry` object from scratch:
- `LJParticleBoxBuilder` — place N identical particles on a cubic lattice at a target density.
- `MoleculeBuilder` — read a molecule geometry from a YAML file (e.g. `water.yaml`).

**Potentials** (`src/engine/potentials/`)  
Three concrete potential classes, all inheriting from the abstract `Potential` base:
- `LJPotential` — the workhorse for this repo right now.
- `DFTPotential` — stub/integration point for DFT energy.
- `MachineLearningPotential` — stub for ML force fields.

Each potential can compute an energy scalar **and** the force vector on every atom.

**Force Calculator** (`src/engine/force_calculator.h`)  
A simple aggregator. You `addPotential(...)` as many potentials as you like; it loops over them and sums energy + forces. This is what makes the potential layer pluggable.

**Integrator** (inside `MolecularDynamics`)  
Velocity-Verlet integration is **not** a separate class — it lives as the private method `MolecularDynamics::integrateVelocityVerlet()` (`molecular_dynamics.cpp:440`), alongside Maxwell-Boltzmann velocity init (`initializeVelocities`, line 122), the thermostat (`applyTemperatureControl`, line 524), and PBC (`applyPeriodicBoundaryConditions`, line 480). The header files under `src/engine/integrators/` (`md_integrator.h`, `velocity_verlet_integrator.h`, `md_parameters.h`) are **unused scaffolding** — nothing in `src/` or `tests/` includes them. Treat them as aspirational, not the live code path.

**MolecularDynamics** (`src/engine/molecular_dynamics.h/cpp`)  
The top-level driver class. You hand it a `Geometry`, a `ForceCalculator`, and `SimulationParameters`, then call `runSimulation()`. It handles the equilibration/production split, trajectory output, and energy logging.

**Utils**  
- `YAMLParser` — reads all `.yaml` config files.
- `TrajectoryWriter` — writes atom positions to XYZ, DCD, or XTC files.

**Python Bindings** (`src/bindings/molsim_bindings.cpp`)  
pybind11 module named `_molsim`. Exposes every C++ class above so you can drive the simulation from Python without touching C++.

**CLI tool** (`src/tools/run_simulation.cpp`)  
A thin `main()` that reads a YAML config and calls through the same stack. This is what `build/run_simulation` is.

---

## Data flow for a typical LJ bulk run

```
1. Read lj_bulk_simulation.yaml  (YAMLParser)
2. Build Geometry                (LJParticleBoxBuilder: cubic lattice)
3. Build LJPotential             (parameters from lj_bulk.yaml: epsilon=1, sigma=1, cutoff=2.5)
4. Wrap it in ForceCalculator
5. Hand both to MolecularDynamics
6. MolecularDynamics.initialize():
       - init Maxwell-Boltzmann velocities
       - compute initial forces
7. Single MD loop (there is no equilibration/production split in code):
       totalSteps = totalTime / timeStep      # e.g. 1.0 / 0.001 = 1000
       for step in range(totalSteps):
           integrateVelocityVerlet()          # method on MolecularDynamics
           applyTemperatureControl()
           write XYZ frame (writeFrequency, default every step)
8. Done. Output: trajectory.xyz (energy is printed to stdout, not written to a file)
```

---

## File map (key files only)

```
src/
  builder/
    geometry.h/.cpp              ← Geometry data class
    lj_particle_box_builder.h/.cpp  ← cubic lattice init
    molecule_builder.h/.cpp      ← molecule from YAML
  engine/
    potentials/
      lj_potential.h/.cpp        ← LJ energy + forces   ← most interesting
      dft_potential.h/.cpp
      machine_learning_potential.h/.cpp
    integrators/                 ← UNUSED scaffolding (not compiled into the code path)
      md_integrator.h
      velocity_verlet_integrator.h
      md_parameters.h
    force_calculator.h/.cpp      ← potential aggregator
    molecular_dynamics.h/.cpp    ← top-level driver + the actual VV loop
  bindings/
    molsim_bindings.cpp          ← pybind11 exposure
  utils/
    yaml_parser.h/.cpp
    trajectory_writer.h/.cpp
  tools/
    run_simulation.cpp           ← CLI main()
data/
  lj_bulk_simulation.yaml        ← example config
  potentials/lj_bulk.yaml        ← LJ params
  geometry/water.yaml            ← water geometry
```

---

Next: [02_lj_implementation.md](02_lj_implementation.md) — how the LJ equation actually lives in `lj_potential.cpp`.
