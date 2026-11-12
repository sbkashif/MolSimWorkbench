# About the GPU module

:::{admonition} Draft
:class: warning
This module is still being written. This page describes the plan; the
chapters below do not exist yet, and no GPU timings are published here. The
chapters will appear as their measurements are finished.
:::

[OpenMP and speedup](../parallel/03_openmp_and_speedup.md) ran the pair loop
on the 10 cores of a processor. A graphics card, or **GPU**, runs thousands
of threads at once, each at a lower clock speed than a processor core: on
the machine below, 1.74 GHz at most against up to 3.5 GHz. The card wins
only when it is given a great deal of the same work, and the pair loop of a
large system is exactly that. This module will run the
engine's gather loop on a GPU with **CUDA**, NVIDIA's way of programming its
graphics cards from C++, and measure what it gains against the fastest
kernel on the processor of the same machine.

The chapters will be measured on one machine: an NVIDIA A40 graphics card
beside an AMD EPYC 7763 processor, with CUDA 12.1 and g++ 8.5 on Linux. The
parallel module's times come from a different machine, so this module will
measure its processor baselines again.

The module builds on the whole
[parallel module](../parallel/index.md): gather and scatter, races, and
what a fair speedup is measured against.

Planned chapters:

1. Thousands of threads
2. The force loop on a GPU
3. How much faster
4. float or double
5. Shared memory
6. The whole step on the GPU

## Next

The first chapter, Thousands of threads, is being written. The next finished
section leaves speed behind and looks at a different energy function: one
learned from quantum chemistry data, and how training it can be handed to a
language model. It starts with
[Use case: training an MLIP](../agent/01_training_an_mlip.md).
