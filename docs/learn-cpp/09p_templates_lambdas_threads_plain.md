# Chapter 9, plain version — recipes with blanks, sticky notes, and more robots

This is a plain-language retelling of [09_templates_lambdas_threads.md](09_templates_lambdas_threads.md).
Same sections, same order, one picture: the robot kitchen from chapter 1. Each section is a step table, **Kitchen | C++ |
Example**. For exact line numbers and full program output, open the technical version. Its one
(deep dive later) section, on what `F&&` means, is left out here. Timings are from this M1 (4 fast and 4
slow cores).

## The kitchen, extended

| Kitchen | C++ |
|---|---|
| a **recipe with a blank**: "___ cake"; fill in "carrot" and you get a real carrot-cake recipe | a template, `template <typename T>` |
| a **sticky-note instruction** written on the spot and handed over (to the stopwatch, or to a robot) | a lambda, `[…](…) { … }` |
| what the sticky note may take from the counter: a **copy**, or **the original** jar | capture by value `[t]` / by reference `[&t]` |
| a **stopwatch** used on the second attempt onwards, keeping the best time | `timeMs` |
| **one more robot**, working at the same time | a `std::thread` |
| **waiting for that robot** to finish before clearing the counter | `join()` |
| each robot writes its result on **its own slip** | `partial[t]` |

---

## Where we are

At the end of chapter 8 the machines were built in a sub-recipe and kept safely in the storeroom, with
sign-out sheets, the same way the repo does it. Nobody has timed how long "every station:
calculateEnergy" takes, and one robot is doing all of it while the kitchen has room for eight. This
chapter brings a stopwatch and more robots. Both need a way to hand over **a piece of work**.

---

## 9.0 The shape of the problem

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | "time this for me" means handing over the job to be timed | pass code to a timer | `timeMs(job, 5)` |
| 2 | "do this for me" means handing a job to another robot | pass code to a thread | `std::thread(job)` |
| 3 | the job is written on the spot, on a sticky note | a lambda (§9.2) | `[&]() { … }` |
| 4 | the stopwatch must accept *any* sticky note, so its own recipe has a blank | a template (§9.1) | `template <typename F>` |

---

## 9.1 Templates: a recipe with a blank

| # | Kitchen | C++ | Example (`template_blank.cpp`) |
|---|---|---|---|
| 1 | write "double the ___" once | `template <typename T> T twice(T x) { return x + x; }` | |
| 2 | the book asks for it with 21: the translator writes a real "double the number" recipe | instantiation for `int` | `42` |
| 3 | with 1.25 | for `double` | `2.5` |
| 4 | with the word "ab" | for `std::string` | `abab` |
| 5 | with a whole shelf of jars: "add a shelf to a shelf" makes no sense; the translator refuses **before any cooking** | compile error | `invalid operands to binary expression ('std::vector<int>' and 'std::vector<int>')` |

The translator fills in the blank and checks the result before the book is made, so "add a shelf to a
shelf" fails at the translator's desk, never in the middle of a run. You have been filling in other
people's blanks for chapters: `std::vector<Vec3>`,
`std::shared_ptr<Potential>`. This is writing a blank of your own.

---

## 9.2 Lambdas: a function written where you need it

| # | Kitchen | C++ | Example (`capture.cpp`) |
|---|---|---|---|
| 1 | write a sticky note that **copies** jar `t` right now | `[t]() { return t; }` | the copy shows 0 |
| 2 | write a sticky note that may use **the original** jar `t` | `[&t]() { return t; }` | |
| 3 | change what is in jar `t` to 5; then read both notes | `t = 5;` | `byValue() = 0, byReference() = 5` |
| 4 | a note that must change your jar has to use the original | `[&calls]() { ++calls; }` | `calls = 2` |

| Written | Kitchen |
|---|---|
| `[t]` | a copy of jar `t`, poured when the note is written |
| `[&t]` | the original jar `t` |
| `[=]` | copies of everything the note mentions |
| `[&]` | the originals of everything the note mentions |

