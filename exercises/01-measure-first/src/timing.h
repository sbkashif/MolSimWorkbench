#ifndef EX01_TIMING_H
#define EX01_TIMING_H

#include <chrono>
#include <functional>

namespace ex01 {

// Time `f`. Perform ONE untimed warm-up call, then `reps` timed calls, and
// return the MINIMUM elapsed milliseconds. reps < 1 is treated as 1.
double timeMs(const std::function<void()>& f, int reps);

// baselineMs / candidateMs; 0.0 if candidateMs <= 0.
double speedup(double baselineMs, double candidateMs);

// (t1/tN)/n, the fraction of linear scaling achieved.
// 0.0 if tN <= 0 or n < 1.
double parallelEfficiency(double t1, double tN, int n);

}  // namespace ex01
#endif
