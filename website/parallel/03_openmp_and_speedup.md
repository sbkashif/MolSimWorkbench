# OpenMP and honest speedup

[Threads and races](02_threads_and_races.md) split the gather loop across
threads by hand: compute a block per thread, start the threads, wait for
them. OpenMP does the same with one line placed above the loop. This chapter
reads that line in the engine, then measures how much faster the threaded
loop is, compared fairly.

## One line of OpenMP

**OpenMP** is a set of instructions to the compiler, written as lines that
start with `#pragma omp`, that turn ordinary loops into parallel ones. The
engine's `OpenMP` backend, in `ljComputeGather`, is the whole gather loop
plus one such line:

```{literalinclude} ../../src/engine/potentials/lj_kernels.cpp
:language: cpp
:start-at: "#pragma omp parallel for schedule(guided)"
:end-at: "doubleCounted = acc;"
:caption: ljComputeGather, the OpenMP backend (src/engine/potentials/lj_kernels.cpp)
```

Read the line one part at a time:

- **`parallel for`**: start a team of threads, and divide the iterations of
  the loop below among them. The OpenMP specification says the loop's
  iterations "will be executed in parallel by threads in the team", and the
  threads wait for each other at the end of the loop
  ([OpenMP 5.0, worksharing-loop construct](https://www.openmp.org/spec-html/5.0/openmpsu41.html)).
  This is the block split and the `join` from the last chapter, done for you.
- **`schedule(guided)`**: how the iterations are handed out. With `static`,
  each thread gets an equal share up front. With `guided`, threads take
  chunks as they finish the previous one, and the chunks shrink: their size
  is "proportional to the number of unassigned iterations divided by the
  number of threads" (same page). A thread on a slow core then simply takes
  fewer chunks.
- **`reduction(+:acc)`**: every thread adds the energy into its own private
  copy of `acc`, and the copies are added together at the end
  ([OpenMP 5.0, reduction clause](https://www.openmp.org/spec-html/5.0/openmpsu107.html)).
  It is the `partial[t]` slots of the last chapter, done for you.
- **`num_threads(numThreads)`**: how many threads to start.

Each iteration calls `computeRange` for one atom, the gather kernel, so every
force still has one writer.

### Not on this machine

A compiler needs an OpenMP runtime library to run these lines. Apple's clang
does not include one, and the build looks for Homebrew's `libomp`, which is
not installed on the machine used here. The build then compiles without
OpenMP, and the `OpenMP` backend runs the plain serial loop instead: in
`ljComputeGather` the lines above sit inside `#ifdef _OPENMP`, and the
`#else` branch after them calls `computeRange` once for all atoms. The engine's benchmark says so:

```console
$ ./build/benchmarks/bench_lj 4096 5
...
OpenMP: no
```

The measurements below therefore use the `Threads` backend: the same gather
kernel, the same split into blocks, with the threads started by hand. That is
exactly the work `parallel for` with `schedule(static)` would do.

## Honest speedup

A **speedup** is a time before divided by a time after. It is only honest if
"before" is the fastest serial code available. Here that is
`serial-triangular`, the scatter kernel, which does half the arithmetic of
gather. Dividing by `serial-gather` instead would credit the threads for the
work that gather wastes.

At N = 4096, measured with `thread_sweep.cpp` (the faster of two runs):

| Threads | Time per call | Speedup over serial-triangular | Over serial-gather (flattering) |
| --- | --- | --- | --- |
| serial-triangular | 9.89 ms | 1.00 | |
| 1 | 17.80 ms | 0.56 | 1.00 |
| 2 | 9.14 ms | 1.08 | 1.95 |
| 4 | 4.97 ms | 1.99 | 3.58 |
| 6 | 5.15 ms | 1.92 | 3.46 |
| 8 | 4.00 ms | 2.47 | 4.45 |
| 10 | 3.91 ms | 2.53 | 4.56 |

Ten threads make the force step 2.5 times faster than the best serial
kernel, not the 4.6 times that dividing by the slower kernel would claim.
{numref}`par-speedup-figure` shows the same for every N measured.

```{figure} ../figures/par_speedup.svg
:name: par-speedup-figure
:width: 600px

Speedup of the threaded gather kernel over `serial-triangular`, against the
number of threads, for five system sizes. Below the dashed line, the threaded
version is slower than serial.
```

Three things stand out.

- **One thread is slower than serial.** The threaded kernel is gather, so on
  one thread it pays the doubled arithmetic in full: 0.56 at N = 4096. Two
  threads only win it back.
- **Going from 4 to 6 threads does not help.** This machine has 4 performance
  cores. The `Threads` backend gives every thread an equal block of atoms, so
  the call lasts as long as the slowest thread. With more than 4 threads, some
  blocks must run on the slower efficiency cores, and the fast threads finish
  and wait. More threads still help a little (8 and 10), because each block
  gets smaller. The `OpenMP` backend's `schedule(guided)` is meant for exactly
  this: a slow core takes fewer chunks.
- **Small systems get slower.** At N = 125 every thread count is slower than
  serial, down to 0.31 at 10 threads. The whole serial force step takes 21 µs
  there, and just starting and finishing threads that do nothing costs about
  as much:

```console
$ ./thread_cost
 1 threads:    8.5 microseconds to start and join
 2 threads:   13.0 microseconds to start and join
 4 threads:   20.0 microseconds to start and join
 8 threads:   36.6 microseconds to start and join
10 threads:   49.0 microseconds to start and join
```

The `Threads` backend starts its threads anew on every call, so at small N
the threads cost more than the work they share. OpenMP implementations
usually keep their threads waiting between loops instead of starting new
ones, which makes this overhead smaller; it is not measured here.

## Tests that guard the threads

A threaded kernel can be fast and wrong, as the race showed. The engine's
unit tests check the threaded backends against the serial one on every
build:

```console
$ ctest -R LJKernels
...
3/7 Test #12: LJKernels.ResultIsIndependentOfThreadCount ........   Passed    0.01 sec
4/7 Test #13: LJKernels.RepeatedRunsAreDeterministic ............   Passed    0.00 sec
5/7 Test #14: LJKernels.NetForceIsZero ..........................   Passed    0.00 sec
...
100% tests passed, 0 tests failed out of 7
```

- **`ResultIsIndependentOfThreadCount`** computes the forces with 1, 2, 3, 4,
  5, 8 and 16 threads and requires each result to match the serial one.
- **`RepeatedRunsAreDeterministic`** runs each backend twice and requires
  byte-identical forces. A race usually shows up here first, as an answer
  that changes between runs.
- **`NetForceIsZero`** requires the total force to be zero up to rounding.
  The racy loop of the last chapter fails it on every run.

On this machine the `OpenMP` and `OpenMPPrivate` backends fall back to serial
code, so these tests exercise only the `Threads` backend for real; on a
machine with OpenMP they check all three. The testing module, coming later,
returns to these tests.

## Check yourself

:::{dropdown} 1. A colleague reports a 4.6× speedup at 10 threads. What would you ask?
Faster than what? 4.6× is the speedup over `serial-gather`, which does twice
the arithmetic it needs. Over the fastest serial kernel, `serial-triangular`,
it is 2.5×.
:::

:::{dropdown} 2. Why is one thread of the threaded kernel slower than the serial kernel?
The threaded kernel uses gather, which computes every pair twice. On one
thread nothing pays for that: 17.8 ms against 9.9 ms at N = 4096.
:::

:::{dropdown} 3. Why is threading a bad idea for N = 125 on this machine?
The serial force step takes 21 µs, while starting and joining 8 threads
alone takes 37 µs. The overhead is larger than the work being shared.
:::

:::{dropdown} 4. What does reduction(+:acc) do, and which part of the hand-written backend does it replace?
Each thread adds into its own private copy of `acc`, and the copies are added
together at the end. It replaces the `partial[t]` slots and the final sum over
them.
:::
