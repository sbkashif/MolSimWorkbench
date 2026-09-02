#include "omp_kernel.h"

#include <algorithm>
#include <cmath>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace ex06 {

double ompGather(const std::vector<double>& x,
                 const std::vector<double>& y,
                 const std::vector<double>& z,
                 double eps4, double sigma2,
                 double bx, double by, double bz, double cutoff,
                 std::vector<double>& fx, std::vector<double>& fy,
                 std::vector<double>& fz, int numThreads, Schedule s) {
    const std::size_t n = x.size();
    fx.assign(n, 0.0); fy.assign(n, 0.0); fz.assign(n, 0.0);
    if (n == 0) return 0.0;

    // TODO(1): hoist invbx/invby/invbz and cutoff2 as before.
    //
    // TODO(2): write the gather loop ONCE as a lambda or macro taking i, so you
    //          do not copy-paste the body four times. Then dispatch on `s`:
    //
    //   double acc = 0.0;
    //   switch (s) {
    //     case Schedule::Static:
    //       #pragma omp parallel for schedule(static) reduction(+:acc) \
    //               num_threads(numThreads)
    //       for (std::ptrdiff_t i = 0; i < (std::ptrdiff_t)n; ++i) acc += body(i);
    //       break;
    //     case Schedule::StaticChunk:  ... schedule(static, 16)  ...
    //     case Schedule::Dynamic:      ... schedule(dynamic, 32) ...
    //     case Schedule::Guided:       ... schedule(guided)      ...
    //   }
    //
    //   The pragma must sit immediately above the `for`. The loop index must be
    //   SIGNED (std::ptrdiff_t) for portable OpenMP.
    //
    // TODO(3): remember the gather energy is double counted -- return acc/2.
    //
    // If _OPENMP is not defined, just run the loop serially; the code must
    // still build and give the right answer.
    (void)y; (void)z; (void)eps4; (void)sigma2;
    (void)bx; (void)by; (void)bz; (void)cutoff; (void)numThreads; (void)s;
    return 0.0;  // replace me
}

double deterministicGather(const std::vector<double>& x,
                           const std::vector<double>& y,
                           const std::vector<double>& z,
                           double eps4, double sigma2,
                           double bx, double by, double bz, double cutoff,
                           std::vector<double>& fx, std::vector<double>& fy,
                           std::vector<double>& fz, int numThreads) {
    // TODO(4): same kernel, but instead of reduction(+:acc):
    //            - std::vector<double> partial(numThreads, 0.0)
    //            - inside the parallel region each thread adds into
    //              partial[omp_get_thread_num()]
    //            - after the region, sum partial[] in INDEX order
    //          Use schedule(static) so the partition is also fixed.
    //
    //          reduction(+:) combines partials in an unspecified order, and
    //          floating-point addition is not associative, so the total can
    //          differ in its last bits between runs. This version cannot.
    (void)x; (void)y; (void)z; (void)eps4; (void)sigma2;
    (void)bx; (void)by; (void)bz; (void)cutoff;
    (void)fx; (void)fy; (void)fz; (void)numThreads;
    return 0.0;  // replace me
}

bool openMPEnabled() {
#ifdef _OPENMP
    return true;
#else
    return false;
#endif
}

int maxThreads() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

// ---------------------------------------------------------------------------
// YOUR ANSWERS (run ./build/exercises/ex06_schedule first)
//
// Q1. At which thread count does schedule(static) stop scaling, and what does
//     that number correspond to on your machine?
//
//     ANSWER:
//
// Q2. Does schedule(guided) fix it? By how much, at that thread count?
//
//     ANSWER:
//
// Q3. Why does handing every thread an equal share hurt when the cores are not
//     equally fast? (One sentence about barriers.)
//
//     ANSWER:
// ---------------------------------------------------------------------------

}  // namespace ex06
