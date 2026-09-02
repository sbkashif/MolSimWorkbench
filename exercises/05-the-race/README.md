# Exercise 5: Break It, Then Fix It

## Goal

Write a data race on purpose, observe that it is fast, plausible and wrong, then
fix it with per-thread buffers.

## Context

Practises [lesson 5](../../docs/learn-parallel/05_the_race.md). This is the
exercise that makes the rest stick.

## Your Task

In `src/race.cpp`:

1. `racyScatter(...)` — the triangular kernel with `#pragma omp parallel for` on
   the outer loop and **no protection** on the `f[j]` writes. Yes, on purpose.
2. `privateBufferScatter(...)` — the fix. Each thread gets its own full force
   array, writes only into it, and the buffers are summed afterwards. Keep
   Newton's third law. Use `schedule(static, 16)` — see the hint for why it
   must be `static`.
3. `countWrongAtoms(reference, fx, fy, fz, relTol)` — how many atoms have any
   component differing from the reference by more than `relTol` relative.

Then run the demo and look at what the race actually does:

```bash
./build/exercises/ex05_demo
```

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex05` passes
- [ ] `privateBufferScatter` matches the reference for every thread count
- [ ] `privateBufferScatter` is **bitwise** reproducible across runs
- [ ] `racyScatter` produces wrong results on ≥2 threads (a test asserts this!)

## Constraints

- Do not modify `tests/`
- `racyScatter` must stay broken. It is a specimen.
