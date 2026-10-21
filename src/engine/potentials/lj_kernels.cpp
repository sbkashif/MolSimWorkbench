#include "lj_kernels.h"

#include <cmath>
#include <thread>
#include <vector>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

// ---------------------------------------------------------------------------
// The kernel body. Every backend calls THIS, so all three run bit-for-bit
// identical arithmetic and any difference in their output is a parallelism
// bug, never an arithmetic one.
//
// Computes forces for atoms [i0, i1) and returns their summed pair energy
// (still double counted -- the caller halves it).
//
// This function is the thing that becomes a CUDA kernel. Compare:
//
//     for (size_t i = i0; i < i1; ++i)          // CPU: a loop over a range
//     size_t i = blockIdx.x*blockDim.x + threadIdx.x;   // GPU: no loop at all
//
// Everything below that line is unchanged in the port.
// ---------------------------------------------------------------------------
double computeRange(const LJKernelView& v, std::size_t i0, std::size_t i1) {
    // Hoisted out of the loop: reciprocal box lengths, so the minimum-image
    // convention costs a multiply instead of a divide.
    //
    // The 0.0 case is handled without a branch: if bx == 0 then invbx == 0,
    // so round(dx * 0) == 0 and dx -= bx * 0 leaves dx untouched. A branch
    // inside the hot loop would cost more than the arithmetic it saves.
    const double invbx = (v.bx > 0.0) ? 1.0 / v.bx : 0.0;
    const double invby = (v.by > 0.0) ? 1.0 / v.by : 0.0;
    const double invbz = (v.bz > 0.0) ? 1.0 / v.bz : 0.0;

    const std::size_t n = v.n;
    const int ntypes = v.ntypes;
    double energy = 0.0;

    for (std::size_t i = i0; i < i1; ++i) {
        // Accumulate into registers, not into v.fx[i]. Writing to memory once
        // per atom instead of once per pair is worth more than it looks, and
        // on a GPU these are literally registers.
        double fxi = 0.0, fyi = 0.0, fzi = 0.0;
        double ei = 0.0;

        const double xi = v.x[i], yi = v.y[i], zi = v.z[i];
        const int ti = v.type[i];
        // Row of the mixed-parameter table for atom i's type. Hoisting this
        // out of the j loop turns a 2-D index into a 1-D one.
        const double* eps4row = v.eps4 + static_cast<std::size_t>(ti) * ntypes;
        const double* sig2row = v.sigma2 + static_cast<std::size_t>(ti) * ntypes;

        for (std::size_t j = 0; j < n; ++j) {
            if (j == i) continue;

            double dx = v.x[j] - xi;
            double dy = v.y[j] - yi;
            double dz = v.z[j] - zi;

            // Minimum image convention.
            dx -= v.bx * std::round(dx * invbx);
            dy -= v.by * std::round(dy * invby);
            dz -= v.bz * std::round(dz * invbz);

            const double r2 = dx * dx + dy * dy + dz * dz;

            // Cutoff test in r^2. No sqrt is ever taken in this kernel:
            // both the energy and the force can be written in terms of r^2.
            if (r2 > v.cutoff2 || r2 < 1e-20) continue;

            const int tj = v.type[j];
            const double eps4 = eps4row[tj];    // 4 * epsilon_ij, pre-mixed
            const double sig2 = sig2row[tj];    // sigma_ij^2,     pre-mixed

            const double sr2 = sig2 / r2;
            const double sr6 = sr2 * sr2 * sr2;      // (sigma/r)^6 with 2 multiplies
            const double sr12 = sr6 * sr6;           // instead of std::pow(x, 6)

            // E = 4*eps*(sr12 - sr6)
            ei += eps4 * (sr12 - sr6);

            // F/r = 24*eps*(2*sr12 - sr6)/r^2, and 24*eps == 6*eps4.
            // Multiplying this by the displacement vector gives the force
            // directly -- no normalisation, hence no sqrt.
            const double fOverR = 6.0 * eps4 * (2.0 * sr12 - sr6) / r2;

            fxi -= fOverR * dx;
            fyi -= fOverR * dy;
            fzi -= fOverR * dz;
        }

        // The single write per atom. Because i is owned by exactly one thread,
        // this needs no lock, no atomic, and no per-thread scratch buffer.
        v.fx[i] = fxi;
        v.fy[i] = fyi;
        v.fz[i] = fzi;
        energy += ei;
    }

    return energy;
}


