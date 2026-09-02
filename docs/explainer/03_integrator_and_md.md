# 03 — Integrator and MD Loop

## From forces to motion

Once you have forces from `LJPotential`, you need to integrate Newton's second law:

$$\mathbf{a}_i = \frac{\mathbf{F}_i}{m_i}$$

The code currently assumes $m = 1$ for all particles (reduced units), so $\mathbf{a}_i = \mathbf{F}_i$. This simplification is consistent with a pure reduced-unit LJ simulation where everything is measured in units of $\varepsilon$, $\sigma$, and $m$.

---

## Velocity Verlet algorithm

> **Where it actually lives:** integration is **not** a separate integrator class. It is the private method `MolecularDynamics::integrateVelocityVerlet()` in `src/engine/molecular_dynamics.cpp:440`. The classes under `src/engine/integrators/` (`velocity_verlet_integrator.h`, `md_integrator.h`, `md_parameters.h`) are unused scaffolding — nothing includes them. The code snippets below are faithful to the *algorithm*, but the real methods are all members of `MolecularDynamics`.

The method is a direct implementation of the standard 3-step Velocity Verlet:

### Step 1 — advance positions

$$\mathbf{r}(t + \Delta t) = \mathbf{r}(t) + \mathbf{v}(t)\,\Delta t + \tfrac{1}{2}\mathbf{a}(t)\,\Delta t^2$$

```cpp
newCoords[d] += velocities[i][d] * params.timeStep
              + 0.5 * forces[i][d] * params.timeStep * params.timeStep;
geometry->setCoordinates(i, newCoords);
```

### Step 2 — recalculate forces at new positions

```cpp
calculateForces();   // calls ForceCalculator → LJPotential
```

### Step 3 — update velocities using average of old and new accelerations

$$\mathbf{v}(t + \Delta t) = \mathbf{v}(t) + \tfrac{1}{2}\bigl[\mathbf{a}(t) + \mathbf{a}(t + \Delta t)\bigr]\,\Delta t$$

```cpp
velocities[i][d] += 0.5 * (oldForces[i][d] + forces[i][d]) * params.timeStep;
```

Why Velocity Verlet instead of plain Euler? It is **symplectic** (conserves the symplectic structure of Hamiltonian mechanics), which means energy drift stays bounded over long runs rather than growing without limit. This matters enormously for NVE (microcanonical) simulations.

---

## Thermostat — optional NVT

After the VV step, if `params.useConstantTemperature == true`, a velocity-rescaling thermostat is applied:

```cpp
if (params.useConstantTemperature) applyThermostat();
```

The thermostat is a simple Berendsen-style rescale. For the LJ-bulk NVE benchmark it is turned off; energy conservation is instead monitored via `energyDriftTolerance`.

---

## Velocity initialisation — Maxwell-Boltzmann

Before the first step, `MolecularDynamics::initializeVelocities(temperature)` (`.cpp:122`, called from `initialize()`) draws each velocity component from a Gaussian:

$$v_\alpha \sim \mathcal{N}\!\left(0,\; \sqrt{k_B T / m}\right)$$

```cpp
std::normal_distribution<double> distribution(0.0, 1.0);
const double kB = 8.3144621e-3;  // kJ/(mol·K)

double sigma_v = std::sqrt(kB * temperature);  // m = 1
velocities[i][d] = sigma_v * distribution(gen);
```

After assigning, the code removes the **centre-of-mass drift** (net momentum set to zero) so the box doesn't translate as a whole, and then **rescales** to hit the exact target temperature.

---

## The MD driver — `MolecularDynamics`

`MolecularDynamics` in `src/engine/molecular_dynamics.h/.cpp` owns the outer loop. The actual flow when you call `runSimulation()` (`.cpp:237`):

```
initialize()   ← called separately, before runSimulation()
    ├── init Maxwell-Boltzmann velocities
    └── compute initial E and F

[optional] minimizeEnergy()          ← only if params.useMinimization
    └── steepest descent / CG / L-BFGS until |F| < forceTolerance

write frame 0 ("Initial configuration")

single loop:  totalSteps = totalTime / timeStep     ← e.g. 1.0 / 0.001 = 1000
    ├── integrateVelocityVerlet()    ← method on MolecularDynamics
    ├── applyTemperatureControl()    ← no-op unless useConstantTemperature
    ├── recompute KE + PE, log to stdout
    └── write XYZ frame (per writeFrequency; the Python/CLI paths pass 1 ⇒ every step)
```

> **Not implemented (config stubs):** despite `equilibration_steps`, `production_steps`, and `output.frequency` in `lj_bulk_simulation.yaml`, there is **no** equilibration/production split and **no** output stride in the loop. The number of steps is `totalTime / timeStep` only; the phase fields (`SimulationParameters.equilibrationSteps/productionSteps`) are parsed but never read. Likewise `output.energy: true` writes no file — energy is only printed to stdout. This matches the `// TODO: phase-aware runSimulation` in `src/tools/run_simulation.cpp:65`.

---

## Energy conservation monitoring

In NVE mode the total energy $E = K + V$ should be constant. `MolecularDynamics` tracks the relative drift:

```
|E(t) - E(0)| / |E(0)| < energyDriftTolerance  (default 1%)
```

If this is violated, a warning is printed. Typical causes: time step too large, or particles overlapping at initialisation (which is why the lattice builder adds small random offsets to break perfect symmetry).

---

## `SimulationParameters` config struct

The live config struct is `SimulationParameters`, defined in `src/engine/molecular_dynamics.h:36` (there is a separate `md_parameters.h` in the unused integrators/ scaffolding — ignore it). This is what `setSimulationParameters()` receives:

```cpp
struct SimulationParameters {
    double timeStep = 0.001;            // ps (or reduced time units)
    double temperature = 300.0;         // K (or reduced units)
    double totalTime = 1.0;             // total time; steps = totalTime / timeStep
    bool useMinimization = true;        // run minimizeEnergy() before the loop
    MinimizationParameters minimization;
    bool usePBC = false;                // minimum-image PBC
    SimulationBox box;                  // box dimensions
    std::size_t equilibrationSteps = 0; // PARSED BUT UNUSED (see note above)
    std::size_t productionSteps = 0;    // PARSED BUT UNUSED
    double energyDriftTolerance = 0.01; // NVE drift warning threshold
    unsigned int randomSeed = 0;        // 0 = auto
};
```

---

## Key files

| File | Role |
|------|------|
| [src/engine/molecular_dynamics.cpp](../../src/engine/molecular_dynamics.cpp) | **The real code:** VV loop (`integrateVelocityVerlet`, :440), MB init (:122), thermostat (:524), PBC (:480), `runSimulation` (:237) |
| [src/engine/molecular_dynamics.h](../../src/engine/molecular_dynamics.h) | Driver API + `SimulationParameters` struct (:36) |
| `src/engine/integrators/*.h` | ⚠️ Unused scaffolding — not part of the compiled code path |

---

Next: [04_builders_and_config.md](04_builders_and_config.md) — how the simulation is set up before the loop starts.
