#ifndef EX07_SCALING_H
#define EX07_SCALING_H

#include <string>
#include <vector>

namespace ex07 {

// S(n) = 1 / (s + (1-s)/n)
double amdahlSpeedup(double serialFraction, int n);

// Invert Amdahl: given a measured speedup on n processors, recover s.
// Clamped to [0,1]. Returns 1.0 if n < 2 (no information).
double amdahlSerialFraction(double measuredSpeedup, int n);

// S(inf) = 1/s. Returns infinity for s == 0.
double maxSpeedup(double serialFraction);

// (t1/tn)/n. 0.0 if tn <= 0 or n < 1.
double efficiency(double t1, double tn, int n);

struct Timing {
    std::string label;
    double ms = 0.0;
    int threads = 1;   // 1 == serial
};

// The honest speedup denominator: the FASTEST SERIAL timing in the list.
// Entries with threads > 1 must be ignored, however fast they are.
// Returns the label and ms; label is empty if there is no serial entry.
Timing chooseBaseline(const std::vector<Timing>& candidates);

}  // namespace ex07
#endif
