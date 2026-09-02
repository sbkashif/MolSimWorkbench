#include "pool.h"

#include <algorithm>
#include <cmath>
#include <thread>

namespace ex04 {

std::pair<std::size_t, std::size_t> partitionRange(std::size_t n,
                                                   int numThreads,
                                                   int t) {
    // TODO(1): guard numThreads < 1 and t out of [0, numThreads).
    //
    // TODO(2): the naive split
    //             chunk = n / T;  begin = t*chunk;  end = begin + chunk;
    //          silently DROPS the last (n % T) items. With n=4096 and T=7 that
    //          is one atom that never gets a force. Do not do this.
    //
    //          Instead give the first (n % T) threads one extra item:
    //             chunk = n / T
    //             rem   = n % T
    //             begin = t*chunk + min(t, rem)
    //             end   = begin + chunk + (t < rem ? 1 : 0)
    (void)n; (void)numThreads; (void)t;
    return {0, 0};  // replace me
}

double parallelGather(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      double eps4, double sigma2,
                      double bx, double by, double bz,
                      double cutoff,
                      std::vector<double>& fx,
                      std::vector<double>& fy,
                      std::vector<double>& fz,
                      int numThreads) {
    const std::size_t n = x.size();
    fx.assign(n, 0.0);
    fy.assign(n, 0.0);
    fz.assign(n, 0.0);
    if (n == 0) return 0.0;
    if (numThreads < 1) numThreads = 1;
    numThreads = std::min<int>(numThreads, static_cast<int>(n));

    // TODO(3): make a std::vector<double> partial(numThreads, 0.0) so each
    //          thread has its own energy slot -- no shared accumulator.
    //
    // TODO(4): spawn the threads.
    //            - reserve() the vector<std::thread> first
    //            - CAPTURE begin, end AND t BY VALUE. Capturing the loop
    //              variable by reference is the classic bug: by the time the
    //              thread runs, the loop has moved on.
    //            - each thread runs the gather loop over [begin, end) and
    //              stores its energy in partial[t]
    //
    // TODO(5): join every thread. (A std::thread destroyed without join()
    //          calls std::terminate.)
    //
    // TODO(6): sum partial[] IN INDEX ORDER, not completion order.
    //          Floating-point addition is not associative, so summing as
    //          threads happen to finish would make the energy jitter between
    //          identical runs.
    //
    // TODO(7): gather double counts the energy -- halve the total.
    (void)y; (void)z; (void)eps4; (void)sigma2;
    (void)bx; (void)by; (void)bz; (void)cutoff;
    return 0.0;  // replace me
}

}  // namespace ex04
