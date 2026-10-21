# About the parallel module

A computer today has several cores, each able to run its own stream of
instructions at the same time. The simulation from
[A minimal molecular simulation algorithm](../molsim/architecture.md) uses one.
This module finds where the simulation spends its time, splits that work
across cores, shows what goes wrong when two cores write the same memory,
and measures how much faster the result really is.

Every time on these pages was measured on one machine: an Apple M4 with 10
cores (4 performance cores and 6 efficiency cores), macOS 15.7, Apple clang
17 at `-O3`. Your numbers will differ; the shapes
should not. The programs that measured them are in `parallel/examples/`.

Chapters:

1. [Measure first](01_measure_first.md)
2. [Threads and races](02_threads_and_races.md)
3. [OpenMP and honest speedup](03_openmp_and_speedup.md)

The C++ module's chapters on [functions](../cpp/02_functions.md) and on
memory ([A first program](../cpp/01_first_program.md)) are enough to follow
this module.