**A note that uses the original reads the jar when the note is used, not when it was written.** That is
why `byReference()` gave 5: jar `t` had been refilled in between. A copy is frozen at the moment the note
is written. C++ makes you choose, jar by jar, on every note.

Every sticky note is a brand-new kind of thing with no name you can write down. That is why the
stopwatch needs a blank (§9.1) to accept one.

---

## 9.3 Timing a piece of code: `timeMs`

| # | Kitchen | C++ (`timing.h`) | Line |
|---|---|---|---|
| 1 | the stopwatch accepts any sticky note | `template <typename F> double timeMs(F&& f, int reps)` | 10–11 |
| 2 | do the job once **without** timing it: the first go always includes fetching and warming things up | `f();` | 12 |
| 3 | write down an impossibly slow "best time" to start | `std::numeric_limits<double>::max()` | 13 |
| 4 | start the stopwatch, do the job, stop it | `steady_clock::now()`, `f()`, `now()` | 15–17 |
| 5 | keep the **fastest** attempt | `std::min(best, …)` | 18 |

Why the warm-up and why the fastest attempt: interruptions only ever slow a robot down, never speed it
up. The full argument, with measurements, is
[learn-parallel 01](../learn-parallel/01_measure_first.md). The stopwatch lives entirely in its summary-card file
page (`timing.h`), because a recipe with a blank must be visible wherever it is filled in.

---

## 9.4 A first thread

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | switch on one more robot and hand it a sticky note; it **starts at once** | `std::thread worker(note);` | |
| 2 | wait until that robot has finished | `worker.join();` | |
| 3 | throw away the handle of a robot without waiting for it: the whole kitchen shuts down | a never-joined `std::thread` destroyed → `std::terminate` | |
| 4 | keep the robots in a row; build each robot's place directly in the row | `std::vector<std::thread> pool;` `pool.emplace_back(note)` | a robot's handle can be moved, not photocopied, like a `unique_ptr` |
| 5 | the robots use the machine `lj` from `main`'s counter, so `main` must wait for them before clearing it | `join()` before `lj` goes out of scope | ch 8's lifetime rule |

Each robot can run on its own processor core, so the robots really do work at the same moment.

---

## 9.5 The capture list decides correctness

The note each extra robot gets (`main.cpp:33-35`):

```cpp
pool.emplace_back([&lj, &geometry, &partial, i0, i1, t]() {
    partial[t] = lj.calculateEnergyRange(geometry, i0, i1);
});
```

| # | Kitchen | C++ | Why |
|---|---|---|---|
| 1 | use the **original** LJ machine | `&lj` | big; everyone can read it; nobody changes it |
| 2 | use the **original** atom list | `&geometry` | 4096 positions; a copy per robot is waste |
| 3 | write on the **original** row of result slips | `&partial` | the results must land on `main`'s slips |
| 4 | take a **copy** of "rows from i0 to i1" | `i0, i1` | those jars are cleared at the end of this loop round, maybe before the robot even starts |
| 5 | take a **copy** of "you are robot number t" | `t` | the loop changes `t` next round |
| 6 | each robot writes only on slip number `t` | `partial[t] = …` | no two robots ever write on the same slip |
| 7 | after all robots are done, add the slips up in order 0, 1, 2, … | `for (double p : partial)` | the same total every time |

### The same loop with `[&]`

| # | Kitchen | C++ | Example (`capture_bug.cpp`) |
|---|---|---|---|
| 1 | give every robot a note that uses the **originals** of `i0`, `i1`, `t` | `[&]` | compiles without a warning |
| 2 | those jars were cleared off the counter at the end of each loop round | the variables' lifetime ended | ch 8's dangling problem |
| 3 | the robots read whatever is there now | undefined behaviour | `E = 0` instead of `-20105.493949`; 3 of 6 launches aborted (`libc++abi: terminating`) |
| 4 | the shelf monitor (ch 8) names the jar | AddressSanitizer | `stack-use-after-scope` … `'i0' (line 25)` |

