# About the GPU module

[OpenMP and speedup](../parallel/03_openmp_and_speedup.md) ran the pair loop
on the 10 cores of a processor. A graphics card, or **GPU**, runs thousands
of threads at once, each at a lower clock speed than a processor core: on
the machine below, 1.74 GHz at most against about 3.2 GHz. The card wins
only when it is given a great deal of the same work, and the pair loop of a
large system is exactly that. This module runs the
engine's gather loop on a GPU with **CUDA**, NVIDIA's way of programming its
graphics cards from C++, and measures what it gains against the fastest
kernel on the processor of the same machine.

The pages are measured on one machine: an NVIDIA A40 graphics card beside an
AMD EPYC 7763 processor, with CUDA 12.1 and g++ 8.5 on Linux. The parallel
module's times come from a different machine, so this module measures its
processor baselines again.

The module builds on the whole
[parallel module](../parallel/index.md): gather and scatter, races, and
what a fair speedup is measured against.

Chapters (being written):

1. Thousands of threads
2. The force loop on a GPU
3. How much faster
4. float or double
5. Shared memory
6. The whole step on the GPU

## Next

The first chapter, Thousands of threads, is being written. Until then,
[OpenMP and speedup](../parallel/03_openmp_and_speedup.md) is the last
finished page.
