# Exercise 8: The O(1) Term

## Goal

Implement the analytic dispersion correction, prove it contributes zero force,
and decide — with a measurement — not to parallelize it.

## Context

Practises [lesson 8](../../docs/learn-parallel/08_not_the_hot_loop.md). The one
piece of *physics* this course adds.

## Your Task

In `src/tail.cpp`:

1. `tailEnergy(n, volume, epsilon, sigma, cutoff)`:

   E_tail = (8/3)·π·N·ρ·ε·σ³·[ (1/3)(σ/r_c)⁹ − (σ/r_c)³ ],  ρ = N/V

2. `tailPressure(n, volume, epsilon, sigma, cutoff)`:

   P_tail = (16/3)·π·ρ²·ε·σ³·[ (2/3)(σ/r_c)⁹ − (σ/r_c)³ ]

3. `tailForceMagnitude(...)` — think before you write. Return the correct value.

Both energy functions must return 0 for a non-physical input (`volume <= 0`,
`cutoff <= 0`, `n <= 0`).

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex08` passes
- [ ] `tailEnergy` is negative for a normal LJ fluid
- [ ] `tailPressure` does **not** depend on N at fixed density
- [ ] `tailForceMagnitude` returns exactly zero, and you can say why

## Constraints

- Do not modify `tests/`
- Do not add anything to a pair loop. There is no pair loop in this exercise.
