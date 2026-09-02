#include "timing.h"

#include <algorithm>
#include <limits>

namespace ex01 {

double timeMs(const std::function<void()>& f, int reps) {
    // TODO(1): call f() once WITHOUT timing it (warm-up).
    //          The first call pays page faults, cold caches and thread-pool
    //          creation; timing it would measure startup, not the kernel.
    //
    // TODO(2): if reps < 1, treat it as 1.
    //
    // TODO(3): call f() `reps` times. Time EACH call separately with
    //          std::chrono::high_resolution_clock and keep the SMALLEST.
    //          Interference only ever makes a run slower, so the fastest run
    //          is the best estimate of the true cost.
    //
    // Convert with:
    //   std::chrono::duration<double, std::milli>(t1 - t0).count()
    (void)f;
    (void)reps;
    return 0.0;  // replace me
}

double speedup(double baselineMs, double candidateMs) {
    // TODO(4): return baselineMs / candidateMs, guarding candidateMs <= 0.
    (void)baselineMs;
    (void)candidateMs;
    return 0.0;  // replace me
}

double parallelEfficiency(double t1, double tN, int n) {
    // TODO(5): return (t1/tN)/n, guarding tN <= 0 and n < 1.
    (void)t1;
    (void)tN;
    (void)n;
    return 0.0;  // replace me
}

}  // namespace ex01