// ---------------------------------------------------------------------------
// The TRIANGULAR (scatter) kernel: i < j, using Newton's third law so each
// pair is visited once and the result applied to both atoms.
//
// Half the arithmetic of the gather kernel, so this is the fastest serial
// code here and therefore the correct baseline for measuring speedup.
//
// It is also unthreadable as written. Two threads handling different i can
// hold the same j, and `v.fx[j] += ...` is then a read-modify-write race.
// Lesson 5 turns that race on deliberately and watches the energy drift.
// ---------------------------------------------------------------------------
double computeTriangular(const LJKernelView& v) {
    const double invbx = (v.bx > 0.0) ? 1.0 / v.bx : 0.0;
    const double invby = (v.by > 0.0) ? 1.0 / v.by : 0.0;
    const double invbz = (v.bz > 0.0) ? 1.0 / v.bz : 0.0;

    const std::size_t n = v.n;
    const int ntypes = v.ntypes;
    double energy = 0.0;

    // Scatter accumulates, so the outputs must start at zero.
    for (std::size_t i = 0; i < n; ++i) { v.fx[i] = 0.0; v.fy[i] = 0.0; v.fz[i] = 0.0; }

    for (std::size_t i = 0; i < n; ++i) {
        const double xi = v.x[i], yi = v.y[i], zi = v.z[i];
        const int ti = v.type[i];
        const double* eps4row = v.eps4 + static_cast<std::size_t>(ti) * ntypes;
        const double* sig2row = v.sigma2 + static_cast<std::size_t>(ti) * ntypes;

        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = v.x[j] - xi;
            double dy = v.y[j] - yi;
            double dz = v.z[j] - zi;

            dx -= v.bx * std::round(dx * invbx);
            dy -= v.by * std::round(dy * invby);
            dz -= v.bz * std::round(dz * invbz);

            const double r2 = dx * dx + dy * dy + dz * dz;
            if (r2 > v.cutoff2 || r2 < 1e-20) continue;

            const int tj = v.type[j];
            const double eps4 = eps4row[tj];
            const double sig2 = sig2row[tj];

            const double sr2 = sig2 / r2;
            const double sr6 = sr2 * sr2 * sr2;
            const double sr12 = sr6 * sr6;

            energy += eps4 * (sr12 - sr6);
            const double fOverR = 6.0 * eps4 * (2.0 * sr12 - sr6) / r2;

            // THE SCATTER. Two writes, and the j write is the dangerous one:
            // j is not owned by this iteration of the outer loop.
            v.fx[i] -= fOverR * dx; v.fy[i] -= fOverR * dy; v.fz[i] -= fOverR * dz;
            v.fx[j] += fOverR * dx; v.fy[j] += fOverR * dy; v.fz[j] += fOverR * dz;
        }
    }
    return energy;
}

