# 05 — Python Interface

> **Where we are.** This is a "widen later" read (deep dive later). [04](04_builders_and_config.md) showed how a run is set up in C++ from YAML; this doc shows the same pipeline reached from Python through pybind11.

## How Python talks to C++

The C++ engine is compiled into a shared library (`_molsim.so` / `_molsim.dylib`) using **pybind11**. pybind11 is a header-only library that lets C++ classes and functions appear as native Python objects. The CMake build fetches it automatically from source.

The Python-facing module name is `_molsim` (the underscore marks it as the internal/native layer). On top of it sits `molsimworkbench.py`, a thin pure-Python convenience wrapper.

---

## What is exposed

Every major class in the C++ stack has a Python counterpart in `_molsim`:

| Python class | C++ class | What it is |
|---|---|---|
| `_molsim.Geometry` | `Geometry` | atoms + positions + box |
| `_molsim.LJPotential` | `LJPotential` | LJ energy + forces |
| `_molsim.DFTPotential` | `DFTPotential` | not implemented; raises an error when built |
| `_molsim.ForceCalculator` | `ForceCalculator` | potential aggregator |
| `_molsim.MolecularDynamics` | `MolecularDynamics` | the MD driver |
| `_molsim.SimulationParameters` | `SimulationParameters` | config struct |
| `_molsim.VelocityVerletIntegrator`, `_molsim.LeapfrogIntegrator` | `Integrator` plugs | pass to `md.setIntegrator(...)` |
| `_molsim.VelocityRescaleThermostat`, `_molsim.LangevinThermostat` | `Thermostat` plugs | pass to `md.setThermostat(...)`; `None` means no thermostat |
| `_molsim.SimulationBox` | `SimulationBox` | box dimensions |
| `_molsim.YAMLParser` | `YAMLParser` | reads config files |
| `_molsim.TrajectoryFormat` | `TrajectoryFormat` | enum: XYZ / DCD / XTC |
| `_molsim.MinimizationMethod` | `MinimizationMethod` | enum: SD / CG / LBFGS |

---

## Running a simulation in Python — two ways

### Way 1: YAML-driven (high level)

`molsimworkbench.System.from_yaml()` in `molsimworkbench.py` does all the plumbing:

```python
from molsimworkbench import System

system = System.from_yaml("data/simulation_config.yaml",
                          trajectory_output="output/trajectory.xyz")
final_energy = system.run()
print(f"Final energy: {final_energy:.4f}")
```

Internally `from_yaml` does:
1. `YAMLParser.parseSimulationConfig()` → `SimulationConfig`
2. `YAMLParser.parseGeometry()` → `Geometry` (or `LJParticleBoxBuilder.createCubicLattice()` in `lj_particle_box` mode)
3. Loop over `potentialConfigFiles` → `ForceCalculator` with potentials added
4. Wire everything into `MolecularDynamics`
5. Call `initialize()` then `runSimulation()` on `.run()`

### Way 2: Fully programmatic (low level)

You can build the simulation object-by-object:

```python
import _molsim

# 1. Build geometry manually
geo = _molsim.Geometry()
geo.addAtom("LJ", [0.0, 0.0, 0.0])
geo.addAtom("LJ", [1.1, 0.0, 0.0])
geo.setBoxDimensions([5.0, 5.0, 5.0])

# 2. Configure LJ potential
lj = _molsim.LJPotential()
lj.addAtomParameters("LJ", 1.0, 1.0)   # epsilon, sigma
lj.setCutoff(2.5)
lj.setMixingRule("geometric")

# 3. Wrap in force calculator
fc = _molsim.ForceCalculator()
fc.addPotential(lj)

# 4. Check energy before MD
print("Energy:", fc.calculateEnergy(geo))

# 5. Run MD
params = _molsim.SimulationParameters()
params.timeStep = 0.001
params.temperature = 0.1        # low enough that the pair stays bound
params.totalTime = 1.0
params.usePBC = True
box = _molsim.SimulationBox()   # forces use the geometry's box; params.box
box.a = box.b = box.c = 5.0     # is what wraps positions back into the box
params.box = box                # and labels the trajectory
params.useMinimization = False  # on by default; skip it to see raw dynamics
params.randomSeed = 42          # same velocities, same step lines, every run

md = _molsim.MolecularDynamics()
md.setGeometry(geo)
md.setForceCalculator(fc)
md.setSimulationParameters(params)
md.setTrajectoryOutput("traj.xyz", _molsim.TrajectoryFormat.XYZ, 10)
md.initialize()
md.runSimulation()
```

The pair vibrates around the LJ minimum: the potential energy swings between
about -0.95 and -1.00 while the thermostat holds the kinetic energy at 0.3.

```
Step 0, Potential Energy: -0.983345, Kinetic Energy: 0.3, Total Energy: -0.683345, Temperature: 0.1
Step 100, Potential Energy: -0.999179, Kinetic Energy: 0.3, Total Energy: -0.699179, Temperature: 0.1
Step 300, Potential Energy: -0.947841, Kinetic Energy: 0.3, Total Energy: -0.647841, Temperature: 0.1
Step 800, Potential Energy: -0.999414, Kinetic Energy: 0.3, Total Energy: -0.699414, Temperature: 0.1
```

The temperature is the setting that matters. At `temperature = 1.0` the
kinetic energy (3.0) is three times the bond energy (1.0), so the pair breaks
apart and the potential energy falls to near 0. Leaving out `params.box` does
not change the energies, since the forces read the box from the geometry, but
the atoms are then never wrapped back into the box in `traj.xyz`. Minimization
runs by default; turning it off shows the dynamics from the starting positions.

---

## How `_molsim` is found at import time

The pure-Python loader in `molsimworkbench.py` searches for the compiled `.so` in this order:

1. Anywhere already on `sys.path` (installed package)
2. `build/`
3. `build/Debug/`, `build/Release/`, etc.

This means you can just `cd` into the project root after building and `import molsimworkbench` without any install step.

---

## Force array return convention

The `ForceCalculator.calculateForces()` binding uses a lambda to convert the C++ output:

```cpp
.def("calculateForces", [](ForceCalculator& self, const Geometry& geometry) {
    std::vector<std::array<double, 3>> forces;
    self.calculateForces(geometry, forces);
    return forces;   // auto-converted to a Python list of [fx, fy, fz] lists
})
```

So in Python you get back a list of `[fx, fy, fz]` lists, one per atom. If you want NumPy arrays, wrap it:

```python
import numpy as np
forces = np.array(fc.calculateForces(geo))  # shape (N, 3)
```

---

## Build steps (reminder)

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
# produces: build/_molsim.cpython-*.so
```

Tests:
```bash
python tests/test_python_api.py
```

---

## Key files

| File | Role |
|------|------|
| [src/bindings/molsim_bindings.cpp](../../src/bindings/molsim_bindings.cpp) | pybind11 module definition |
| [molsimworkbench.py](../../molsimworkbench.py) | High-level Python API |
| [tests/test_python_api.py](../../tests/test_python_api.py) | API integration tests |
| [examples/lj/water_lj_sim.py](../../examples/lj/water_lj_sim.py) | LJ example script |
| [examples/lj_bulk_box/run_lj_bulk_sim.py](../../examples/lj_bulk_box/run_lj_bulk_sim.py) | LJ bulk box example |

---

That's the full stack. Go back to [01_architecture.md](01_architecture.md) for the overview, or dive into any individual layer doc.

**Next:** if you have not done it yet, [learn-parallel/START_HERE.md](../learn-parallel/START_HERE.md) answers how the LJ loop under all of this is made fast on many cores.
