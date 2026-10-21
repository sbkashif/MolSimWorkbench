# Start Here

Reading and working plan for the parallelization course. Follow it top to
bottom; everything else in this folder is reference.

**Before this course:** C++ basics from [docs/learn-cpp/](../learn-cpp/) (chapters 01–10), and this
program's structure from [docs/explainer/](../explainer/README.md) 01–04. If you did learn-cpp, you have
already written lesson 00's loop: `mini_lj` in learn-cpp chapters 04 and 06 computes the same reference
energy, −20105.493949 at N=4096, as the `reference` row of `bench_lj` in Step 0 below.

---

## Step 0 — See the thing you are going to explain (5 min)

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/benchmarks/bench_lj 4096 7
```

You should get something close to this:

```
backend               ms/call    vs ref           energy      max|df|      max rel
reference              34.850     1.00x -20105.493949019            -            -
serial-triangular      15.221     2.29x -20105.493949019    1.137e-13    1.490e-11
serial-gather          28.285     1.23x -20105.493949019    1.137e-13    1.490e-11
std::thread             6.562     5.31x -20105.493949019    1.137e-13    1.490e-11
openmp-gather           5.969     5.84x -20105.493949019    1.137e-13    1.490e-11
openmp-private          3.773     9.24x -20105.493949019    1.279e-13    1.158e-11
```

Every lesson exists to explain one row of that table. Do not read on until you
have run it — the whole course is calibrated to numbers from *your* machine, and
yours will differ from these.

Lesson 2's ablation tables have their own tool:

```bash
./build/benchmarks/bench_ablation 4096
```

---

## Step 1 — Read (about 2 hours, splits cleanly into two sittings)

Keep **[GLOSSARY.md](GLOSSARY.md)** open in another tab. It defines every
specialist term in plain words, with why it matters here. Lessons 03, 05, 06 and
09 each open with an "In plain English" box that gives you the whole idea before
the technical version — read that box first, always.


### Sitting one — why the loop is slow (~70 min)

| Read | Have this open beside it | Take away |
|---|---|---|
| [00_starting_point.md](00_starting_point.md) | `src/engine/potentials/lj_potential.cpp` | the four things wrong with the loop, and where parallelism gets inserted |
| [01_measure_first.md](01_measure_first.md) | `benchmarks/bench_lj.cpp` | warm-up, minimum-not-mean, and why N=125 measures nothing |
| [02_serial_refactor.md](02_serial_refactor.md) | `src/engine/potentials/lj_kernels.cpp` | **the most useful lesson.** My hypothesis was wrong; the ablation says why |
| [03_scatter_vs_gather.md](03_scatter_vs_gather.md) | — | the one genuine design decision in the course |

**If you read nothing else, read 00 and the section "A hypothesis, and its
demolition" in 02.**

### Sitting two — making it parallel (~60 min)

| Read | Take away |
|---|---|
| [04_threads_by_hand.md](04_threads_by_hand.md) | the machinery OpenMP hides, and which CUDA concept each piece becomes |
| [05_the_race.md](05_the_race.md) | 2.1% of atoms wrong, aggregate error ≈ 0, and it is the *fastest* version |
| [06_openmp.md](06_openmp.md) | two lines replace all of lesson 4; `schedule(static)` on 5 threads is slower than on 4 |
| [07_honest_speedup.md](07_honest_speedup.md) | 4.03×, not 9.24×, and how to tell which number someone is quoting you |
| [08_not_the_hot_loop.md](08_not_the_hot_loop.md) | 8.73% of the energy was missing, and it contributes zero force |
| [09_where_this_goes.md](09_where_this_goes.md) | the CUDA port (read-only) and why GROMACS looks the way it does |

---

## Step 2 — Write code

```bash
cmake -B build-learn -S . -DMOLSIM_BUILD_LEARN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-learn --parallel
cd build-learn && ctest -L learn --output-on-failure
```

All 64 exercise tests fail at the start. That is correct — they are the
specification.

Work through `exercises/NN-*/` in order. Each contains:

- `README.md` — the task and the acceptance criteria
- `src/*.cpp` — stubs with numbered `TODO`s explaining what and why
- `tests/` — **do not edit these**; they define "done"
- `hints.md` — three collapsible hints, from a nudge to nearly the answer

Run one exercise at a time:

```bash
cd build-learn && ctest -L learn -R Ex01 --output-on-failure
cd build-learn && ctest -L learn -R Ex02 --output-on-failure
```

Three of them build a runnable tool worth actually running:

```bash
./build-learn/exercises/ex05_demo        # watch the race corrupt real forces
./build-learn/exercises/ex06_schedule    # find the knee on YOUR cores
./build-learn/exercises/ex07_scaling     # your own strong-scaling study
```

### Suggested pacing

| Session | Do | Roughly |
|---|---|---|
| 1 | Read sitting one. Exercises 01, 02 | 2½ h |
| 2 | Exercise 03. Read 04, 05 | 2 h |
| 3 | Exercises 04, 05 — including the race demo | 2½ h |
| 4 | Read 06, 07. Exercises 06, 07 | 2½ h |
| 5 | Read 08, 09. Exercise 08, and the paper questions in 09 | 2 h |
| 6 | Capstone | 3–4 h |

---

## Step 3 — Capstone

`project/README.md`. Write all four backends yourself against the real
`LJKernelView` seam, pass 13 acceptance tests, clear a 2× performance bar, and
fill in `project/REPORT.md` — which is where you show you understand *why*, since
`ctest` cannot grade judgement.

```bash
cd build-learn && ctest -L learn -R Capstone --output-on-failure
./build-learn/project/capstone_report 4096
```

---

## Four things to hold on to

**1. The serial rewrite was worth more than the threads.**

```
34.85 ms  original
15.22 ms  ← 2.29x, zero threads involved
 3.77 ms  ← 4.03x more, from 8 threads
```

**2. Half that serial rewrite is just "how it should have been written"; half was
bought.** Flat tables, r² arithmetic and hoisted reciprocals are free wins with
no downside. Structure-of-arrays costs 13% on this CPU, and the gather
formulation costs 1.76× — both are deliberate trades for parallelism and GPU
portability, not improvements. Lesson 2 marks which is which.

**3. Quote speedup against the fastest correct serial code.** 4.03×, not 9.24×.
The difference is the serial work being silently re-credited to threading.

**4. Algorithm beats parallelism.** The pair loop evaluates 8,386,560 pairs and
only 113,910 (1.36%) are inside the cutoff. A cell list on one core would beat
all eight of yours. `CLAUDE.md` marks neighbour lists as deliberate scope, so
this course does not build one — but the all-pairs kernel is the right thing to
*learn* on (it is the canonical GPU example), not the right thing to *ship*.

---

## Turning parallelism on in real runs

It is opt-in. The default is `LJBackend::Serial`, so a stock build is unchanged
and deterministic.

```bash
MOLSIM_LJ_BACKEND=openmp-private ./build/run_simulation data/lj_bulk_simulation.yaml
MOLSIM_LJ_THREADS=4              # optional; default is omp_get_max_threads()
```

```cpp
lj.setBackend(LJBackend::OpenMPPrivate);   // serial | threads | openmp | openmp-private
lj.setNumThreads(8);                        // 0 = runtime default
lj.setTailCorrection(true);                 // lesson 8; off by default
```

**Note the config.** `data/lj_bulk_simulation.yaml` uses `num_particles: 125`,
where threading is a net *loss* (0.052 ms parallel vs 0.045 ms serial — thread
creation costs more than the work). Raise it to 4000+ before drawing any
conclusion about speed.

---

## Where things live

| Path | What |
|---|---|
| `docs/learn-parallel/` | the nine lessons |
| `src/engine/potentials/lj_kernels.{h,cpp}` | the real kernel and its five backends |
| `src/engine/potentials/lj_potential.cpp` | marshalling, dispatch, and the preserved original |
| `benchmarks/bench_lj.cpp` | the measurement harness |
| `tests/unit/test_lj_kernels.cpp` | correctness contract for every backend |
| `exercises/` | your work |
| `project/` | the capstone |
