# Chapter 10, plain version — from the practice kitchen to the real one

This is a plain-language retelling of [10_mini_lj_to_the_repo.md](10_mini_lj_to_the_repo.md). Same
sections, same order, one picture: the robot kitchen from chapter 1. Each section is a step table, **Kitchen | C++ | Example**.
For exact line numbers and full program output, open the technical version. Its one (deep dive later)
section, on what CMake's Debug and Release settings pass to the translator, is summarised in one table
here rather than told in full. Timings are from this M1 (4 fast and 4 slow cores).

## The kitchen, extended

| Kitchen | C++ |
|---|---|
| the **practice kitchen**, where you learned one technique per lesson | `mini_lj` in `docs/learn-cpp/examples/` |
| the **real kitchen**, with its full robot book | the repo: `src/`, built into `build/` |
| the **original house recipe**, kept framed on the wall and never changed, so every new method can be checked against it | `calculateEnergyReference` / `calculateForcesReference` |
| a **test bench** where the robot cooks the same dish every way the kitchen knows and compares the results | `build/benchmarks/bench_lj` |
| **a full run**: the robot cooks from an order slip | `build/run_simulation data/lj_bulk_simulation.yaml` |
| the translator working **carefully and literally** vs **cleverly** | `-O0` vs `-O2` |

---

## Where we are

At the end of chapter 9 the practice kitchen timed its energy dish and split it between two robots, and
got `-20105.493949` every time. Over nine lessons it learned cookie cutters, a station job description, a
storeroom with sign-out sheets, a stopwatch and more robots, and each lesson ended by glancing at a few
lines of the real kitchen. Now we walk the whole real kitchen, matching every practice piece to its real
one, then have the robot do a full run.

---

## 10.0 The map

| # | Practice kitchen (chapter) | Real kitchen | Where |
|---|---|---|---|
| 1 | one pair of atoms, energy by hand (2) | `calculatePairEnergy` | `lj_potential.cpp:368-376` |
| 2 | wrapping round the box (3) | three `std::round` lines | `lj_potential.cpp:290-292` |
| 3 | laying atoms out on a lattice (4) | `createCubicLattice` | `lj_particle_box_builder.cpp:38-50` |
| 4 | the every-pair-once loop (4) | inside `calculateEnergyReference` | `lj_potential.cpp:284-302` |
| 5 | mixing, pair force (5) | `getMixedParameters`, `calculatePairForce` | `lj_potential.cpp:343-366`, `:378-394` |
| 6 | the `Geometry` and `LJPotential` cookie cutters (6) | same names | `geometry.h:12-33`; `lj_potential.cpp:277-341` |
| 7 | the job description `Potential` (7) | identical, plus one duty with default instructions | `force_calculator.h:39-60` |
| 8 | the station rack, the storeroom, the sub-recipe that builds machines (8) | `ForceCalculator`, `make_shared`, the YAML parser | `force_calculator.h:13-36`; `run_simulation.cpp:43-46`; `yaml_parser.cpp:339` |
| 9 | the stopwatch, the extra robots (9) | `timeMs`, the `Threads` backend | `bench_lj.cpp:56-68`; `lj_kernels.cpp:302-333` |

---

## 10.1 In the real code: the reference loop is mini_lj chapter 6

| # | Kitchen | Real (`lj_potential.cpp`) | Practice (`examples/06/lj_potential.cpp`) |
|---|---|---|---|
| 1 | for every atom, pair it with every later atom | `for i …`, `for j = i + 1 …` (284–285) | same (20–21) |
| 2 | measure the gap along each wall | `dx`, `dy`, `dz` (286–288) | `d[k]` in a little loop (22–26) |
| 3 | wrap round the box, **but only if that wall wraps** | `if (boxDim[0] > 0.0) …` (290–292) | always wraps (25) |
| 4 | too far apart, or **on top of each other**: skip | `distance > cutoff \|\| distance < 1e-10` (297) | only `r > cutoff` (28) |
| 5 | look up the mixed ε and σ, add the pair's energy | `getMixedParameters`, `calculatePairEnergy` (299–300) | same (30–31) |

That is the whole difference. The recipe is framed on the wall (`lj_potential.h:56-63`) so every faster
method can be checked against it. The kitchen's everyday `calculateEnergy` now uses the faster methods,
and learn-parallel is about those.

---

## 10.2 Build the real program

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | write the order book, asking for the **clever** translator | configure, Release | `cmake -B build -S . -DCMAKE_BUILD_TYPE=Release` |
| 2 | translate and bind everything | build | `cmake --build build --parallel` |
| 3 | the first time, fetch three outside cookbooks | FetchContent | needs a network connection |
| 4 | already built? the binder just checks | up to date | `[100%] Built target run_simulation`, `[100%] Built target bench_lj` |

---

## 10.3 Run the benchmark: the same energy

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the test bench sets up the same 4096-atom lattice | `bench_lj.cpp:87-97` | density 0.8, seed 42, cutoff 2.5 |
| 2 | the robot cooks the framed house recipe first | `reference` row | `-20105.493949019` |
| 3 | the practice kitchen's number | `mini_lj` ch 4 and ch 9 | `-20105.493949`: **the same dish** |
| 4 | five faster methods, same result | the other rows | all `-20105.493949019`; how they are 2–9× faster is learn-parallel |
| 5 | the small dish, 125 atoms | `bench_lj 125 5` | `-611.491665613`, the practice kitchen's chapters 4–6 number |

