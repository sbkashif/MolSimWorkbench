# Capstone Report

> Fill this in from `./build/project/capstone_report 4096`. `ctest` checks that
> your code is correct and fast; this file is where you show you understand why.

## 1. Results

| backend | ms/call | threads | speedup vs baseline |
|---|---:|---:|---:|
| Triangular | | 1 | |
| Gather | | 1 | |
| Parallel | | 8 | |
| ParallelDeterministic | | 8 | |

**Baseline chosen:** _____________  **Why:**

_(Lesson 7: the honest denominator is the fastest correct serial code. Say what
number you would have got by dividing by the reference instead, and why that
would have been misleading.)_

## 2. Strong scaling

| threads | ms | speedup | efficiency |
|---:|---:|---:|---:|
| 1 | | | |
| 2 | | | |
| 4 | | | |
| 5 | | | |
| 8 | | | |

**Shape of the curve:**

_(Is there a knee? At what thread count? What does that number correspond to on
your machine? Did changing the schedule move it?)_

## 3. Amdahl

Speedup at 4 threads: ______  →  implied serial fraction s = ______
→ maximum possible speedup 1/s = ______

**Is Amdahl your limiting factor?** If not, what is?

## 4. Formulation

**`Parallel` uses:** ☐ gather  ☐ per-thread private buffers  ☐ atomics  ☐ other

**Why:**

**What it costs:**

_(Gather does 2× the arithmetic. Private buffers cost nthreads×N×3 doubles and
do not port to a GPU. Whatever you chose, name the price.)_

## 5. Something that surprised you

_(One measurement that contradicted what you expected before running it. This
happened three times while the course was being written: the `std::map` lookup
was worth 10% rather than the predicted 2.5×, `std::pow(x,6)` turned out to be
already optimised by the compiler, and structure-of-arrays came out 13% *slower*
on CPU.)_
