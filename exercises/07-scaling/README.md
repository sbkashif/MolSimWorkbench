# Exercise 7: Honest Numbers

## Goal

Implement the arithmetic of scaling analysis, then run a real strong-scaling
study on the repo's kernels and interpret the curve.

## Context

Practises [lesson 7](../../docs/learn-parallel/07_honest_speedup.md).

## Your Task

In `src/scaling.cpp`:

1. `amdahlSpeedup(s, n)` = `1/(s + (1-s)/n)`.
2. `amdahlSerialFraction(speedup, n)` — invert it: given a measured speedup on
   `n` processors, recover `s`. Clamp the result to `[0,1]`.
3. `maxSpeedup(s)` = `1/s`, or infinity when `s == 0`.
4. `efficiency(t1, tn, n)` = `(t1/tn)/n`.
5. `chooseBaseline(candidates)` — given labelled timings, return the **smallest
   serial** one. The honest denominator is the fastest correct single-threaded
   implementation, not the original code and not the parallel code on 1 thread.

Then run the study and read the curve:

```bash
./build/exercises/ex07_scaling
```

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex07` passes
- [ ] `amdahlSerialFraction` inverts `amdahlSpeedup` to 1e-12
- [ ] `chooseBaseline` ignores parallel entries entirely

## Constraints

- Do not modify `tests/`
