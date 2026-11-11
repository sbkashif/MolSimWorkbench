#ifndef TIMING_H
#define TIMING_H

#include <algorithm>
#include <chrono>
#include <limits>

// Run f() once to warm up, then `reps` more times; return the fastest run in milliseconds.
// F is a blank the compiler fills in with the type of whatever you pass (usually a lambda).
template <typename F>
double timeMs(F&& f, int reps) {
    f();
    double best = std::numeric_limits<double>::max();
    for (int r = 0; r < reps; ++r) {
        const auto start = std::chrono::steady_clock::now();
        f();
        const auto stop = std::chrono::steady_clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(stop - start).count());
    }
    return best;
}

#endif
