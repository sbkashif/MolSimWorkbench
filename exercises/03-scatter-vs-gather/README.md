# Exercise 3: The Gather Formulation

## Goal

Rewrite the kernel so every atom owns its own force, then measure what that
costs you.

## Context

Practises [lesson 3](../../docs/learn-parallel/03_scatter_vs_gather.md). This is
the formulation that makes lessons 4–6 possible and that ports to a GPU.

## Your Task

In `src/gather.cpp`:

1. `computeGather(...)` — loop `i` over all atoms, and for each, loop `j` over
   **all** atoms (skipping `j == i`). Accumulate into **local scalars**, write
   `fx[i]`/`fy[i]`/`fz[i]` exactly once at the end of the i iteration.
   Return the total potential energy — remember gather visits every pair twice,
   so halve the accumulated energy.
2. `gatherArithmeticRatio(n)` — return the ratio of pair evaluations that gather
   does versus the triangular loop, as an exact expression in `n`.

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex03` passes
- [ ] `fx[i]` is assigned, not accumulated across the j loop
- [ ] Energy matches the reference (not double, not half)

## Constraints

- Do not modify `tests/`
- Do not write to `fx[j]` anywhere. That is the whole point.