// ---------------------------------------------------------------------------
// Triangular scatter, parallelised with PRIVATE per-thread force buffers.
//
// The race in the naive parallel scatter comes from two threads writing the
// same f[j]. Here each thread writes only into its own buffer, so the write
// conflict is removed without giving up Newton's third law. The buffers are
// summed afterwards in a fixed thread order, which keeps the result
// deterministic run to run.
//
// Cost: numThreads * n * 3 doubles. At 8 threads and N=4096 that is 0.8 MB.
// At GPU thread counts it would be gigabytes -- which is exactly why the GPU
// path uses gather instead.
// ---------------------------------------------------------------------------
#ifdef _OPENMP
double computePrivateScatter(const LJKernelView& v, int numThreads) {
    const double invbx = (v.bx > 0.0) ? 1.0 / v.bx : 0.0;
    const double invby = (v.by > 0.0) ? 1.0 / v.by : 0.0;
    const double invbz = (v.bz > 0.0) ? 1.0 / v.bz : 0.0;

    const std::ptrdiff_t n = static_cast<std::ptrdiff_t>(v.n);
    const int ntypes = v.ntypes;
    const int T = numThreads;

    // One full force array per thread.
    std::vector<std::vector<double>> bufX(T, std::vector<double>(v.n, 0.0));
    std::vector<std::vector<double>> bufY(T, std::vector<double>(v.n, 0.0));
    std::vector<std::vector<double>> bufZ(T, std::vector<double>(v.n, 0.0));
    std::vector<double> partial(T, 0.0);

    #pragma omp parallel num_threads(T)
    {
        const int t = omp_get_thread_num();
        double* fx = bufX[t].data();
        double* fy = bufY[t].data();
        double* fz = bufZ[t].data();
        double e = 0.0;

        // schedule(static, 16), NOT schedule(dynamic, 16).
        //
        // A triangular loop is badly imbalanced -- row i has (n-i) pairs -- so
        // plain schedule(static) would hand thread 0 many times the work of the
        // last thread. Small round-robin chunks fix that: each thread gets an
        // interleaved mix of long and short rows.
        //
        // It has to be *static* rather than *dynamic* for a subtler reason.
        // Each atom's final force is a sum over the per-thread buffers, and
        // floating-point addition is not associative. Dynamic scheduling
        // assigns chunks to threads differently on every run, so the same
        // values get partitioned differently, sum in a different order, and
        // round differently -- the forces then change in the last bits between
        // identical runs. Static scheduling fixes the partition, so the result
        // is reproducible. (LJKernels.RepeatedRunsAreDeterministic catches this.)
        #pragma omp for schedule(static, 16) nowait
        for (std::ptrdiff_t i = 0; i < n; ++i) {
            const double xi = v.x[i], yi = v.y[i], zi = v.z[i];
            const double* eps4row = v.eps4 + static_cast<std::size_t>(v.type[i]) * ntypes;
            const double* sig2row = v.sigma2 + static_cast<std::size_t>(v.type[i]) * ntypes;

            for (std::ptrdiff_t j = i + 1; j < n; ++j) {
                double dx = v.x[j] - xi;
                double dy = v.y[j] - yi;
                double dz = v.z[j] - zi;
                dx -= v.bx * std::round(dx * invbx);
                dy -= v.by * std::round(dy * invby);
                dz -= v.bz * std::round(dz * invbz);

                const double r2 = dx * dx + dy * dy + dz * dz;
                if (r2 > v.cutoff2 || r2 < 1e-20) continue;

                const double eps4 = eps4row[v.type[j]];
                const double sig2 = sig2row[v.type[j]];
                const double sr2 = sig2 / r2;
                const double sr6 = sr2 * sr2 * sr2;
                const double sr12 = sr6 * sr6;

                e += eps4 * (sr12 - sr6);
                const double fOverR = 6.0 * eps4 * (2.0 * sr12 - sr6) / r2;

                // Both writes go into THIS thread's private buffer.
                fx[i] -= fOverR * dx; fy[i] -= fOverR * dy; fz[i] -= fOverR * dz;
                fx[j] += fOverR * dx; fy[j] += fOverR * dy; fz[j] += fOverR * dz;
            }
        }
        partial[t] = e;
    }

    // Reduction: sum the private buffers. Parallel over atoms, and the inner
    // sum runs over threads in index order so the result is reproducible.
    #pragma omp parallel for schedule(static) num_threads(T)
    for (std::ptrdiff_t i = 0; i < n; ++i) {
        double ax = 0.0, ay = 0.0, az = 0.0;
        for (int t = 0; t < T; ++t) { ax += bufX[t][i]; ay += bufY[t][i]; az += bufZ[t][i]; }
        v.fx[i] = ax; v.fy[i] = ay; v.fz[i] = az;
    }

    double energy = 0.0;
    for (double e : partial) energy += e;
    return energy;
}
#endif  // _OPENMP

}  // namespace

