# OpenMP and speedup

[Threads and races](02_threads_and_races.md) split the gather loop across
threads by hand: compute a block per thread, start the threads, wait for
them. OpenMP does the same with one line placed above the loop. This chapter
builds the engine with OpenMP, reads that line, adds a second OpenMP kernel
that keeps the cheaper scatter loop, and measures all of them against the
fastest serial kernel. At N = 4096 the best one computes the forces 4.5
times faster than the best serial kernel.

## Building with OpenMP

**OpenMP** is a set of instructions to the compiler, written as lines that
start with `#pragma omp`, that turn ordinary loops into parallel ones. The
compiler needs a flag to read them, and the program needs an OpenMP runtime
library, which starts and manages the threads. GCC on Linux brings both.
Apple's clang reads the lines but comes without the library; Homebrew
provides it:

```console
$ brew install libomp
```

Nothing else changes. The engine's `CMakeLists.txt` asks Homebrew where
`libomp` is (`brew --prefix libomp`), and a fresh configure finds it:

```console
$ cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
...
-- Found OpenMP: TRUE (found version "5.1")
-- OpenMP 5.1 found - parallel LJ backend available (opt-in)
```

The engine's benchmark confirms it, and reports how many threads OpenMP uses
when not told otherwise, one per core:

```console
$ ./build/benchmarks/bench_lj 4096 5
...
OpenMP: yes  (default threads = 10)
```

Without the library the build still succeeds. Every OpenMP line in the
engine sits inside `#ifdef _OPENMP`, a check whether the compiler is reading
OpenMP lines, and the `#else` branch runs the serial loop instead.

## One line of OpenMP

The engine's `OpenMP` backend, in `ljComputeGather`, is the whole gather
loop plus one such line:

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
  each thread gets an equal share up front, as in the last chapter. With
  `guided`, threads take chunks as they finish the previous one, and the
  chunks shrink: their size is "proportional to the number of unassigned
  iterations divided by the number of threads" (same page). A thread on a
  slow core then simply takes fewer chunks.
