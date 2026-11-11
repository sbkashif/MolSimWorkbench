# Chapter 10 — From mini_lj to the real repo

Plain version: [10p_mini_lj_to_the_repo_plain.md](10p_mini_lj_to_the_repo_plain.md).
No new `mini_lj` code in this chapter. It uses [`examples/04/`](examples/04/) and
[`examples/09/`](examples/09/), and the repo's own programs in `build/`. Every output below was produced by
running them on this machine: an M1 (4 performance + 4 efficiency cores), Apple clang 17. Your timings
will differ; your energies should not.

**After this chapter you can say:**

- which repo file and function each piece of `mini_lj` corresponds to, chapter by chapter;
- that `LJPotential::calculateEnergyReference` in the repo *is* `mini_lj`'s chapter 6 loop, and what its
  few extra lines are for;
- how to build the repo in Release mode, run `bench_lj` and `run_simulation`, and read the energy that
  matches `mini_lj` to every printed digit;
- what `-O0` vs `-O2` is worth on this loop (about 12–15×), and what CMake's Debug and Release build
  types pass to the compiler;
- which C++ constructs in the repo this course has not taught, and where each one is explained;
- the reading order from here: explainer 01–04, then learn-parallel.

Time: about 30 minutes, including building and running.

---

## Where we are

Chapter 9 ended with `mini_lj` timing its own energy loop and splitting it across two threads, landing on
`-20105.493949` every time. Over nine chapters it grew classes, a `Potential` interface, shared ownership,
a timer and threads, and each chapter ended on a few real repo lines. What has not happened yet is laying
the *whole* of `mini_lj` over the *whole* of the repo. This chapter does that, then builds and runs the
real program and checks the numbers.

---

## 10.0 The map

Every `mini_lj` piece has a twin in the repo. Line numbers are in the current source.

| ch | `mini_lj` (`docs/learn-cpp/examples/`) | repo | where |
|---|---|---|---|
| 2 | one pair: `E = 4ε[(σ/r)¹² − (σ/r)⁶]` with `std::pow` | `LJPotential::calculatePairEnergy` | `src/engine/potentials/lj_potential.cpp:368-376` |
| 3 | `applyMinimumImage(Vec3& d, double box)` | the three `std::round` lines inside the reference loops | `lj_potential.cpp:290-292` |
| 3 | `using Vec3 = std::array<double, 3>` | written out in full as `std::array<double, 3>` everywhere | e.g. `src/builder/geometry.h:20` |
| 4 | the cubic lattice loop | `LJParticleBoxBuilder::createCubicLattice` | `src/builder/lj_particle_box_builder.cpp:38-50` |
| 4 | the `i < j` pair loop in `main` | the body of `calculateEnergyReference` | `lj_potential.cpp:284-302` |
| 5 | `getMixedParameters` returning `std::pair`; `auto [epsilon, sigma]` | same name, plus an arithmetic mixing rule | `lj_potential.cpp:343-366`, `:299` |
| 5 | `calculatePairForce(…, Vec3& force1, Vec3& force2)` | same name, same out-parameters | `lj_potential.cpp:378-394` |
| 5 | `makeCubicLattice(…, double& box)` | `createCubicLattice(config, boxSize)`: box handed back the same way | `lj_particle_box_builder.cpp:13-16` |
| 6 | `class Geometry` | `class Geometry` (same public shape, a few more methods) | `src/builder/geometry.h:12-33` |
| 6 | `class LJPotential` and its `calculateEnergy` / `calculateForces` | `calculateEnergyReference` / `calculateForcesReference` | `lj_potential.cpp:277-341` |
| 7 | `class Potential` (pure virtual, virtual destructor) | identical, plus a fourth method with a default body, `calculateForcesAndEnergy` | `src/engine/force_calculator.h:39-60` |
| 7 | `HarmonicTether`, a second potential | `DFTPotential` plays that role in the repo | `src/engine/potentials/dft_potential.h:10-17` |
| 8 | `class ForceCalculator` holding `vector<shared_ptr<Potential>>` | identical idea | `force_calculator.h:13-36` |
| 8 | `buildForceCalculator()` with `make_shared` | `run_simulation.cpp` + `YAMLParser::parseClassicalPotential` | `src/tools/run_simulation.cpp:43-46`, `src/utils/yaml_parser.cpp:339` |
| 9 | `timeMs` template in `timing.h` | `timeMs` in the benchmark | `benchmarks/bench_lj.cpp:56-68` |
| 9 | threads, each writing `partial[t]` | the `Threads` backend | `src/engine/potentials/lj_kernels.cpp:302-333` |
| — | `main` | two programs: the simulation, and the benchmark | `run_simulation.cpp:10-77`, `bench_lj.cpp:83-163` |

