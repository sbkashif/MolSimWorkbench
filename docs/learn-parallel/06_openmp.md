# Lesson 6: OpenMP

> ### In plain English
>
> Chapter 4 built the worker management by hand — deciding how many workers,
> dividing the atoms between them, starting them, waiting for them, adding up
> their answers. About 25 lines.
>
> OpenMP does all of that if you write one line above the loop. The reason we
> did it by hand first is so that line is something you understand rather than
> something you copy.
>
> The interesting part of this chapter is what happens when your cores are not
> all the same speed. Your Mac has 4 fast cores and 4 slow ones, and the
> straightforward way of dividing up the work makes a **5-worker run slower
> than a 4-worker run**. One line fixes it.
>
> New words here: **pragma**, **schedule**, **reduction**, **shared/private**,
> **barrier** — see [GLOSSARY.md](GLOSSARY.md).

## Learning Objectives

- Replace the hand-rolled pool from lesson 4 with two lines
- Use `shared`, `private`, `reduction` and `schedule` deliberately
- Diagnose load imbalance, including the case where adding a thread is slower
- Map every clause to its CUDA counterpart

## Background

Lesson 4 built a thread pool by hand: partitioning, lambdas, `join`, a partial
array, a fixed-order reduction. About 25 lines, and every line was a chance to
get the remainder handling wrong.

OpenMP is a set of compiler pragmas that generate all of it. The code becomes:

```cpp
#pragma omp parallel for schedule(guided) reduction(+:acc) num_threads(T)
for (std::ptrdiff_t i = 0; i < n; ++i)
    acc += computeRange(v, i, i + 1);
```

The value of having written lesson 4 first is that this line is now *readable*:
`parallel for` is the pool and the partition, `reduction` is the `partial[]`
array and its final sum, and the closing brace is the `join`.

Your build already has it — libomp 19.1.7, OpenMP 5.1, found automatically by
CMake. If it is missing the code still compiles and falls back to serial.

## Core Concepts

### `parallel for`

```cpp
#pragma omp parallel for
for (int i = 0; i < n; ++i) { /* body */ }
```

Splits `[0, n)` across the team, runs the body, and waits at an implicit barrier.
The loop variable is automatically private — each thread gets its own `i`.

The loop must be a *canonical* form: a signed integer counter, a bound that does
not change during the loop, and no `break`. `continue` is fine (it just ends
that iteration). This is why the kernel uses `std::ptrdiff_t` rather than
`std::size_t` — an unsigned counter is only permitted from OpenMP 3.0 and is
still a portability wart.

### `shared` and `private`

Every variable in scope is one or the other:

- **`shared`** (default for anything declared outside the loop) — one copy, all
  threads see it. Reads are fine. Concurrent writes are lesson 5.
- **`private`** (default for anything declared inside) — each thread gets its own
  uninitialised copy.
- **`firstprivate`** — private, but initialised from the value outside.

The kernel relies on this without saying so explicitly:

```cpp
for (std::ptrdiff_t i = 0; i < n; ++i) {
    double fxi = 0.0, fyi = 0.0, fzi = 0.0;   // declared inside => private
    ...
    v.fx[i] = fxi;                            // shared array, but i is unique
}
```

`v` is shared and read-only. `v.fx` is shared and written — but each thread
writes disjoint indices, which is the whole gather argument from lesson 3.

> **The CUDA mapping:** `shared` is global device memory; `private` is a thread's
> registers. Declaring accumulators inside the loop is exactly why the CUDA port
> needs no changes — they are already per-thread.

### `reduction`

```cpp
#pragma omp parallel for reduction(+:acc)
for (...) acc += something;
```

Each thread gets a private zero-initialised `acc`, and OpenMP sums them at the
end. This replaces the entire `partial[]` array from lesson 4.

**It is not deterministic.** The order in which per-thread partials are combined
is unspecified and can differ between runs. Floating-point addition is not
associative, so the total energy can change in its last bits run to run. The
hand-rolled backend sums `partial[]` in thread-index order and therefore *is*
reproducible.

If bitwise reproducibility matters — regression tests, published results,
debugging a divergence — do the reduction yourself. This is the same trap as the
`schedule(dynamic)` issue in lesson 5, arriving from a different direction.

> **The CUDA mapping:** `reduction` is a block-level tree reduction in
> `__shared__` memory followed by an atomic or a second kernel pass. It is one
> word here and roughly 30 lines there. It is the single biggest reason the GPU
> port is more than a copy-paste.

### `schedule` — and a result that should surprise you

