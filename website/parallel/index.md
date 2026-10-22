# About the parallel module: CPU cores

[Reading the engine](../engine/reading_the_engine.md) ended at the pair
loop, which visits every pair of atoms on every step and runs on one core of
the processor. A computer today has several cores, each able to run its own
stream of instructions at the same time. This module finds where the
simulation spends its time, splits that work across cores, shows what goes
wrong when two cores write the same memory, and measures how much faster the
result really is.

Every time on these pages was measured on one machine: an Apple M4 with 10
cores (4 performance cores and 6 efficiency cores), macOS 15.7, Apple clang
17 at `-O3`. Your numbers will differ; the shapes
should not. The programs that measured them are in `parallel/examples/`.

Chapters:

1. [Measure first](01_measure_first.md)
2. [Threads and races](02_threads_and_races.md)
3. [OpenMP and speedup](03_openmp_and_speedup.md)

The module reads the engine's own force code, so it builds on
[Reading the engine](../engine/reading_the_engine.md), in particular
[the pair loop](../engine/reading_the_engine.md#the-pair-loop), and on the
C++ module's chapters on [memory](../cpp/01_first_program.md),
[functions](../cpp/02_functions.md) and [classes](../cpp/04_classes.md).
The [GPU module](../gpu/index.md) that follows runs the same loop on a
graphics card instead of the processor's cores.

## Next

Start with [Measure first](01_measure_first.md), which times one step of the
simulation piece by piece before anything is made parallel.
