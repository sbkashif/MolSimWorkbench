# Exercise 2: The Refactor That Is Also the Port

## Goal

Rewrite the pair loop so it contains no `std::map`, no `std::string`, no
`std::pow` and no `sqrt` — and prove it still gives the reference answer.

## Context

Practises [lesson 2](../../docs/learn-parallel/02_serial_refactor.md). The
result is the kernel every later exercise builds on.

## Your Task

In `src/refactor.cpp`:

1. `buildMixingTable(epsilons, sigmas, rule)` — given per-type ε and σ, produce
   flat `ntypes × ntypes` tables of **4ε_ab** and **σ_ab²**, row-major, applying
   the mixing rule once. `"geometric"` → `sqrt(a*b)`; `"arithmetic"` → `(a+b)/2`.
2. `computeTriangularFast(...)` — the i<j loop, with:
   - reciprocal box lengths hoisted out (multiply, do not divide, in the loop)
   - the cutoff tested in **r²**, never taking a square root
   - `(σ²/r²)³` by multiplication, not `std::pow`
   - the force as `(F/r) · r⃗`, with no normalised direction vector
   - accumulation into the flat `fx/fy/fz` arrays (zero them first)
   - return the total potential energy

Remember the no-periodicity case: when a box length is 0, that axis has no
minimum-image wrapping. Handle it **without a branch in the inner loop**.

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex02` passes
- [ ] Forces match the reference to better than 1e-9 relative
- [ ] No `std::pow`, `std::sqrt`, `std::map` or `std::string` in the inner loop

## Constraints

- Do not modify `tests/`
