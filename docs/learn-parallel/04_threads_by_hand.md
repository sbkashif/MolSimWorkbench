# Lesson 4: Threads by Hand

## Learning Objectives

- Create, partition work for, and join `std::thread`s without a framework
- Split an index range so no thread is more than one item behind
- Give each thread private output and avoid false sharing
- Name each piece of machinery you build, because CUDA makes you build it again

## Background

You will not write this code again after this lesson — OpenMP does it in one
line, and lesson 6 replaces it. The point is to build the machinery once, by
hand, so that the pragma later is a *shorthand you understand* rather than a
magic word.

Everything in this lesson has a direct CUDA analogue, listed at the end.

## Core Concepts

### A thread is a callable plus a stack

```cpp
#include <thread>

std::thread t([]{ std::puts("hello from another thread"); });
t.join();       // block until it finishes
```

`std::thread` starts running immediately on construction. `join()` waits. If a
`std::thread` object is destroyed without `join()` or `detach()`, the program
calls `std::terminate` — there is no implicit wait.

Threads are not free. Creation costs on the order of 10–100 µs. That is why
lesson 1's N=125 case ran *slower* in parallel: 0.045 ms of work cannot absorb
eight thread creations.

### Partitioning: the balanced split

The obvious split has a bug:

```cpp
size_t chunk = n / T;
size_t i0 = t * chunk;
size_t i1 = i0 + chunk;      // WRONG: drops n % T atoms off the end
```

With n=4096 and T=7, `chunk=585`, and 7×585 = 4095. One atom silently never gets
a force. A one-atom error in an MD simulation is not going to announce itself.

The fix distributes the remainder, one extra item to each of the first `n % T`
threads:

```cpp
const size_t chunk = n / T;
const size_t rem   = n % T;
const size_t i0 = t * chunk + std::min<size_t>(t, rem);
const size_t i1 = i0 + chunk + (t < rem ? 1 : 0);
```

Now every atom is covered exactly once and no thread has more than one extra
item. This is precisely what OpenMP's `schedule(static)` computes for you.

### Private output, and false sharing

Each thread needs somewhere to put its partial energy. The obvious approach:

```cpp
std::vector<double> partial(T, 0.0);
// thread t writes partial[t]
```

This is correct — each thread writes a distinct element — but it has a
performance trap called **false sharing**. `partial` is 8 bytes per entry, and a
cache line is 64 bytes, so all 8 threads' slots sit on **one cache line**. Every
write by any thread invalidates that line in all other cores' caches. The
hardware is doing coherence traffic for data that is not actually shared.

Two ways out:

1. Accumulate in a local variable and write `partial[t]` **once at the end** —
   which is what the shipped kernel does. One write per thread cannot thrash.
2. Pad each entry to a full cache line (`alignas(64)`).

Option 1 is better: it removes the problem instead of hiding it. The same
instinct applies inside the kernel, where `fxi`/`fyi`/`fzi` accumulate in
registers and `v.fx[i]` is written once.

### Putting it together

From `src/engine/potentials/lj_kernels.cpp`:

```cpp
std::vector<double> partial(T, 0.0);
std::vector<std::thread> pool;
pool.reserve(T);

for (int t = 0; t < T; ++t) {
    const size_t chunk = n / T, rem = n % T, ut = t;
    const size_t i0 = ut*chunk + std::min(ut, rem);
    const size_t i1 = i0 + chunk + (ut < rem ? 1u : 0u);

    pool.emplace_back([&v, i0, i1, t, &partial]() {
        partial[t] = computeRange(v, i0, i1);
    });
}
for (auto& th : pool) th.join();

for (double p : partial) doubleCounted += p;   // fixed order — see below
```

Three details worth pausing on:

**Capture by value where it matters.** `i0`, `i1` and `t` are captured *by
value*. Capturing the loop variable by reference is a classic bug — by the time
the thread runs, the loop has moved on and every thread sees the same final
value. `v` and `partial` are captured by reference deliberately, and both
outlive the join.

**`reserve` before `emplace_back`.** Without it the vector reallocates mid-loop.
`std::thread` is movable so this is legal, but reserving avoids the churn while
threads are being spawned.

**The reduction is summed in thread order, not completion order.** Floating-point
addition is not associative: (a+b)+c ≠ a+(b+c) in the last bits. Summing
`partial[]` by index gives the same answer every run. Summing as threads finish
would make the total energy jitter run to run — reproducible science requires
the fixed order. This is a real difference from OpenMP's `reduction(+:)`, which
does *not* guarantee order (lesson 6).

### Every piece has a CUDA counterpart

| Built here by hand | CUDA equivalent |
|---|---|
| choosing `T` | grid and block dimensions |
| `i0`/`i1` range per thread | `i = blockIdx.x*blockDim.x + threadIdx.x` |
| `pool.emplace_back(...)` | the `<<<grid, block>>>` launch |
| `th.join()` | `cudaDeviceSynchronize()` (launches are async) |
| `partial[t]`, summed after | block-level reduction in `__shared__` memory |
| local `fxi`, written once | thread registers |
| false sharing | uncoalesced access / shared-memory bank conflicts |

The concepts do not change on a GPU. Only the scale does — 8 threads becomes
100,000, which is why the *manual* version stops being viable and why the
reduction becomes the hard part rather than a `for` loop.

## Common Pitfalls

- **Forgetting to `join()`.** `std::terminate`, not a warning.
- **Capturing the loop counter by reference.** Every thread races to read a
  variable that is being mutated by the spawning loop.
- **Spawning threads inside the timestep loop.** 5000 steps × 8 threads = 40,000
  thread creations. A real thread pool is created once and reused; this kernel
  gets away with it only because the work per call is large. OpenMP maintains a
  persistent pool for exactly this reason.
- **`hardware_concurrency()` as gospel.** It returns 8 on a 4P+4E machine and
  tells you nothing about the cores being different speeds — see lesson 7.

## Summary

- `std::thread` runs on construction; you must `join`.
- Distribute the remainder when partitioning, or silently drop work.
- Accumulate locally and write once: correctness *and* no false sharing.
- Sum reductions in a fixed order for run-to-run reproducibility.
- Every mechanism here reappears in CUDA under a different name.

## Further Reading

- Exercise: `exercises/04-threads-by-hand/`

**Next:** [05_the_race.md](05_the_race.md)
