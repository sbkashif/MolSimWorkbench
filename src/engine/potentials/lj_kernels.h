#ifndef LJ_KERNELS_H
#define LJ_KERNELS_H

#include <cstddef>

// ---------------------------------------------------------------------------
// lj_kernels — the backend seam.
//
// Everything below is deliberately POD: raw pointers, ints, doubles. No
// std::string, no std::map, no std::vector, no vector<array<double,3>>.
//
// That is not stylistic. It is the exact subset that a CUDA kernel can accept
// as device pointers. Keeping the seam this narrow is what lets a future
// lj_kernels.cu drop in beside lj_kernels.cpp without touching LJPotential.
//
// See docs/learn-parallel/ for the curriculum built around this file.
// ---------------------------------------------------------------------------

// A flat, structure-of-arrays (SoA) view of the system.
//
// SoA (x[], y[], z[]) rather than AoS (vector<array<double,3>>) because on a
// GPU it is what makes memory accesses coalesce across a warp. On this CPU it
// is measured ~13% SLOWER than AoS (docs/learn-parallel/02) -- a deliberate
// trade for GPU portability, not a CPU optimisation.
struct LJKernelView {
    // Positions, length n each.
    const double* x = nullptr;
    const double* y = nullptr;
    const double* z = nullptr;

    // Integer type id per atom, length n. Replaces the per-pair
    // std::map<std::string, ...> lookup in the original kernel.
    const int* type = nullptr;

    // Force output, length n each. Written by the kernel.
    double* fx = nullptr;
    double* fy = nullptr;
    double* fz = nullptr;

    std::size_t n = 0;

    // Pre-mixed parameter tables, row-major, ntypes x ntypes.
    // Indexed as table[ti * ntypes + tj]. Mixing rules are applied ONCE when
    // these are built, not once per pair as in the original implementation.
    const double* eps4 = nullptr;    // 4 * epsilon_ij
    const double* sigma2 = nullptr;  // sigma_ij^2
    int ntypes = 0;

    // Box dimensions. A value of 0.0 means "no periodicity on this axis".
    double bx = 0.0, by = 0.0, bz = 0.0;

    // Cutoff SQUARED. Stored squared so the inner loop never calls sqrt().
    double cutoff2 = 0.0;
};

// Which implementation to run. Adding LJBackend::Cuda later requires no change
// to any caller — only a new case inside ljComputeGather.
enum class LJBackend {
    // The fastest SERIAL kernel: triangular i<j loop exploiting Newton's third
    // law, so it does half the arithmetic of the gather form. This is the
    // honest denominator for any speedup claim (lesson 7) -- and it is also a
    // SCATTER, writing f[i] and f[j], which is exactly why it cannot be
    // naively threaded (lessons 3 and 5).
    SerialTriangular,

    Serial,   // one thread, gather formulation  (lesson 3)
    Threads,  // hand-rolled std::thread pool    (lesson 4)
    OpenMP,   // #pragma omp parallel for        (lesson 6)

    // The fastest CORRECT parallel kernel on this CPU: keep the triangular
    // scatter (half the arithmetic) and give every thread a private copy of
    // the whole force array, then sum the copies. No race, because no two
    // threads write the same memory.
    //
    // This is what LAMMPS and GROMACS do on CPU, and it beats the gather form
    // by ~1.9x here. It does NOT port to a GPU: the buffers cost
    // nthreads * N * 3 doubles, which is fine for 8 threads and absurd for
    // 100,000. Gather remains the GPU-bound path. See lesson 5.
    OpenMPPrivate
};

// Gather formulation: atom i accumulates over ALL j != i and writes only f[i].
//
// This does 2x the arithmetic of the triangular i<j loop because it discards
// Newton's third law. In exchange, atom i's force is written by exactly one
// thread -- no races, no atomics, no per-thread reduction buffers -- and the
// loop body is structurally identical to a CUDA thread-per-atom kernel.
//
// Returns the total potential energy (each pair counted once).
// Forces are overwritten, not accumulated.
double ljComputeGather(const LJKernelView& view, LJBackend backend, int numThreads);

// True if this build has OpenMP available.
bool ljOpenMPAvailable();

// Threads OpenMP would use by default (omp_get_max_threads, or 1 without OpenMP).
int ljDefaultThreadCount();

#endif  // LJ_KERNELS_H
