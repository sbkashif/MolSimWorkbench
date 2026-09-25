# Parallelizing MolSimWorkbench — A Course

> You know the LJ physics. This series takes the force loop you already
> understand and makes it run on 8 cores, then shows you why the same rewrite
> is the CUDA port.

Nine lessons, built around one file: `src/engine/potentials/lj_potential.cpp`.
Everything here is measured on this repo, on this machine. No numbers are quoted
that were not produced by `benchmarks/bench_lj.cpp`.

## The short version

The LJ force loop went from **34.9 ms** to **6.6 ms** per call at N=4096.
But that headline hides the actual lesson, which is this table:

| Kernel | ms/call | What changed |
|---|---:|---|
| reference (original) | 34.89 | — |
| serial-triangular | 16.14 | **No parallelism at all.** Just deleting a `std::map` lookup from the inner loop. |
| serial-gather | 28.39 | Slower again — gave up Newton's third law to make threading possible |
| `std::thread` × 8 | 6.57 | |
| OpenMP × 8 | 6.57 | |

Threading bought **2.46×**, not 5.31×. The serial rewrite bought more than the
threads did, and the formulation change that *enabled* threading cost 1.8×
before a single thread was spawned. Lesson 7 is entirely about not lying to
yourself with the 5.31× number.

## Lessons

**Picking up where you left off?** [PROGRESS.md](PROGRESS.md).
**Want the code change in plain language?** [CODE_TOUR.md](CODE_TOUR.md).

**New here? Read [START_HERE.md](START_HERE.md) first**, and keep
[GLOSSARY.md](GLOSSARY.md) open beside you — it defines every specialist term
in plain words.

**Reading order and pacing:** — reading order,
pacing, and the commands to run.

| # | File | Covers |
|---|---|---|
| 00 | [00_starting_point.md](00_starting_point.md) | The code as it is today, and the exact lines parallelism goes into |
| 01 | [01_measure_first.md](01_measure_first.md) | Benchmark methodology; why N=125 measures nothing |
| 02 | [02_serial_refactor.md](02_serial_refactor.md) | The refactor that is simultaneously the GPU port |
| 03 | [03_scatter_vs_gather.md](03_scatter_vs_gather.md) | Choosing a parallel formulation; write conflicts |
| 04 | [04_threads_by_hand.md](04_threads_by_hand.md) | `std::thread`, partitioning, joins, per-thread state |
| 05 | [05_the_race.md](05_the_race.md) | Break it on purpose; watch conserved energy drift |
| 06 | [06_openmp.md](06_openmp.md) | The same kernel in two lines; scheduling and load imbalance |
| 07 | [07_honest_speedup.md](07_honest_speedup.md) | Amdahl, strong/weak scaling, and choosing a baseline |
| 08 | [08_not_the_hot_loop.md](08_not_the_hot_loop.md) | The O(1) dispersion tail correction; when NOT to parallelize |
| 09 | [09_where_this_goes.md](09_where_this_goes.md) | CUDA (read-only), and how GROMACS is actually built |

Exercises live in `exercises/NN-*/`, the capstone in `project/`.

## Setup

Nothing to install. OpenMP comes from Homebrew's `libomp`, which CMake finds
automatically on macOS:

```bash
brew install libomp          # only if `brew list libomp` comes up empty
cmake -B build -S .
cmake --build build --parallel
```

Confirm the parallel backend is available:

```
$ cmake -B build -S . | grep OpenMP
-- OpenMP 5.1 found - parallel LJ backend available (opt-in)
```

If OpenMP is *not* found the build still succeeds and every test still passes —
the OpenMP backend silently falls back to the serial kernel. You lose the
speedup, not the correctness.

## Running things

```bash
# The benchmark harness: correctness AND timing for every backend
./build/benchmarks/bench_lj 4096 7

# Correctness tests for the kernels
./build/tests/unit/MolSimTests --gtest_filter='LJKernels.*'

# An actual simulation, with parallelism opted into via the environment
MOLSIM_LJ_BACKEND=openmp ./build/run_simulation data/lj_bulk_simulation.yaml
```

## Parallelism is opt-in

The default backend is `LJBackend::Serial`. A stock build behaves exactly as it
did before this course existed. You turn parallelism on deliberately:

```cpp
lj.setBackend(LJBackend::OpenMP);   // C++
lj.setNumThreads(8);                // 0 = runtime default
```
```bash
MOLSIM_LJ_BACKEND=openmp            # serial | threads | openmp
MOLSIM_LJ_THREADS=4
```

That is a design decision, not laziness. A parallel default would make the
program's numerical output depend on how many cores the machine happened to
have — see lesson 5 on why `reduction(+:)` is not deterministic.

## What this course does not build

- **Neighbour / cell lists.** At ρ=0.8 and r_c=2.5σ only ~1.3% of pairs are
  inside the cutoff, so ~98.7% of the O(N²) work is computing distances in
  order to throw them away. A cell list would beat all 8 threads on one core.
  `CLAUDE.md` marks this as deliberate scope; lesson 9 explains what it would
  look like. **The all-pairs kernel is the pedagogical GPU vehicle, not the
  physicist's answer.**
- **PME / long-range electrostatics.** This system has no charges. The
  long-range term that *does* apply here — the analytic dispersion correction —
  is lesson 8, and it turns out to contribute exactly zero force.
