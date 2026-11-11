# Exercise 1: Build a Benchmark You Can Trust

## Goal

Implement a timing helper with the properties lesson 1 argued for: a warm-up
call, repeated measurement, and minimum-not-mean.

## Context

Practises [lesson 1](../../docs/learn-parallel/01_measure_first.md). Every later
exercise measures something; if this one is wrong, all of them lie.

## Your Task

In `src/timing.cpp`:

1. Implement `timeMs(f, reps)`:
   - call `f()` **once, untimed**, as a warm-up
   - then call it `reps` more times, timing each individually
   - return the **minimum** elapsed time in milliseconds
   - if `reps < 1`, treat it as 1
2. Implement `speedup(baselineMs, candidateMs)` returning `baselineMs/candidateMs`,
   and `0.0` if `candidateMs <= 0`.
3. Implement `parallelEfficiency(t1, tN, n)` = `(t1/tN)/n`, and `0.0` if
   `tN <= 0` or `n < 1`.

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex01` passes
- [ ] `f` is invoked exactly `reps + 1` times
- [ ] The value returned is the minimum, not the mean or the last

## Constraints

- Do not modify anything in `tests/`