Do not compare the bench's 34.9 ms with chapter 9's 24.1 ms: the bench times the **forces** recipe, with
a different translator setting.

---

## 10.4 Run a simulation

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | switch the robot on at the kitchen's front door (the repo root) | the YAML reader looks for a helper script relative to where you are | started elsewhere: `Python YAML parser script not found` |
| 2 | hand it the order slip, with a second word saying where to put the trajectory | `build/run_simulation data/lj_bulk_simulation.yaml /tmp/trajectory.xyz` | second word keeps `trajectory.xyz` out of the repo |
| 3 | same 125-atom lattice as the practice kitchen | | `N=125, rho=0.8, L=5.38609` |
| 4 | the first line on the serving tray comes **after** one step of cooking | velocity Verlet runs before the print | `Step 0, Potential Energy: -611.553` (not −611.4917: the atoms have already moved) |
| 5 | the order slip names no warm-up or main course, so the plain run length is cooked: 1000 steps | no `equilibration_steps` / `production_steps`, so `totalTime / timeStep` (`molecular_dynamics.cpp:295-297`) | last line `Step 990` |
| 6 | the run is finished | | `Final energy: -458.604` |

---

## 10.5 What the optimiser is worth: `-O0` vs `-O2`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the translator renders every line literally, word for word | `-O0` (also what you get with no `-O` at all) | ch 4 `mini_lj`, N = 4096: about 314 ms |
| 2 | the translator rewrites the recipe cleverly: keeps things in hand instead of putting them down and picking them up, folds small sub-recipes into the main one, does once what never changes | `-O2` | same program: 27 ms |
| 3 | same dish both times | same energy | **about 12×** from one flag |
| 4 | with the proper stopwatch (ch 9 program) | `-O0` vs `-O2`, serial | 356.5 ms vs 24.1 ms: **about 15×** |
| 5 | rule | never time an `-O0` build | learn-parallel 01 says the same |

### Debug and Release in CMake (deep dive later)

| Order book says | Translator flags | Kitchen |
|---|---|---|
| nothing | this repo fills in `Release` for you (`CMakeLists.txt:9-10`) | the cleverest translation, as in the last row |
| `Debug` | `-g` | literal translation, with page-and-line notes for a debugger |
| `Release` | `-O3 -DNDEBUG` | the cleverest translation; the "check this" asserts are removed |

A plain `cmake -B build -S .`, and `scripts/ci/run_all.sh` (which names no build type), therefore give an
optimised Release build. For a debug build, ask for it: `-DCMAKE_BUILD_TYPE=Debug`.

---

## 10.6 C++ you will still meet in the repo

| # | Kitchen | C++ | Where it is explained |
|---|---|---|---|
| 1 | "if there is a note on the door, read it" | `if (const char* env = std::getenv(…))` (`lj_potential.cpp:19`) | technical §10.6 |
| 2 | turn the note's text into a number | `std::atoi` (`lj_potential.cpp:35`) | technical §10.6 |
| 3 | a fixed list of choices, nothing else accepted | `enum class LJBackend` | [GLOSSARY](../learn-parallel/GLOSSARY.md) |
| 4 | recipes private to one card | anonymous `namespace { … }` | GLOSSARY |
| 5 | "if the kitchen has the OpenMP gadget, use this page, else that one" | `#ifdef _OPENMP` | GLOSSARY |
| 6 | pick one branch by that choice | `switch` / `case` | GLOSSARY |
| 7 | finding a card in the tabbed box: a finger on its position | `find`, `it->second` (`lj_potential.cpp:107`, `:111`) | technical §10.6 |
| 8 | a number worked out by the translator, before the robot is even switched on | `constexpr` (`lj_potential.cpp:232`) | technical §10.6 |

---

## 10.7 Where to read next

| # | Read | Kitchen |
|---|---|---|
| 1 | [explainer 01, Architecture](../explainer/01_architecture.md) | the floor plan: which station passes what to which |
| 2 | [explainer 02, LJ implementation](../explainer/02_lj_implementation.md) | the LJ station in detail |
| 3 | [explainer 03, Integrator and MD](../explainer/03_integrator_and_md.md) | how a full run goes, step by step |
| 4 | [explainer 04, Builders and config](../explainer/04_builders_and_config.md) | setting up the room and reading the order slip |
| 5 | [learn-parallel START_HERE](../learn-parallel/START_HERE.md) | making the LJ station fast with many robots |

[`explainer/00_cpp_primer.md`](../explainer/00_cpp_primer.md) is now a review.

---

## Next

Read [`docs/explainer/01_architecture.md`](../explainer/01_architecture.md) next: the floor plan of the
real kitchen, with the same stations you built in practice. After explainer 04, go to
[`docs/learn-parallel/START_HERE.md`](../learn-parallel/START_HERE.md). Its lesson 00 opens on the framed
house recipe, the loop you wrote in chapter 4, and its "Two actual atoms" works out −0.890965 and
−2.211693: the numbers you printed in chapter 2 (`-0.890965`, `-2.21169`).