The repo has more around these pieces (YAML parsing, the MD integrator, trajectory output, the fast
kernels), but nothing in this table is a different idea from the one you practised.

---

## 10.1 In the real code: the reference loop is mini_lj chapter 6

`src/engine/potentials/lj_potential.cpp:277-305`, the repo's original energy loop, kept unchanged as the
correctness baseline:

```cpp
277  double LJPotential::calculateEnergyReference(const Geometry& geometry) {
278      double totalEnergy = 0.0;
279
280      const auto& atoms = geometry.getAtoms();
281      const auto& positions = geometry.getPositions();
282      const auto& boxDim = geometry.getBoxDimensions();
283
284      for (size_t i = 0; i < atoms.size(); ++i) {
285          for (size_t j = i + 1; j < atoms.size(); ++j) {
286              double dx = positions[j][0] - positions[i][0];
287              double dy = positions[j][1] - positions[i][1];
288              double dz = positions[j][2] - positions[i][2];
289
290              if (boxDim[0] > 0.0) dx = dx - boxDim[0] * std::round(dx / boxDim[0]);
291              if (boxDim[1] > 0.0) dy = dy - boxDim[1] * std::round(dy / boxDim[1]);
292              if (boxDim[2] > 0.0) dz = dz - boxDim[2] * std::round(dz / boxDim[2]);
293
294              double distanceSquared = dx*dx + dy*dy + dz*dz;
295              double distance = std::sqrt(distanceSquared);
296
297              if (distance > cutoff || distance < 1e-10) continue;
298
299              auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
300              totalEnergy += calculatePairEnergy(epsilon, sigma, distance);
301          }
302      }
303
304      return totalEnergy;
305  }
```

And `mini_lj`'s chapter 6 version (`examples/06/lj_potential.cpp:14-35`), for comparison:

```cpp
14  double LJPotential::calculateEnergy(const Geometry& geometry) {
15      const auto& atoms = geometry.getAtoms();
16      const auto& positions = geometry.getPositions();
17      const auto& box = geometry.getBoxDimensions();
18      double totalEnergy = 0.0;
19
20      for (std::size_t i = 0; i < atoms.size(); ++i) {
21          for (std::size_t j = i + 1; j < atoms.size(); ++j) {
22              Vec3 d;
23              for (int k = 0; k < 3; ++k) {
24                  d[k] = positions[j][k] - positions[i][k];
25                  d[k] -= box[k] * std::round(d[k] / box[k]);
26              }
27              const double r = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
28              if (r > cutoff) continue;
29
30              auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
31              totalEnergy += calculatePairEnergy(epsilon, sigma, r);
32          }
33      }
34      return totalEnergy;
35  }
```

Line for line, the differences are:

| repo | mini_lj | what the repo's version adds |
|---|---|---|
| 286–288: `dx`, `dy`, `dz` | 22–26: `Vec3 d` and a `k` loop | nothing: the same arithmetic, written out per axis |
| 290–292: `if (boxDim[0] > 0.0)` | 25: no test | a box length of 0 means "this axis does not wrap"; `mini_lj` always has a box |
| 297: `distance < 1e-10` | 28: only `r > cutoff` | skips two atoms sitting on the same point (division by zero in `calculatePairEnergy`) |
| `size_t` | `std::size_t` | the same type, without the `std::` prefix |
| 299: `getMixedParameters` | 30: same | the repo's version (343–366) returns `{0, 0}` for an unknown type and supports `"arithmetic"` mixing |

