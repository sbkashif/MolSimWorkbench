#include "scaling.h"

#include <algorithm>
#include <limits>

namespace ex07 {

double amdahlSpeedup(double serialFraction, int n) {
    // TODO(1): return 1.0 / (s + (1 - s)/n). Guard n < 1.
    (void)serialFraction; (void)n;
    return 0.0;
}

double amdahlSerialFraction(double measuredSpeedup, int n) {
    // TODO(2): solve S = 1/(s + (1-s)/n) for s.
    //
    //   1/S = s + (1-s)/n
    //   1/S = s + 1/n - s/n
    //   1/S - 1/n = s(1 - 1/n)
    //   s = (1/S - 1/n) / (1 - 1/n)
    //
    // Return 1.0 when n < 2 (one processor tells you nothing about scaling),
    // and clamp the answer into [0, 1].
    (void)measuredSpeedup; (void)n;
    return 1.0;
}

double maxSpeedup(double serialFraction) {
    // TODO(3): 1/s, or std::numeric_limits<double>::infinity() when s == 0.
    (void)serialFraction;
    return 0.0;
}

double efficiency(double t1, double tn, int n) {
    // TODO(4): (t1/tn)/n, guarding tn <= 0 and n < 1.
    (void)t1; (void)tn; (void)n;
    return 0.0;
}

Timing chooseBaseline(const std::vector<Timing>& candidates) {
    // TODO(5): return the SERIAL entry (threads <= 1) with the smallest ms.
    //          Ignore every parallel entry, even a faster one -- dividing by a
    //          parallel time is not a speedup, and dividing by the ORIGINAL
    //          serial code credits threading for serial optimisation work.
    //          If there is no serial entry, return a default-constructed Timing.
    (void)candidates;
    return {};
}

}  // namespace ex07