---

## 9.6 The mini_lj step: time it, then split it

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | one LJ machine on `main`'s counter; no station rack, no tether, no clean-up notes | `LJPotential lj;` in `main` | |
| 2 | the machine can now do "only rows i0 to i1" | `calculateEnergyRange(geometry, i0, i1)` | `calculateEnergy` = rows 0 to N |
| 3 | time one robot doing all of it | `timeMs([&]() { … calculateEnergy … }, 5)` | `serial E = -20105.493949   24.101416 ms` |
| 4 | time two robots, each given half the rows | `timeMs(runThreads, 5)` | `threaded E = -20105.493949   17.411542 ms` |
| 5 | compare | | `speedup = 1.38x` (1.33–1.39 over repeated runs) |
| 6 | four robots | `./out/mini_lj 4096 4` | `2.33x` |
| 7 | the repo's tiny default system | `./out/mini_lj 125 2` | `1.06x`: switching on robots costs about as much as the job |

The energy is the same every time, and the same as chapter 4's.

---

## 9.7 Why 1.38× and not 2×

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | row `i` means "pair atom `i` with every atom after it" | `for j = i + 1 …` | row 0: 4095 pairs; row 4095: none |
| 2 | half the **rows** each is not half the **work** | equal slices of `i` | robot 0: 6,290,432 pairs; robot 1: 2,096,128 |
| 3 | robot 1 finishes early and stands idle | `join()` waits for the slowest | robot 0 alone: 17.13 ms; both together: 17.41 ms |
| 4 | four robots: the first has seven times the last one's pairs | | `2.33x` |

The name for this is **load imbalance**: work split evenly by count, but the items do not cost the same.
This chapter only names it. Learn-parallel [03](../learn-parallel/03_scatter_vs_gather.md) and
[06](../learn-parallel/06_openmp.md) deal with it.

---

## 9.8 Energy only: why we did not split the forces

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | for forces, each pair puts a note on **both** atoms' trays | `forces[i]` and `forces[j]` | `lj_potential.cpp:56` |
| 2 | robot 0, doing row 10, must also write on atom 3000's tray, which belongs to robot 1's rows | | |
| 3 | two robots writing on the same tray at the same moment, with nothing to take turns | a **data race** | |

Energy was safe only because each robot kept a private running total and wrote one slip. What a data race
does, and the ways around it, are learn-parallel [03](../learn-parallel/03_scatter_vs_gather.md) and
[05](../learn-parallel/05_the_race.md).

---

## 9.9 In the real code

| # | Kitchen | repo (`src/engine/potentials/lj_kernels.cpp`) | Line |
|---|---|---|---|
| 1 | one result slip per robot | `std::vector<double> partial(…, 0.0);` | 307 |
| 2 | the row of robots, with all places set out in advance so it never has to move shelves | `pool.reserve(T)` | 309 |
| 3 | work out each robot's rows (a different formula from ours) | `chunk`, `rem`, `i0`, `i1` | 314–318 |
| 4 | the sticky note: originals of the shared things, copies of the per-robot numbers | `[&v, i0, i1, t, &partial]` | 322 |
| 5 | each robot writes its own slip | `partial[t] = computeRange(v, i0, i1);` | 323 |
| 6 | wait for every robot | `th.join()` | 326 |
| 7 | add the slips in a fixed order | `for (double p : partial) …` | 331 |

One real difference: the repo's row recipe pairs atom `i` with **every** other atom, not just later ones
(`lj_kernels.cpp:58-59`), so the triangle from §9.7 is not there. Why, and what it costs, is
learn-parallel 03. And the repo's stopwatch (`benchmarks/bench_lj.cpp:56-68`) is ours with a different
starting "best time" and a different clock.

---

## Next

`mini_lj` has now used every idea the real LJ kitchen is built from. But it is a practice
kitchen in `docs/learn-cpp/examples/`. Chapter 10 walks through the real kitchen room by room, matching
each practice machine to the real one, has the robot cook a real run with it, and checks the numbers agree.