That is the whole difference. `calculateForcesReference` (`:307-341`) is `mini_lj`'s chapter 6
`calculateForces` in the same way. The header says what they are for (`src/engine/potentials/lj_potential.h:56-63`):

```cpp
56      // ---- Reference implementation ------------------------------------------
57      // The ORIGINAL triangular (i<j) loop, kept verbatim as the correctness
58      // baseline that every parallel backend is tested against. Slow on purpose:
59      // it does a std::map<std::string> lookup per pair. Do not "optimise" it --
60      // its entire value is being the thing we did not touch.
61      double calculateEnergyReference(const Geometry& geometry);
62      void calculateForcesReference(const Geometry& geometry,
63                                    std::vector<std::array<double, 3>>& forces);
```

The repo's *default* `calculateEnergy` (`lj_potential.cpp:147-157`) no longer runs this loop. It builds
the `LJKernelView` of chapter 8 §8.8 and calls the fast kernels in `lj_kernels.cpp`. Those kernels, and why
they look the way they do, are what learn-parallel teaches. They start from the loop above.

---

## 10.2 Build the real program

From the repo root (see also `CLAUDE.md`):

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The first configure downloads pybind11, nlohmann/json and GoogleTest, so it needs a network connection.
On macOS, `scripts/ci/run_all.sh` sets the compiler and SDK paths for you and also runs the tests (§10.5.1 explains
which build type you get). This checkout was already built, so rebuilding the two programs we need
only confirmed they were up to date:

```bash
cmake --build build --target run_simulation bench_lj
```
```
[ 85%] Built target EngineLib
[100%] Built target run_simulation
[ 42%] Built target BuilderLib
[ 85%] Built target EngineLib
[100%] Built target bench_lj
```

`BuilderLib` and `EngineLib` are the two static libraries from chapter 1 §1.4; each program is linked
against them.

---

## 10.3 Run the benchmark: the same energy

`bench_lj` builds the same N = 4096 lattice as `mini_lj` (`bench_lj.cpp:87-92`: density 0.8, seed 42,
`createCubicLattice`), sets ε = σ = 1 and cutoff 2.5 (`:95-97`), then times the reference loop and each
fast kernel:

```bash
build/benchmarks/bench_lj 4096 7
```
```
Creating LJ particle box: N=4096, rho=0.8, L=17.2355
Placed 4096 particles on cubic lattice

N = 4096   rho = 0.80   L = 17.235   cutoff = 2.5   reps = 7
OpenMP: yes  (default threads = 8)

backend               ms/call    vs ref           energy      max|df|      max rel
reference              34.876     1.00x -20105.493949019            -            -
serial-triangular      15.075     2.31x -20105.493949019    1.137e-13    1.490e-11
serial-gather          28.307     1.23x -20105.493949019    1.137e-13    1.490e-11
std::thread             6.566     5.31x -20105.493949019    1.137e-13    1.490e-11
openmp-gather           5.961     5.85x -20105.493949019    1.137e-13    1.490e-11
openmp-private          3.810     9.15x -20105.493949019    1.279e-13    1.158e-11

Best serial kernel: serial-triangular at 15.075 ms.
Speedup measured against THAT (the number you may honestly quote):
    std::thread        2.30x   (vs reference it would look like 5.31x)
    openmp-gather      2.53x   (vs reference it would look like 5.85x)
    openmp-private     3.96x   (vs reference it would look like 9.15x)
...
```

Look only at the `reference` row for now. Its energy is `-20105.493949019`. `mini_lj` printed
`E = -20105.493949` in chapter 4 (and again in chapter 9), with 12 significant digits: the same number. The
lattice is identical, the loop is identical, the arithmetic is identical.

The other five rows are the subject of learn-parallel. Every one of them gets the same energy as the
reference; how they get there 2–9× faster is what those lessons explain.

Two cautions when comparing times. `bench_lj`'s reference row times `calculateForcesReference`
(`bench_lj.cpp:107`), which does more per pair than `mini_lj`'s energy-only loop, so 34.9 ms here and
24.1 ms in chapter 9 are not the same measurement. And `bench_lj` was built with `-O3` (Release, §10.5.1),
`mini_lj` with `-O2`.

