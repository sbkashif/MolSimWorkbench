# Exercise 6: OpenMP, and the Scheduling Experiment

## Goal

Replace exercise 4's hand-rolled pool with pragmas, then **reproduce the
scheduling measurement** rather than taking lesson 6's word for it.

## Context

Practises [lesson 6](../../docs/learn-parallel/06_openmp.md).

## Your Task

In `src/omp_kernel.cpp`:

1. `ompGather(..., Schedule s)` — the gather kernel under
   `#pragma omp parallel for`, with `reduction(+:)` for the energy and the
   schedule selected by `s` (`Static`, `StaticChunk`, `Dynamic`, `Guided`).
   Use `std::ptrdiff_t` for the loop index.
2. `deterministicGather(...)` — same kernel, but the energy reduction done by
   hand into a per-thread array summed in index order, so the result is bitwise
   reproducible. (`reduction(+:)` is not.)

Then run the scheduling study:

```bash
./build/exercises/ex06_schedule
```

and answer, in a comment at the bottom of `src/omp_kernel.cpp`:

- At which thread count does `Static` stop scaling, and what does that number
  correspond to on your machine?
- Does `Guided` fix it?

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex06` passes
- [ ] All four schedules give the same forces
- [ ] `deterministicGather` is bitwise identical across runs; the test proves it
- [ ] You have written your answer in the comment block

## Constraints

- Do not modify `tests/`
