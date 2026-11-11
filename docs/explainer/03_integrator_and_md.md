# 03 — Integrator and MD Loop

> **Where we are.** [02](02_lj_implementation.md) left you able to compute energy and forces for one fixed set of positions. This doc is about the loop that calls that code over and over to move the atoms.

## From forces to motion

Once you have forces from `LJPotential`, you need to integrate Newton's second law:

$$\mathbf{a}_i = \frac{\mathbf{F}_i}{m_i}$$

The code currently assumes $m = 1$ for all particles (reduced units), so $\mathbf{a}_i = \mathbf{F}_i$. This simplification is consistent with a pure reduced-unit LJ simulation where everything is measured in units of $\varepsilon$, $\sigma$, and $m$.

---

## Velocity Verlet algorithm

> **Where it actually lives:** the integrator is a plug. `Integrator` (`src/engine/integrators/integrator.h`) is an abstract class with one pure virtual method, `step(MDState& state, double dt)`. The default plug is `VelocityVerletIntegrator::step` in `src/engine/integrators/velocity_verlet_integrator.cpp`. `MDState` (`src/engine/md_state.h`) is a view of the positions, velocities and forces that `MolecularDynamics` owns, plus `computeForces()` and `wrapPositions()`.

The step is the standard Velocity Verlet, written as a half kick around the force call:

### Step 1 — advance positions, half-kick velocities with the old forces

$$\mathbf{r}(t + \Delta t) = \mathbf{r}(t) + \mathbf{v}(t)\,\Delta t + \tfrac{1}{2}\mathbf{a}(t)\,\Delta t^2$$

$$\mathbf{v}(t + \tfrac12\Delta t) = \mathbf{v}(t) + \tfrac{1}{2}\mathbf{a}(t)\,\Delta t$$

```cpp
pos[0] + velocities[i][0] * dt + forces[i][0] * dtSquaredHalf,   // likewise y, z
...
velocities[i][0] += forces[i][0] * 0.5 * dt;
```

### Step 2 — wrap into the box, recalculate forces at the new positions

```cpp
state.wrapPositions();   // no-op without PBC
state.computeForces();   // calls ForceCalculator → LJPotential
```

### Step 3 — finish the velocities with the new forces

$$\mathbf{v}(t + \Delta t) = \mathbf{v}(t + \tfrac12\Delta t) + \tfrac{1}{2}\mathbf{a}(t + \Delta t)\,\Delta t$$

```cpp
velocities[i][0] += forces[i][0] * 0.5 * dt;
```

Steps 1 and 3 together give the textbook form $\mathbf{v}(t + \Delta t) = \mathbf{v}(t) + \tfrac{1}{2}[\mathbf{a}(t) + \mathbf{a}(t + \Delta t)]\,\Delta t$.

Why Velocity Verlet instead of plain Euler? It is **symplectic** (conserves the symplectic structure of Hamiltonian mechanics), which means energy drift stays bounded over long runs rather than growing without limit. This matters enormously for NVE (microcanonical) simulations.

The other plug, `LeapfrogIntegrator` (`leapfrog_integrator.cpp`), is written as kick, drift, kick. In exact arithmetic it gives the same trajectory. Pick it with `integrator: "leapfrog"` under `simulation:` in the YAML.

---

## Thermostat — a plug too

`Thermostat` (`src/engine/thermostats/thermostat.h`) has two hooks: `beforeStep` (default: do nothing) and `afterStep` (pure virtual). The loop calls them around the integrator step while the temperature is held. The YAML key `thermostat:` picks one:

| Name | Class | What it does |
|------|-------|--------------|
| `velocity_rescale` (default) | `VelocityRescaleThermostat`, `tau = 0` | after every step, scale all velocities so the temperature is exactly the target |
| `berendsen` | `VelocityRescaleThermostat`, `tau > 0` | after every step, scale by $\sqrt{1 + (\Delta t/\tau)(T_0/T - 1)}$, so the temperature relaxes towards the target |
| `langevin` | `LangevinThermostat` | before and after the step, a friction plus random kick on every velocity (an exact Ornstein–Uhlenbeck update over $\Delta t/2$). Samples the canonical ensemble |
| `none` | no object | no temperature control (NVE) |

```yaml
thermostat: "velocity_rescale"
thermostat: { type: "berendsen", tau: 0.1 }
thermostat: { type: "langevin", friction: 2.0 }
```

The rescaling thermostats hold the temperature but do not let it fluctuate as it would in a real system; Langevin does. When `production_steps` is set, the production phase runs with no thermostat at all (NVE), and energy conservation is checked via `energyDriftTolerance`.

---

## Velocity initialisation — Maxwell-Boltzmann

Before the first step, `MolecularDynamics::initializeVelocities(temperature)` (`molecular_dynamics.cpp:124`, called from `initialize()`) draws each velocity component from a Gaussian:

$$v_\alpha \sim \mathcal{N}\!\left(0,\; \sqrt{k_B T / m}\right)$$

