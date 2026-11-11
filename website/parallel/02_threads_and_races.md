# Threads and races

[Measure first](01_measure_first.md) found that nearly all of a step is the
pair loop. This chapter splits that loop across threads. There are two ways
to write the loop, and they behave very differently once several threads run
it: one of them gives wrong forces, differently on every run.

## Splitting the work

The loop runs over atoms: for each atom `i`, visit its partners. The natural
split gives each thread a block of atoms. With 8 threads and 4096 atoms,
thread 0 takes atoms 0–511, thread 1 takes 512–1023, and so on. The engine's
`Threads` backend does exactly this in `ljComputeGather`: it starts one
`std::thread` per block, and then `join`s each one, which "waits for the
thread to finish its execution"
([std::thread](https://en.cppreference.com/w/cpp/thread/thread)).

What each thread writes depends on how the pair loop is written.

## Two ways to write the pair loop

**Scatter.** Visit each pair once, with `j > i`, and apply its force to both
atoms: by Newton's third law, the force on `j` is minus the force on `i`. The
engine's serial kernel `computeTriangular` works this way. Each pair is
computed once, but a pair writes two forces, `f[i]` and `f[j]`.

**Gather.** For each atom `i`, visit every other atom `j` and add up the
force on `i` only. The engine's `computeRange` works this way. Each pair is
computed twice, once from each side, but an atom's force is written only
while that atom is `i`.

Run on one thread, scatter is faster, because it does half the arithmetic.
Measured with `thread_sweep.cpp` at N = 4096 (the faster of two runs):

| Kernel | Writes per pair | Time per call |
| --- | --- | --- |
| `serial-triangular` (scatter) | 2 | 10.14 ms |
| `serial-gather` | 1 (each pair visited twice) | 18.82 ms |

Split across threads, the difference is in who writes what
({numref}`par-scatter-gather-figure`). With gather, thread A writes only the
forces of its own atoms. With scatter, thread A's pair (1, 6) writes `f[6]`,
which belongs to thread B's block, and thread B writes `f[6]` too.

```{figure} ../figures/par_scatter_gather.svg
:name: par-scatter-gather-figure

Two threads, eight atoms. Left: with scatter, a pair can write a force in the
other thread's block, so `f[6]` has two writers. Right: with gather, every
force has exactly one writer.
```

## A race, measured

`naive_scatter.cpp` splits the scatter loop across 8 threads in blocks, and
lets every thread add into the same force arrays:

```{literalinclude} examples/naive_scatter.cpp
:language: cpp
:start-at: "            fx[i] -= f * dx;"
:end-at: "            fx[j] += f * dx;"
:caption: naive_scatter.cpp (the two writes of each pair)
```

It runs the threaded loop five times on 4096 atoms and compares each result
with the serial one. Two numbers per run: the largest error in any force
component, and the size of the total force on all atoms, which Newton's third
law makes zero (the builder's progress lines are left out):

```console
$ ./naive_scatter
N = 4096, 8 threads
 run max|F - Fserial|  |sum of forces|
   1      5.208e+00        5.527e+00
   2      7.735e+00        1.064e+01
   3      2.153e+01        1.986e+01
   4      1.444e+00        2.301e+00
   5      3.866e+00        3.772e+00
serial: |sum of forces| = 2.143e-13, largest force component = 4.056e+01
```

Every run is wrong, and wrong by a different amount: some force is off by up
to 21.5, about half of the largest force in the system (40.6). The total
force, 2 × 10⁻¹³ when run serially (zero up to rounding), comes out between
2.3 and 19.9. A second run of the program gives five different numbers again.

### What goes wrong

`fx[j] += f * dx` looks like one step, but the processor does it in three:
read `fx[j]` from memory into a register, add, write the register back (the
same pattern as in [A first program](../cpp/01_first_program.md)). If two
threads do this to the same `fx[j]` at the same time, both can read the old
value before either writes, and then the second write replaces the first
({numref}`par-race-figure`). One contribution is lost.

```{figure} ../figures/par_race.svg
:name: par-race-figure

Two threads add to `f[6]` at the same time. Both read 2.0; A writes 2.5, then
B writes 3.0. A's addition is lost.
```

Whether two threads collide depends on exactly when each one reaches each
line, which changes from run to run; so the error changes too. C++ names this
a **data race**: two evaluations conflict "if one of them modifies a memory
location" and "the other one reads or modifies the same memory location",
and "if a data race occurs, the behavior of the program is undefined"
([multi-threaded executions and data races](https://en.cppreference.com/w/cpp/language/multithread)).
Undefined means the language promises nothing at all about the result, not
even that the error stays small.

## How the engine avoids it

The engine's threaded backends use **gather**. Each thread runs
`computeRange` on its own block of atoms, so every force has one writer and
no two threads ever write the same memory. The energy is handled the same
way: each thread adds its share into its own slot, `partial[t]`, and the
slots are added up after all threads have finished, always in thread order 0,
1, 2, … so that the sum comes out the same on every run.

Gather pays for its safety with the doubled arithmetic measured above. The
engine has one more backend, `OpenMPPrivate`, that keeps scatter and removes
the race differently: each thread adds into its own private copy of the whole
force array, and the copies are summed at the end. It is written with OpenMP,
the subject of the next chapter, and it turns out to be the fastest.

The threaded gather gives exactly the serial answer, to the last bit, at
every thread count and every N that `thread_sweep.cpp` measured: the largest
difference from `serial-triangular`, in any force component, is 0. Gather and
scatter add up the same numbers, and here even in an order that rounds the
same way.

:::{admonition} If you know Python
:class: tip
`x += 1` in Python is also a read, an add and a write. In the standard
interpreter the lock described in [Measure first](01_measure_first.md) makes
such races rarer in pure Python code, but it does not rule them out between
those steps; shared counters in threaded Python code still need a lock.
:::

## Check yourself

:::{dropdown} 1. In the scatter loop, which of the two writes of a pair can race, f[i] or f[j]?
Mostly `f[j]`: `j` can be in any thread's block. `f[i]` is in the thread's
own block, but it is not safe either, because another thread's pair can have
this atom as its `j`.
:::

:::{dropdown} 2. The engine's threaded gather also computes the energy. Why can the energy not race?
Each thread adds its share of the energy into its own slot, `partial[t]`, and
the slots are added up only after every thread has finished. No two threads
ever write the same slot.
:::

:::{dropdown} 3. Gather does twice the arithmetic of scatter. Why use it?
Because every force then has one writer: threads never write the same memory,
so there is no race and no need for locks or private copies. On one thread it
is about 1.9 times slower (18.8 against 10.1 ms at N = 4096); the threads have
to win that back.
:::

## Next

[OpenMP and speedup](03_openmp_and_speedup.md) writes the same split with one
line of OpenMP, adds the private-copy scatter, and measures how much faster
each is than the best serial kernel.