- **`reduction(+:acc)`**: every thread adds the energy into its own private
  copy of `acc`, and the copies are added together at the end
  ([OpenMP 5.0, reduction clause](https://www.openmp.org/spec-html/5.0/openmpsu107.html)).
  It is the `partial[t]` slots of the last chapter, done for you.
- **`num_threads(numThreads)`**: how many threads to start.

Each iteration calls `computeRange` for one atom, the gather kernel, so every
force still has one writer.

### Why guided, not static

The schedule matters on this machine because its cores are not equal: 4
performance cores and 6 slower efficiency cores. The same loop with
`schedule(static)` in place of `schedule(guided)`, at N = 4096:

| Threads | `static` | `guided` |
| --- | --- | --- |
| 4 | 5.01 ms | 4.97 ms |
| 5 | 6.07 ms | 4.52 ms |
| 6 | 5.13 ms | 4.38 ms |
| 10 | 3.87 ms | 3.17 ms |

With `static`, the fifth thread makes the loop **slower**. Every thread gets
an equal share; the share that lands on an efficiency core finishes last,
and the other four wait for it at the end of the loop. With `guided`, the
slow thread takes fewer chunks and nobody waits long, so every added thread
helps.

## Keeping scatter: private copies

Gather does twice the arithmetic of scatter, so the threads first have to
win back a factor of about two. The `OpenMPPrivate` backend keeps the scatter
loop, which visits each pair once, and removes the race of the last chapter
in a different way: every thread gets its own copy of the whole force array,
and writes only into its copy. No two threads then write the same memory.
The function that does this is `computePrivateScatter`:

```{literalinclude} ../../src/engine/potentials/lj_kernels.cpp
:language: cpp
:start-at: "    // One full force array per thread."
:end-at: "        double e = 0.0;"
:caption: computePrivateScatter, one copy per thread (src/engine/potentials/lj_kernels.cpp)
```

- `bufX`, `bufY` and `bufZ` hold `T` force arrays each, one per thread, all
  starting at zero.
- **`#pragma omp parallel num_threads(T)`** starts `T` threads, and every
  thread runs the whole block in braces below it.
- **`omp_get_thread_num()`** gives each thread its number `t`, from 0 to
  `T - 1`. The thread then points `fx`, `fy`, `fz` at its own copy.

Inside the block, the scatter loop is split among the threads:

```{literalinclude} ../../src/engine/potentials/lj_kernels.cpp
:language: cpp
:start-at: "#pragma omp for schedule(static, 16) nowait"
:end-at: "for (std::ptrdiff_t j = i + 1; j < n; ++j) {"
:dedent: 8
:caption: computePrivateScatter, the shared loop
```

- **`omp for`** divides the iterations among the threads already started,
  without starting new ones.
- **`schedule(static, 16)`** deals out chunks of 16 atoms in turn: thread 0
  takes atoms 0–15, thread 1 takes 16–31, and so on, round and round. In the
  scatter loop atom `i` has `n - i - 1` partners, so the first atoms carry far
  more pairs than the last. Equal blocks would give thread 0 most of the
  work; small chunks dealt in turn give every thread a mix of long and short
  rows. The deal is fixed, so each force is always added up from the same
  pieces in the same order, and the result is the same on every run.
- **`nowait`**: a thread that finishes its chunks does not wait for the
  others at the end of this loop; the block ends right after it anyway.

The body of the loop is the scatter kernel of the last chapter, writing both
`f[i]` and `f[j]` into the thread's own copy. At the end, a second parallel
loop adds the copies, for each atom over threads 0, 1, 2, … in order:

```{literalinclude} ../../src/engine/potentials/lj_kernels.cpp
:language: cpp
:start-at: "    #pragma omp parallel for schedule(static) num_threads(T)"
:end-at: "    }"
:caption: computePrivateScatter, summing the copies
```

The copies cost memory: 10 threads × 4096 atoms × 3 components × 8 bytes is
about 1 MB, and the summing loop has to read all of it. Against the halved
arithmetic, that is cheap. The result differs from the serial kernel only by
rounding, at most 1.4 × 10⁻¹³ in a force component at N = 4096, because the
pieces of each force are added up in a different order.

## How much faster

A **speedup** is a time before divided by a time after. The time before is
the fastest serial kernel, `serial-triangular` at 10.14 ms for N = 4096;
dividing by the slower `serial-gather` would credit the threads with the work
that gather adds. `thread_sweep.cpp` times every backend at every thread
count (the faster of two runs):

| Threads | `std::thread` (gather) | `OpenMP` (gather) | `OpenMPPrivate` (scatter) |
| --- | --- | --- | --- |
| 1 | 17.75 ms · 0.57× | 18.20 ms · 0.56× | 9.89 ms · 1.02× |
| 2 | 9.31 ms · 1.09× | 9.12 ms · 1.11× | 4.90 ms · 2.07× |
| 4 | 5.01 ms · 2.02× | 4.95 ms · 2.05× | 3.12 ms · 3.25× |
| 6 | 5.15 ms · 1.97× | 4.14 ms · 2.45× | 3.01 ms · 3.36× |
| 8 | 4.04 ms · 2.51× | 3.57 ms · 2.84× | 2.33 ms · 4.35× |
| 10 | 3.89 ms · 2.60× | 3.17 ms · 3.20× | 2.24 ms · 4.52× |

```{figure} ../figures/par_backends.svg
:name: par-backends-figure
:width: 600px

Speedup over `serial-triangular` at N = 4096, against the number of threads,
for the three parallel backends. Below the dashed line, a backend is slower
than serial.
```

{numref}`par-backends-figure` shows the same numbers. Three things stand out.

- **Scatter wins from the first thread.** On one thread `OpenMPPrivate` is
  the serial kernel plus a copy, 1.02×; the gather backends start at 0.57×
  and spend their first two threads winning back the doubled arithmetic. At
  10 threads, private copies give 4.5×, gather 3.2×.
- **guided beats equal blocks.** The two gather backends run the same kernel
  and are equal up to 4 threads, one per performance core. From 5 threads on,
  `std::thread` with its equal blocks waits for the efficiency cores (2.02×
  at 4 threads, 1.97× at 6); `OpenMP` with `schedule(guided)` keeps gaining.
- **10 threads do not give 10×.** Six of the ten cores are efficiency cores,
  which run this loop more slowly than performance cores. Against its own
  one-thread time, the OpenMP gather loop reaches 5.7× on 10 threads, close
  to what 4 fast and 6 slower cores can deliver. `OpenMPPrivate` deals out
  fixed chunks, like `static`, which is why it gains little from 4 to 6
  threads before the smaller chunks per thread pay off at 8 and 10.

## Small systems

{numref}`par-speedup-figure` repeats the `OpenMPPrivate` measurement for
every system size.

```{figure} ../figures/par_speedup.svg
:name: par-speedup-figure
:width: 600px

Speedup of `OpenMPPrivate` over `serial-triangular`, against the number of
threads, for five system sizes.
```

The larger the system, the larger the speedup: 4.5× at N = 4096, 4.2× at
2197, 3.4× at 1000, 2.9× at 512. At N = 125 there is almost nothing to gain:
the serial force step takes 24 µs, and the best thread count gives 1.1×.
Starting threads costs time, and at small N that time is comparable to the
work. `thread_cost.cpp` measures it, for threads that do nothing:

```console
$ ./thread_cost
 1 threads:    7.9 microseconds to start and join
 2 threads:   10.4 microseconds to start and join
 4 threads:   17.1 microseconds to start and join
 8 threads:   35.2 microseconds to start and join
10 threads:   45.6 microseconds to start and join
 1 threads:    0.0 microseconds for an OpenMP region
 2 threads:    2.0 microseconds for an OpenMP region
 4 threads:    5.7 microseconds for an OpenMP region
 8 threads:    9.1 microseconds for an OpenMP region
10 threads:   11.7 microseconds for an OpenMP region
```

The `Threads` backend starts new threads on every call, which takes 46 µs
for 10 of them, longer than the whole serial force step at N = 125. OpenMP
starts its threads once and keeps them waiting between loops, so a parallel
region costs a few microseconds instead. That is why the OpenMP gather loop
is still 2.1× faster at N = 125 on 4 threads, where every `std::thread`
count is slower than serial. `OpenMPPrivate` pays something extra at small
N: it creates and zeroes its private copies on every call.

## The whole step

The force step was nearly all of a step ([Measure first](01_measure_first.md)),
so a faster force step makes a faster simulation. Run with the argument
`openmp`, `step_split.cpp` switches the LJ potential to `OpenMPPrivate` and
times the engine's own loop again:

```console
$ ./step_split openmp
     N    step_ms  forces_ms  energy_ms    rest_ms
   125     0.0393     0.0744     0.0568     0.0010
   343     0.0903     0.0747     0.0849     0.0028
   729     0.1801     0.1702     0.1678     0.0060
  1331     0.3811     0.3590     0.3490     0.0124
  2197     0.7957     0.7590     0.7640     0.0211
  4096     2.3071     2.2504     2.2076     0.0375
```

At N = 4096 one step of the simulation now takes 2.3 ms. The same step took
18.4 ms with the engine's default serial kernel, and 37.5 ms before the
energy came out of the force pass: 16 times faster in all, about half of it
from not running the loop twice and the rest from the threads.

Parallel kernels are opt-in; the engine runs the serial gather kernel unless
asked otherwise. In C++ the request is `setBackend`:

```cpp
lj->setBackend(LJBackend::OpenMPPrivate);
```

There is no setting for it in the YAML file yet. The environment variable
`MOLSIM_LJ_BACKEND` sets it for every `LJPotential` in a run, which works for
the Python scripts too:

```console
$ MOLSIM_LJ_BACKEND=openmp-private python run.py
```

## Tests that guard the threads

A threaded kernel can be fast and wrong, as the race showed. The engine's
unit tests check every backend against the serial one on every build, and
with OpenMP installed they now exercise all three parallel backends:

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
  that changes between runs. It also guards the choice of
  `schedule(static, 16)` in `OpenMPPrivate`: with a schedule that deals the
  chunks differently on each run, the copies would be added in a different
  order, and the last digits would change.
- **`NetForceIsZero`** requires the total force to be zero up to rounding.
  The racy loop of the last chapter fails it on every run.

The testing module, coming later, returns to these tests.

## Check yourself

:::{dropdown} 1. A colleague reports a 5.7× speedup for the OpenMP gather loop at 10 threads. What would you ask?
Faster than what? 5.7× is against the same gather loop on one thread, which
does twice the arithmetic it needs. Against the fastest serial kernel,
`serial-triangular`, it is 3.2×.
:::

:::{dropdown} 2. Why does OpenMPPrivate beat the OpenMP gather loop at every thread count?
It keeps the scatter loop, which computes each pair once; gather computes
each pair twice. The private copies remove the race at the cost of some
memory and one summing loop, which is much cheaper than doubling the
arithmetic.
:::

:::{dropdown} 3. With schedule(static), why is the gather loop slower on 5 threads than on 4?
Each thread gets an equal share. With 5 threads, one share runs on an
efficiency core and finishes last, and the four threads on performance cores
wait for it. `schedule(guided)` lets that thread take fewer chunks.
:::

:::{dropdown} 4. Why is the OpenMP gather loop faster than serial at N = 125, when the std::thread version never is?
OpenMP keeps its threads between loops, so a parallel region costs a few
microseconds. The `Threads` backend starts and joins new threads on every
call, 17 µs for 4 of them, which is most of the 24 µs serial force step.
:::

:::{dropdown} 5. What does reduction(+:acc) do, and which part of the hand-written backend does it replace?
Each thread adds into its own private copy of `acc`, and the copies are added
together at the end. It replaces the `partial[t]` slots and the final sum over
them.
:::

## Next

`OpenMPPrivate` is the fastest kernel on this processor, 4.5 times faster
than serial on 10 cores. Its trick does not grow with the number of threads:
every thread needs its own copy of the force array. That is 1 MB for 10
threads, but on a graphics card the force loop runs one thread per atom,
and 4096 copies of
4096 forces would take about 400 MB. With that many threads, the gather loop,
where every force has one writer and nothing is copied, is the right shape
again. The [GPU module](../gpu/index.md), still in progress, will run it there.
