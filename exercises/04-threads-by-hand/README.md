# Exercise 4: A Thread Pool, By Hand

## Goal

Partition an index range across threads without dropping work, then run the
gather kernel on `std::thread` and get a deterministic answer.

## Context

Practises [lesson 4](../../docs/learn-parallel/04_threads_by_hand.md). You build
here what one OpenMP pragma will replace in exercise 6.

## Your Task

In `src/pool.cpp`:

1. `partitionRange(n, numThreads, t)` → the half-open `[begin, end)` for thread
   `t`. **Distribute the remainder**: the first `n % T` threads get one extra
   item. Every index in `[0,n)` must be covered exactly once.
2. `parallelGather(...)` — spawn `numThreads` `std::thread`s, give each its
   range, run the gather kernel over it, `join`, and combine the per-thread
   energies **in thread-index order** so the result is reproducible.

## Acceptance Criteria

- [ ] `ctest -L learn -R Ex04` passes
- [ ] Partitions tile `[0,n)` exactly, for every `n` and `T` tested
- [ ] Results are identical for 1, 2, 3, 5 and 8 threads
- [ ] Repeated runs are **bitwise** identical

## Constraints

- Do not modify `tests/`
- `std::thread` only. No OpenMP — that is exercise 6.