The same check at the repo's default size:

```bash
build/benchmarks/bench_lj 125 5
```
```
...
backend               ms/call    vs ref           energy      max|df|      max rel
reference               0.262     1.00x   -611.491665613            -            -
serial-triangular       0.045     5.88x   -611.491665613    1.066e-13    3.413e-13
...
```

`-611.491665613`: `mini_lj`'s N = 125 energy from chapters 4, 5 and 6.

---

## 10.4 Run a simulation

`run_simulation` reads a YAML config (ch 1 §1.2 walked its `main`). Run it from the repo root: the YAML
loader shells out to a Python helper and looks for `yaml_parser.py` relative to the current directory
(`src/utils/yaml_parser.cpp:46-55`). Started from another directory, it failed with:

```
Warning: Python YAML parser script not found at any expected location
...
Simulation failed: Failed to parse YAML file: [json.exception.parse_error.101] parse error at line 1, column 1: syntax error while parsing value - unexpected end of input; expected '[', '{', or a literal
```

By default it writes `trajectory.xyz` into the current directory. To keep the repo clean, give a second
argument with somewhere else to put it (`run_simulation.cpp:17`):

```bash
build/run_simulation data/lj_bulk_simulation.yaml /tmp/trajectory.xyz
```

The output is 133 lines. The part that matters here:

```
=== LJ Particle Box Mode ===
Creating LJ particle box: N=125, rho=0.8, L=5.38609
Placed 125 particles on cubic lattice
LJ box initialized: 125 particles
...
Initial kinetic energy: 187.5
Step 0, Potential Energy: -611.553, Kinetic Energy: 187.5, Total Energy: -424.053, Temperature: 1
Step 10, Potential Energy: -608.269, Kinetic Energy: 187.5, Total Energy: -420.769, Temperature: 1
...
Step 990, Potential Energy: -645.364, Kinetic Energy: 187.5, Total Energy: -457.864, Temperature: 1

Simulation complete.
Final energy: -458.604
Trajectory written to: …/trajectory.xyz
```

(My run pointed the second argument at a scratch directory; the path is shortened to `…` here. Yours
prints whatever path you gave.)

Three things to read off it:

- **Same box, same atoms.** N = 125, ρ = 0.8, L = 5.38609: the system of `mini_lj` chapters 4–8.
- **Step 0 is −611.553, not −611.491665613.** The "Step 0" line is printed *after* the first
  velocity-Verlet step: the loop body integrates first, then prints the energy
  (`src/engine/molecular_dynamics.cpp:308-362`: `integrator->step(...)` at 319, the print at 341–349). The atoms have already moved by one timestep. The
  energy of the starting lattice is computed at `molecular_dynamics.cpp:265` but not printed; `bench_lj
  125` above shows it is `-611.491665613`.
- **It ran 1000 steps.** The file sets neither `equilibration_steps` nor `production_steps`, so the
  loop runs `totalTime / timeStep` = 1.0 / 0.001 steps (`molecular_dynamics.cpp:295-297`).
  `data/lj_nve_simulation.yaml` sets both, and then the run is split into the two phases.

The kinetic energy stays at exactly 187.5 because the run is thermostatted. The integrator and the
thermostat are [explainer 03](../explainer/03_integrator_and_md.md).

---

## 10.5 What the optimiser is worth: `-O0` vs `-O2`

Every `mini_lj` command used `-O2`. That flag asks the compiler to **optimise**: rearrange and simplify
the machine code while keeping what the program computes the same. Without any `-O` flag clang uses
`-O0`: translate each line as literally as possible, which is easiest to debug and slowest to run.

Chapter 4's single-file `mini_lj`, both ways, at N = 4096 (whole-program wall time, best of five runs):

