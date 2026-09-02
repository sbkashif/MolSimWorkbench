#include "race.h"

#include <algorithm>
#include <cmath>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace ex05 {

double racyScatter(const std::vector<double>& x,
                   const std::vector<double>& y,
                   const std::vector<double>& z,
                   double eps4, double sigma2,
                   double bx, double by, double bz, double cutoff,
                   std::vector<double>& fx, std::vector<double>& fy,
                   std::vector<double>& fz, int numThreads) {
    const std::size_t n = x.size();
    fx.assign(n, 0.0); fy.assign(n, 0.0); fz.assign(n, 0.0);

    // TODO(1): the triangular i<j kernel, with
    //             #pragma omp parallel for schedule(dynamic,16) \
    //                     reduction(+:energy) num_threads(numThreads)
    //          on the OUTER loop, and BOTH scatter writes left unguarded:
    //             fx[i] -= fo*dx;   // safe:  i is this thread's
    //             fx[j] += fo*dx;   // RACE:  j belongs to nobody
    //
    //          Use a signed loop index (std::ptrdiff_t) -- OpenMP requires it.
    //          This is undefined behaviour and that is the point of the exercise.
    (void)y; (void)z; (void)eps4; (void)sigma2;
    (void)bx; (void)by; (void)bz; (void)cutoff; (void)numThreads;
    return 0.0;  // replace me
}

double privateBufferScatter(const std::vector<double>& x,
                            const std::vector<double>& y,
                            const std::vector<double>& z,
                            double eps4, double sigma2,
                            double bx, double by, double bz, double cutoff,
                            std::vector<double>& fx, std::vector<double>& fy,
                            std::vector<double>& fz, int numThreads) {
    const std::size_t n = x.size();
    fx.assign(n, 0.0); fy.assign(n, 0.0); fz.assign(n, 0.0);
    if (n == 0) return 0.0;
    if (numThreads < 1) numThreads = 1;

    // TODO(2): allocate numThreads private force arrays, each of length n:
    //             std::vector<std::vector<double>> bufX(numThreads,
    //                                                   std::vector<double>(n, 0.0));
    //          (and bufY, bufZ). Cost: numThreads * n * 3 doubles.
    //
    // TODO(3): inside `#pragma omp parallel num_threads(numThreads)`:
    //            - int t = omp_get_thread_num();
    //            - grab raw pointers to bufX[t] etc.
    //            - `#pragma omp for schedule(static, 16) nowait` over i
    //            - run the SAME triangular kernel, but write into the private
    //              buffers instead of fx/fy/fz. Both writes are now safe.
    //
    //          Why schedule(STATIC,16) and not dynamic?
    //            * the ,16 chunking is for load balance -- a triangular loop
    //              gives row i exactly (n-i) pairs, so plain static would hand
    //              thread 0 many times the work of the last thread.
    //            * `static` rather than `dynamic` is for REPRODUCIBILITY. Each
    //              atom's final force is a sum over the private buffers, and
    //              floating-point addition is not associative. Dynamic
    //              scheduling repartitions the work differently every run, so
    //              the sums round differently and the forces change in their
    //              last bits between identical runs. A test checks this.
    //
    // TODO(4): reduce -- for each atom, sum over threads IN INDEX ORDER into
    //          fx/fy/fz. Sum the per-thread energies in index order too.
    (void)y; (void)z; (void)eps4; (void)sigma2;
    (void)bx; (void)by; (void)bz; (void)cutoff;
    return 0.0;  // replace me
}

std::size_t countWrongAtoms(const std::vector<std::array<double, 3>>& reference,
                            const std::vector<double>& fx,
                            const std::vector<double>& fy,
                            const std::vector<double>& fz,
                            double relTol) {
    // TODO(5): count atoms where any of the three components differs from the
    //          reference by more than relTol RELATIVE. Guard against dividing
    //          by a near-zero reference component (use max(|ref|, 1e-30)).
    (void)reference; (void)fx; (void)fy; (void)fz; (void)relTol;
    return 0;  // replace me
}

}  // namespace ex05
