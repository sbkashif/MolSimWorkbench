# Capstone: Your Own Parallel LJ Backend

## Overview

Write a complete, production-quality parallel Lennard-Jones force kernel from
scratch, plugged into the repo's real `LJKernelView` seam, and defend it with
measurements.

Everything in the nine lessons feeds into this: the serial rewrite that makes
threading possible, the formulation choice that makes it safe, the scheduling
that makes it scale, the discipline that makes the speedup number honest, and
the judgement to leave the O(1) physics out of the hot loop.

You are not re-deriving the LJ potential. You are answering: **given this
kernel, how do you make it fast, correct, and reproducible on 8 cores — and how
do you prove all three?**

## Skills Applied

| Lesson | What it contributes |
|---|---|
| 1 | the harness, and the discipline of measuring before changing |
| 2 | flat tables, r² arithmetic, hoisted reciprocals |
| 3 | scatter vs gather, and pricing the choice |
| 4 | partitioning without dropping work, fixed-order reduction |
| 5 | why your write pattern is the whole safety argument |
| 6 | scheduling under heterogeneous cores |
| 7 | choosing a baseline you can defend |
| 8 | recognising the work that must *not* be parallelized |

## Specification

Implement `project/src/student_kernel.cpp`:

```cpp
double studentCompute(const LJKernelView& view, StudentBackend backend, int numThreads);
```

### Functional requirements

1. **Four backends** — `Triangular` (best serial), `Gather` (serial),
   `Parallel` (your best parallel effort), `ParallelDeterministic`.
2. **Correctness.** Forces must match `LJPotential::calculateForcesReference`
   to better than 1e-9 relative, for every backend.
3. **Energy.** Total potential energy, each pair counted once, matching the
   reference to 1e-9 relative.
4. **Thread independence.** `Parallel` must give the same forces (to 1e-10) for
   1, 2, 3, 5, 8 and 16 threads. If it does not, you have a race.
5. **Determinism.** `ParallelDeterministic` must be **bitwise** identical across
   repeated runs at 8 threads. `Parallel` need not be — but must still be correct.
6. **No races.** Verified by requirements 4 and 5, and by running under TSan.
7. **Edge cases.** n = 0, n = 1, a non-periodic axis (box length 0), and an atom
   whose type has no parameters, must all behave — no crash, no NaN.
8. **Physics invariant.** Σ F⃗ = 0 across the periodic system for every backend.
9. **Performance.** `Parallel` at 8 threads must be at least **2× faster than
   your own `Triangular`** at N = 4096. (The repo's shipped kernel manages 4.03×;
   2× is the passing bar.)
10. **Tail correction.** Implement `studentTailEnergy`. It must not change any
    force — asserted bitwise.

### Interface

```bash
cmake -B build -S . -DMOLSIM_BUILD_LEARN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && ctest -L learn -R Capstone --output-on-failure
./build/project/capstone_report 4096        # your own measurements
```

## Architecture Guidance

Suggested, not prescribed:

```
project/src/
  student_kernel.h      the interface (given -- do not change the signatures)
  student_kernel.cpp    your implementation
```

A shape that works:

- one `computeAtomRange(view, i0, i1)` doing the gather body, shared by every
  parallel backend, so all of them run identical arithmetic and any difference
  is a parallelism bug rather than an arithmetic one
- `Parallel` = OpenMP over that, `schedule` chosen by measurement
- `ParallelDeterministic` = same, with a hand-rolled fixed-order reduction

If you want the *fastest* kernel rather than the most portable one, revisit
lesson 5: per-thread private buffers over the triangular loop beat gather by
~1.9× on this CPU. That is a legitimate answer for `Parallel`, and it is worth
being able to say why it would not be the right answer on a GPU.

## The written part

`ctest` cannot grade judgement. Fill in `project/REPORT.md`:

1. Your measured table: every backend, ms/call at N=4096, speedup **against the
   baseline you chose**, and a sentence justifying that baseline.
2. A strong-scaling study, 1–8 threads, with efficiency. Explain the shape of
   the curve. If it has a knee, say what causes it.
3. The serial fraction implied by Amdahl at 4 threads. Is Amdahl your limit? If
   not, what is?
4. Which formulation you chose for `Parallel` and why — including what it costs.
5. One thing you measured that contradicted what you expected.

Item 5 is not filler. It happened three times while this course was being
written: the `std::map` lookup was worth 10% rather than the predicted 2.5×,
`std::pow` was already optimised by the compiler, and SoA turned out to be
*slower* on CPU.

## Stretch Goals

- Add a `Vectorized` backend and see whether you can beat the compiler's
  auto-vectorization.
- Wire the pressure virial through the kernel and validate against `P_tail`.
- Make the marshalling in `LJPotential::buildView` incremental — positions
  change every step, but types and tables do not.
- Profile with Instruments (macOS) and find out what fraction of cycles is stalled
  on memory rather than arithmetic.