`schedule` decides how iterations are handed out:

- **`static`** — contiguous equal blocks, decided up front. Zero runtime
  overhead. Correct when every iteration costs the same.
- **`static, C`** — round-robin chunks of size `C`. Still decided up front (so
  still reproducible), but interleaved, which balances triangular loops.
- **`dynamic, C`** — threads grab chunks of `C` on demand. Handles uneven work;
  costs a synchronised counter per chunk.
- **`guided`** — like dynamic, but chunks start large and shrink. Most of the
  balancing benefit, far fewer handoffs.

Here is the gather kernel at N=4096 on this 8-core machine:

| threads | `static` | `dynamic,32` | `guided` |
|---:|---:|---:|---:|
| 1 | 28.37 | 27.98 | 28.06 |
| 2 | 14.61 | 14.68 | 14.59 |
| 4 | 7.62 | 7.58 | 7.54 |
| **5** | **9.43** | **7.05** | **7.06** |
| 6 | 8.41 | 6.61 | 6.60 |
| 8 | 6.53 | 5.86 | 5.84 |

**With `schedule(static)`, going from 4 threads to 5 makes the program slower**
— 7.62 ms to 9.43 ms. Adding a core cost 24% performance.

The cause is not OpenMP. This is an Apple Silicon chip with **4 performance
cores and 4 efficiency cores**. `schedule(static)` gives all five threads an
equal 1/5 of the atoms, but the thread that lands on an efficiency core runs at
roughly half speed. The other four finish and sit at the barrier waiting for it.
**A barrier runs at the speed of the slowest participant.**

`guided` fixes it completely (7.06 ms at 5 threads, faster than 4). Slow cores
simply request fewer chunks. That is why the shipped kernel uses `guided`, and
why the comment in `lj_kernels.cpp` says so with the numbers attached.

The general lesson is not "always use guided" — `static` is free and `guided`
is not, and on homogeneous cores with uniform work `static` wins. The lesson is
that **your iterations are not equal cost and your cores are not equal speed**,
and only measurement tells you when that starts to matter.

> **The CUDA mapping:** the GPU's warp scheduler does `dynamic` in hardware,
> switching warps whenever one stalls. The analogous problem there is *warp
> divergence* — threads in a warp taking different branches — and *tail effect*,
> a final block that leaves most of the GPU idle.

### The clause-to-CUDA table

| OpenMP | CUDA | Concept |
|---|---|---|
| `parallel for` | `kernel<<<grid, block>>>` | launch a team |
| loop index `i` | `blockIdx.x*blockDim.x + threadIdx.x` | who am I |
| `num_threads(T)` | grid × block dimensions | how many |
| `private` / declared-inside | registers | per-thread state |
| `shared` | global device memory | shared state |
| `reduction(+:x)` | `__shared__` tree reduction + atomic | combining |
| implicit barrier at `}` | `__syncthreads()` / `cudaDeviceSynchronize()` | waiting |
| `schedule(guided)` | hardware warp scheduler | load balancing |
| false sharing | uncoalesced access, bank conflicts | memory contention |
| `omp_get_thread_num()` | `threadIdx.x` | identity |

Learning OpenMP properly is most of the conceptual work of learning CUDA. What
remains is the memory hierarchy, which has no CPU analogue you have had to
manage by hand.

## Common Pitfalls

- **Forgetting `reduction` and just writing `acc += ...`.** That is lesson 5's
  race with nicer syntax. It compiles silently.
- **Assuming `reduction` is deterministic.** It is not.
- **`#pragma omp parallel` and `#pragma omp for` confused.** `parallel` alone
  runs the *whole block* on every thread — n× the work, not 1/n.
- **Parallelising the inner loop.** You would pay a fork/join per atom. Always
  parallelise the outermost loop with enough iterations.
- **Believing 8 threads means 8× on a laptop.** See lesson 7.

## Summary

- `parallel for` + `reduction` replaces lesson 4's entire hand-rolled pool.
- Variables declared inside the loop are private; that is why the kernel ports.
- `reduction(+:)` is not reproducible; hand-roll it if you need bitwise stability.
- `schedule(static)` was *slower on 5 threads than on 4* on a 4P+4E machine.
  `guided` fixed it. Measure; do not assume your cores are identical.

## Further Reading

- [OpenMP 5.1 specification](https://www.openmp.org/spec-html/5.1/openmp.html)
- Exercise: `exercises/06-openmp/` — reproduce the scheduling table yourself

**Next:** [07_honest_speedup.md](07_honest_speedup.md)