```bash
cd docs/learn-cpp/examples/04
mkdir -p out
c++ -std=c++17 -Wall -O0 mini_lj.cpp -o out/mini_lj_O0
c++ -std=c++17 -Wall -O2 mini_lj.cpp -o out/mini_lj_O2
TIMEFMT='%mE'                                # zsh: print elapsed time in ms
for k in 1 2 3 4 5; do time ./out/mini_lj_O0 4096 >/dev/null; done
for k in 1 2 3 4 5; do time ./out/mini_lj_O2 4096 >/dev/null; done
```
```
319ms
314ms
326ms
326ms
340ms
```
```
27ms
27ms
27ms
27ms
27ms
```

314 ms against 27 ms: **about 12× from one flag**, and the energy printed is the same. Chapter 9's
program has a proper timer (warm-up and minimum, §9.3), so it is a cleaner measurement. Built at `-O0`:

```bash
cd ../09
c++ -std=c++17 -Wall -O0 main.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/mini_lj_O0
./out/mini_lj_O0 4096 2
```
```
N = 4096, threads = 2
serial   E = -20105.493949   356.54975 ms
threaded E = -20105.493949   262.199417 ms
speedup  = 1.36x
```

356.5 ms serial at `-O0` against 24.1 ms at `-O2` (§9.6): **about 15×**. The thread speedup, 1.36× here
against 1.38× at `-O2`, survived because the imbalance of §9.7 is a property of the loop, not of the
compiler.

What does the optimiser do with 8.4 million passes through a small loop body? Among other things: keeps
values in CPU registers instead of writing every variable to memory and reading it back; copies small
functions such as `calculatePairEnergy` into the loop instead of calling them (**inlining**); and moves
work that does not change between passes out of the loop. At `-O0` it does none of that.

The practical rule, also in learn-parallel 01's pitfalls: **never time an unoptimised build.** It is
slower by an order of magnitude, and *which* parts are slow changes too, so you would end up optimising
the wrong thing.

### 10.5.1 Debug and Release in CMake (deep dive later)

CMake chooses compiler flags from `CMAKE_BUILD_TYPE`. On this machine, with Apple clang, a minimal
project configured three ways compiled its source file with:

| `-DCMAKE_BUILD_TYPE=` | flags CMake added | meaning |
|---|---|---|
| *(not given)* | none | clang's default: `-O0`, no debug information |
| `Debug` | `-g` | `-O0`, plus debug information (file and line numbers for a debugger) |
| `Release` | `-O3 -DNDEBUG` | the strongest standard optimisation level, and `NDEBUG` defined, which turns every `assert(...)` into nothing |

This checkout's `build/` was configured as Release: `build/CMakeCache.txt` contains
`CMAKE_BUILD_TYPE:STRING=Release` and `CMAKE_CXX_FLAGS_RELEASE:STRING=-O3 -DNDEBUG`. The build type is
stored in that cache and kept by later `cmake --build` runs until you configure again.

If you give no build type at all, `CMakeLists.txt` now picks Release for you (the block just above
`set(CMAKE_CXX_STANDARD 17)`), so a plain `cmake -B build -S .`, or `scripts/ci/run_all.sh`, gives an
optimised build. Before that block was added, a configure with no build type got the first row of the
table, no optimisation at all, and every timing in the parallel course would have been 12–15× too slow.
Pass `-DCMAKE_BUILD_TYPE=Debug` explicitly when you want a debug build.

---

## 10.6 C++ you will still meet in the repo

This course taught the C++ that the LJ path is made of. Reading the repo, you will meet a few more
constructs. Most are already explained in the learn-parallel glossary's section **"C++ you will meet in
`lj_kernels.{h,cpp}`"** ([GLOSSARY.md](../learn-parallel/GLOSSARY.md)); this table only says where they are
and points there.