```cpp
// Using kB = 1 and m = 1 in reduced units
double stddev = std::sqrt(temperature);
std::normal_distribution<double> dist(0.0, stddev);
...
velocities[i][d] = dist(gen);
```

After assigning, the code removes the **centre-of-mass drift** (net momentum set to zero) so the box doesn't translate as a whole, and then **rescales** to hit the exact target temperature.

---

## The MD driver — `MolecularDynamics`

`MolecularDynamics` in `src/engine/molecular_dynamics.h/.cpp` owns the outer loop. The actual flow when you call `runSimulation()` (`.cpp:271`):

```
initialize()   ← called separately, before runSimulation()
    ├── init Maxwell-Boltzmann velocities
    └── compute initial E and F

[optional] minimizeEnergy()          ← only if params.useMinimization
    └── steepest descent / CG / L-BFGS until |F| < forceTolerance

write frame 0 ("Initial configuration")

single loop:  totalSteps = totalTime / timeStep     ← e.g. 1.0 / 0.001 = 1000
              (or equilibration_steps + production_steps, if either is set)
    ├── thermostat->beforeStep()     ← skipped in NVE production
    ├── integrator->step()           ← Velocity Verlet by default; leaves F and PE at the new positions
    ├── thermostat->afterStep()      ← skipped in NVE production
    ├── KE + PE, print to stdout every 10 steps
    └── write XYZ frame (per writeFrequency; the Python/CLI paths pass 1 ⇒ every step)
```

> **Phases:** `equilibration_steps` and `production_steps` under `simulation:` switch on phase control. Equilibration holds the temperature; production runs with no thermostat (NVE) and reports its energy drift. `data/lj_nve_simulation.yaml` is such a file. Without them, `totalTime / timeStep` sets the length and the thermostat runs the whole time. (`lj_box.equilibration_steps` / `production_steps` are parsed into `LJBoxConfig` but not read by the loop.)
>
> **Not implemented (config stubs):** `output.frequency` and `output.energy: true` in `lj_bulk_simulation.yaml` are not read; there is no output stride, and energy is only printed to stdout.

---

## Energy conservation monitoring

In NVE production the total energy $E = K + V$ should be constant. `MolecularDynamics` tracks the largest deviation from the first production step, and at the end prints

```
Production energy drift: max|E(t) - E(0)| / |E(0)|  (tolerance energyDriftTolerance, default 1%): PASS or FAIL
```

A FAIL means the energy was not conserved. Typical causes: time step too large, or particles overlapping at initialisation (which is why the lattice builder adds small random offsets to break perfect symmetry).

---

## `SimulationParameters` config struct

The config struct is `SimulationParameters`, defined in `src/engine/molecular_dynamics.h:38`. This is what `setSimulationParameters()` receives; it also builds the integrator and thermostat plugs from the names in it:

```cpp
struct SimulationParameters {
    double timeStep = 0.001;            // reduced units
    double temperature = 300.0;         // reduced units, kB = 1
    double totalTime = 1.0;             // steps = totalTime / timeStep, unless phases are set
    bool useMinimization = true;        // run minimizeEnergy() before the loop
    MinimizationParameters minimization;
    bool usePBC = false;                // minimum-image PBC
    SimulationBox box;                  // box dimensions
    std::size_t equilibrationSteps = 0; // thermostat on
    std::size_t productionSteps = 0;    // NVE, energy drift reported
    double energyDriftTolerance = 0.01; // NVE drift threshold
    unsigned int randomSeed = 0;        // 0 = auto
    std::string integrator = "velocity_verlet";  // or "leapfrog"
    std::string thermostat = "velocity_rescale"; // or "berendsen", "langevin", "none"
    double thermostatTau = 0.0;         // Berendsen time constant
    double langevinFriction = 1.0;      // Langevin gamma
};
```

`setIntegrator()` and `setThermostat()` on `MolecularDynamics` plug in an object directly instead.

---

## Key files

| File | Role |
|------|------|
| [src/engine/molecular_dynamics.cpp](../../src/engine/molecular_dynamics.cpp) | The loop (`runSimulation`, :271), MB init (`initializeVelocities`, :124), plug choice (`setSimulationParameters`, :212) |
| [src/engine/molecular_dynamics.h](../../src/engine/molecular_dynamics.h) | Driver API + `SimulationParameters` struct (:38) |
| [src/engine/md_state.h](../../src/engine/md_state.h) | `MDState`, the view the plugs work on |
| [src/engine/integrators/](../../src/engine/integrators/) | `Integrator` base, Velocity Verlet, leapfrog |
| [src/engine/thermostats/](../../src/engine/thermostats/) | `Thermostat` base, velocity rescale / Berendsen, Langevin |

---

**Next:** [04_builders_and_config.md](04_builders_and_config.md) — the loop assumes atoms, a box, and parameters already exist; the next doc answers where they come from: how the simulation is set up before the loop starts.
