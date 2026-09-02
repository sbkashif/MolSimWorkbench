# 04 — Builders and Configuration

## Two ways to start a simulation

Before any MD loop can run, you need a `Geometry` — a populated list of atom types and positions. There are two paths in this repo:

```
Path A: LJ bulk box
  YAML (lj_bulk_simulation.yaml)
      → YAMLParser detects system_mode: "lj_particle_box"
      → LJParticleBoxBuilder::createCubicLattice()
      → Geometry (N identical "LJ" particles, cubic lattice)

Path B: Molecule (e.g. water)
  YAML (simulation_config.yaml + geometry/water.yaml)
      → YAMLParser reads geometry file
      → MoleculeBuilder constructs atoms from YAML spec
      → Geometry (O + H atoms, explicit coordinates)
```

---

## Path A — LJ Particle Box Builder

`LJParticleBoxBuilder` in `src/builder/lj_particle_box_builder.cpp` takes a `LJBoxConfig` and returns a `Geometry`.

### Box size from density

$$L = \left(\frac{N}{\rho}\right)^{1/3}$$

```cpp
double volume = numParticles / density;
double L      = std::cbrt(volume);
```

With the default config (`N=125`, `rho=0.8`), this gives $L \approx 5.37$ in reduced units.

### Cubic lattice placement

Particles per dimension: $n = \lceil N^{1/3} \rceil$. Lattice spacing: $a = L / n$.

Each particle is placed at:

$$\mathbf{r}_{ijk} = \left((i + 0.5)\,a,\; (j + 0.5)\,a,\; (k + 0.5)\,a\right) - L/2 + \delta$$

where $\delta$ is a tiny random displacement ($\pm 5\%$ of $a$) seeded by `randomSeed`. Without this, a perfect lattice is an unstable equilibrium and the simulation tends to produce unphysical symmetric motion.

```cpp
double x = (i + 0.5) * spacing - boxLength / 2.0 + smallDisp(rng);
geometry.addAtom("LJ", {x, y, z});
```

The box dimensions are stored on the `Geometry` object, which `LJPotential` reads when applying the minimum image convention.

---

## Path B — Molecule Builder

`MoleculeBuilder` reads a YAML file like `data/geometry/water.yaml` and calls `geometry.addAtom(type, coords)` for each atom. This is used for water simulations, where the geometry specifies explicit O and H positions.

---

## YAML config layout

There are three levels of YAML config:

### 1. Simulation config — `data/lj_bulk_simulation.yaml`

```yaml
simulation:
  type: "dynamics"
  system_mode: "lj_particle_box"
  timestep: 0.001
  temperature: 1.0
  equilibration_steps: 1000
  production_steps: 5000
  energy_drift_tolerance: 0.01
  random_seed: 42

  lj_box:
    num_particles: 125
    density: 0.8
    initialization: "cubic_lattice"

  output:
    frequency: 50
    trajectory: true
    energy: true

potential_config_files:
  - "potentials/lj_bulk.yaml"
```

The `system_mode` key is what tells the parser which builder to invoke. The `potential_config_files` list lets you compose multiple potentials (e.g. LJ + electrostatics) though only LJ is active now.

> **Which keys actually do something:** `system_mode`, `timestep`, `temperature`, `lj_box.*`, and `potential_config_files` are honored. The run length is `totalTime / timestep`, **not** `equilibration_steps + production_steps` — those two, plus `output.frequency` and `output.energy`, are parsed into structs but never read by the loop (see doc 03). So the trajectory is written every step and no energy file is produced; energy only goes to stdout.

### 2. Potential config — `data/potentials/lj_bulk.yaml`

```yaml
potential_type: "classical"
components:
  - type: "lennard_jones"
    cutoff: 2.5
    atom_parameters:
      "LJ": { epsilon: 1.0, sigma: 1.0 }
    mixing_rule: "geometric"
    tail_correction: false
    shift_potential: false
```

This is parsed into `LJPotential::addAtomParameters("LJ", 1.0, 1.0)` and `LJPotential::setCutoff(2.5)`.

### 3. Geometry config — `data/geometry/water.yaml` (Path B only)

Used only for explicit molecule simulations (not for the LJ-bulk path which generates geometry programmatically).

---

## YAMLParser

`src/utils/yaml_parser.h/.cpp` wraps the `nlohmann/json` library (fetched via CMake FetchContent as `json-src` in `build/_deps/`). It reads YAML/JSON, validates required keys, and populates the C++ parameter structs.

---

## Minimizers (pre-MD relaxation)

`MolecularDynamics` can optionally run an energy minimisation before the MD loop. Three methods are available:

| `MinimizationMethod` | Description |
|---|---|
| `STEEPEST_DESCENT` | Move each atom along $-\nabla V$ with adaptive step |
| `CONJUGATE_GRADIENT` | CG direction update, faster convergence |
| `LBFGS` | Quasi-Newton, best for larger systems |

Controlled by:

```yaml
minimization:
  algorithm: "steepest_descent"
  max_iterations: 100
  energy_tolerance: 1e-6
  force_tolerance: 1e-4
```

For LJ-bulk NVE, minimisation is typically skipped (`useMinimization: false`) because the point is to start from a lattice and watch it melt into a liquid.

---

## Key files

| File | Role |
|------|------|
| [src/builder/lj_particle_box_builder.h](../../src/builder/lj_particle_box_builder.h) | `LJBoxConfig` struct + builder API |
| [src/builder/lj_particle_box_builder.cpp](../../src/builder/lj_particle_box_builder.cpp) | Cubic lattice implementation |
| [src/builder/geometry.h](../../src/builder/geometry.h) | `Geometry` data class |
| [src/utils/yaml_parser.h](../../src/utils/yaml_parser.h) | Config file reader |
| [data/lj_bulk_simulation.yaml](../../data/lj_bulk_simulation.yaml) | Full example config |
| [data/potentials/lj_bulk.yaml](../../data/potentials/lj_bulk.yaml) | LJ potential parameters |

---

Next: [05_python_interface.md](05_python_interface.md) — running all of this from Python.