| Construct | Where | Explained |
|---|---|---|
| `if (const char* env = std::getenv("MOLSIM_LJ_BACKEND"))` | `lj_potential.cpp:19` | below |
| `std::atoi(env)` | `lj_potential.cpp:35` | below |
| `enum class LJBackend { … }` | `lj_kernels.h:58` | GLOSSARY: `enum class` |
| `namespace { … }` (anonymous namespace) | `lj_kernels.cpp:12` to `:271` | GLOSSARY: Anonymous namespace |
| `#ifdef _OPENMP` … `#else` … `#endif` | `lj_kernels.cpp:8`, `:284` | GLOSSARY: `#ifdef _OPENMP`; the preprocessor is ch 1 §1.3 |
| `switch (backend) { case …: … break; }` | `lj_kernels.cpp:282` | GLOSSARY: `switch` / `case` / `break` |
| `static_cast<std::size_t>(t)` | `lj_kernels.cpp:307` and throughout | GLOSSARY: `static_cast<T>(x)` |
| `#pragma omp parallel for …` | `lj_kernels.cpp:352` | learn-parallel 06 |
| `auto it = typeIds.find(atoms[i]);` … `it->second` | `lj_potential.cpp:107`, `:111` | below |
| `constexpr double kPi = …;` | `lj_potential.cpp:232` | below |

The four marked "below", briefly:

- **`if (T x = expr)`** declares `x`, then tests it. `std::getenv` returns a `const char*` to the
  environment variable's text, or `nullptr` if it is not set; a null pointer tests as false. So the body
  runs only when the variable is set, and `env` exists only inside the `if`.
- **`std::atoi`** (from `<cstdlib>`) turns text into an `int`, returning 0 if the text is not a number.
- **`map.find(key)`** returns an **iterator**: a position inside the map. If the key is missing it
  returns `map.end()`, a "one past the last" position, which is why line 111 compares `it != typeIds.end()`
  first. For a map, the iterator points at a key–value pair: `it->first` is the key, `it->second` the
  value.
- **`constexpr`** means "a constant the compiler can work out while compiling". `kPi` is fixed before
  the program ever runs. It is a stronger `const` (ch 2).

> **Unlike Python:** `int("abc")` raises `ValueError`; `std::atoi("abc")` returns 0 with no error at all.
> So `MOLSIM_LJ_THREADS=four` is not rejected at line 35: it silently reads as 0.

---

## 10.7 Where to read next

`mini_lj` was the wide part of this path: one C++ idea at a time, practised on a small program. From here
the path narrows onto this one program, then onto its parallel loop.

| Order | Read | What you get | Uses from this course |
|---|---|---|---|
| 1 | [explainer 01, Architecture](../explainer/01_architecture.md) | how Geometry, the potentials, `ForceCalculator` and `MolecularDynamics` fit together, and how a run flows through them | ch 6–8: classes, `Potential`, `shared_ptr` ownership |
| 2 | [explainer 02, LJ implementation](../explainer/02_lj_implementation.md) | how the LJ equations live in the code: energy, forces, cutoff, periodic boundaries | ch 2–6 and §10.1 |
| 3 | [explainer 03, Integrator and MD](../explainer/03_integrator_and_md.md) | velocity Verlet, starting velocities, the thermostat: why §10.4's kinetic energy stays at 187.5 | ch 6–8 |
| 4 | [explainer 04, Builders and config](../explainer/04_builders_and_config.md) | the lattice builder and the three layers of YAML | ch 4–5, §10.4 |
| 5 | [learn-parallel START_HERE](../learn-parallel/START_HERE.md) | the plan for the parallel course: run `bench_lj` (done, §10.3), then lessons 00–09 | ch 9 |

[`explainer/00_cpp_primer.md`](../explainer/00_cpp_primer.md) walks `lj_potential.h` keyword by keyword.
After this course it is a review: skim it or skip it.

---

## Next

Read [`docs/explainer/01_architecture.md`](../explainer/01_architecture.md) next. It shows how the pieces
you built in `mini_lj` (Geometry, potentials behind one interface, a `ForceCalculator` that owns them, a
driver that owns that) sit together in the full program, layer by layer. After explainer 04, go to
[`docs/learn-parallel/START_HERE.md`](../learn-parallel/START_HERE.md). Its lesson
[00](../learn-parallel/00_starting_point.md) opens on `calculateForcesReference`: the loop `mini_lj`
wrote in chapter 4 and put in a class in chapter 6. Its "Two actual atoms" section works out
U(1.2) = −0.890965 and F(1.2) = −2.211693: chapter 2's `mini_lj` printed `E(1.2) = -0.890965` and
`F(1.2) = -2.21169`, the same numbers to six digits.