double ljComputeGather(const LJKernelView& v, LJBackend backend, int numThreads) {
    if (v.n == 0) return 0.0;

    if (numThreads < 1) numThreads = ljDefaultThreadCount();
    // Never spawn more threads than atoms.
    numThreads = std::min<int>(numThreads, static_cast<int>(v.n));

    double doubleCounted = 0.0;

    switch (backend) {
        case LJBackend::OpenMPPrivate: {
#ifdef _OPENMP
            // Triangular, so each pair is counted once already -- not halved.
            return computePrivateScatter(v, numThreads);
#else
            return computeTriangular(v);
#endif
        }

        case LJBackend::SerialTriangular: {
            // Already counts each pair exactly once, so it is NOT halved below.
            return computeTriangular(v);
        }

        case LJBackend::Serial: {
            doubleCounted = computeRange(v, 0, v.n);
            break;
        }

        case LJBackend::Threads: {
            // Hand-rolled partitioning: exactly what OpenMP's schedule(static)
            // does for you, written out so you can see it (lesson 4).
            const std::size_t n = v.n;
            const int T = numThreads;
            std::vector<double> partial(static_cast<std::size_t>(T), 0.0);
            std::vector<std::thread> pool;
            pool.reserve(static_cast<std::size_t>(T));

            for (int t = 0; t < T; ++t) {
                // Balanced split: the first (n % T) chunks get one extra atom,
                // so no thread is ever more than one atom behind another.
                const std::size_t chunk = n / static_cast<std::size_t>(T);
                const std::size_t rem = n % static_cast<std::size_t>(T);
                const std::size_t ut = static_cast<std::size_t>(t);
                const std::size_t i0 = ut * chunk + std::min(ut, rem);
                const std::size_t i1 = i0 + chunk + (ut < rem ? 1u : 0u);

                // Each thread writes partial[t] -- its own slot -- so the
                // energy reduction needs no synchronisation either.
                pool.emplace_back([&v, i0, i1, t, &partial]() {
                    partial[static_cast<std::size_t>(t)] = computeRange(v, i0, i1);
                });
            }
            for (auto& th : pool) th.join();

            // Summed in a fixed thread order, so the result is deterministic
            // run to run. Floating-point addition is not associative; summing
            // in completion order would make the energy jitter in the last bits.
            for (double p : partial) doubleCounted += p;
            break;
        }

        case LJBackend::OpenMP: {
#ifdef _OPENMP
            const std::ptrdiff_t n = static_cast<std::ptrdiff_t>(v.n);
            double acc = 0.0;
            // schedule(guided), not schedule(static).
            //
            // Measured on an Apple M4 (4 performance + 6 efficiency cores),
            // N=4096: static took 6.07 ms on 5 threads vs 5.01 ms on 4 -- adding
            // a thread made it SLOWER. static hands every thread an equal slice,
            // so the slice that lands on an efficiency core finishes last and
            // the other threads sit at the barrier waiting for it.
            //
            // guided hands out shrinking chunks on demand, so a slow core simply
            // takes fewer of them. Same result, no straggler: 4.52 ms on 5
            // threads, 3.17 ms on 10 (static: 3.87 ms on 10).
            //
            // Reproduce this yourself in exercise 06 before believing it.
            #pragma omp parallel for schedule(guided) reduction(+:acc) num_threads(numThreads)
            for (std::ptrdiff_t i = 0; i < n; ++i) {
                acc += computeRange(v, static_cast<std::size_t>(i),
                                       static_cast<std::size_t>(i) + 1);
            }
            doubleCounted = acc;
#else
            doubleCounted = computeRange(v, 0, v.n);
#endif
            break;
        }
    }

    // Gather visits every pair twice (once as i-j, once as j-i).
    return 0.5 * doubleCounted;
}

bool ljOpenMPAvailable() {
#ifdef _OPENMP
    return true;
#else
    return false;
#endif
}

int ljDefaultThreadCount() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    const unsigned hw = std::thread::hardware_concurrency();
    return hw > 0 ? static_cast<int>(hw) : 1;
#endif
}
