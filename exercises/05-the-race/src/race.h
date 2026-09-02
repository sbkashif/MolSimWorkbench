#ifndef EX05_RACE_H
#define EX05_RACE_H

#include <array>
#include <cstddef>
#include <vector>

namespace ex05 {

// DELIBERATELY BROKEN. Parallel triangular scatter with an unprotected f[j]
// write. Never copy this into real code.
double racyScatter(const std::vector<double>& x,
                   const std::vector<double>& y,
                   const std::vector<double>& z,
                   double eps4, double sigma2,
                   double bx, double by, double bz, double cutoff,
                   std::vector<double>& fx, std::vector<double>& fy,
                   std::vector<double>& fz, int numThreads);

// The fix: per-thread private force buffers, summed at the end.
// Keeps the triangular loop (half the arithmetic of gather) and is race-free
// because no two threads ever write the same memory.
double privateBufferScatter(const std::vector<double>& x,
                            const std::vector<double>& y,
                            const std::vector<double>& z,
                            double eps4, double sigma2,
                            double bx, double by, double bz, double cutoff,
                            std::vector<double>& fx, std::vector<double>& fy,
                            std::vector<double>& fz, int numThreads);

// Number of atoms with ANY component off by more than relTol (relative).
std::size_t countWrongAtoms(const std::vector<std::array<double, 3>>& reference,
                            const std::vector<double>& fx,
                            const std::vector<double>& fy,
                            const std::vector<double>& fz,
                            double relTol);

}  // namespace ex05
#endif
