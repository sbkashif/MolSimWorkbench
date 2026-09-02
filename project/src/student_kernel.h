#ifndef STUDENT_KERNEL_H
#define STUDENT_KERNEL_H

#include "engine/potentials/lj_kernels.h"

// Do not change these signatures -- the acceptance tests depend on them.

enum class StudentBackend {
    Triangular,             // best serial: i<j, Newton's third law
    Gather,                 // serial gather: i over all j, writes only f[i]
    Parallel,               // your best parallel effort
    ParallelDeterministic   // parallel AND bitwise reproducible
};

// Compute forces into view.fx/fy/fz and return the total potential energy
// (each pair counted ONCE). Forces are overwritten, not accumulated.
double studentCompute(const LJKernelView& view, StudentBackend backend, int numThreads);

// Analytic dispersion tail correction (lesson 8). Energy only -- it must not
// change any force. Return 0.0 for non-physical input.
double studentTailEnergy(std::size_t n, double volume, double epsilon,
                         double sigma, double cutoff);

#endif
