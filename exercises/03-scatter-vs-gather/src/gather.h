#ifndef EX03_GATHER_H
#define EX03_GATHER_H

#include <cstddef>
#include <vector>

namespace ex03 {

// Gather formulation: atom i sums over ALL j != i and writes only f[i].
// Returns the total potential energy, each pair counted ONCE.
double computeGather(const std::vector<double>& x,
                     const std::vector<double>& y,
                     const std::vector<double>& z,
                     double eps4, double sigma2,
                     double bx, double by, double bz,
                     double cutoff,
                     std::vector<double>& fx,
                     std::vector<double>& fy,
                     std::vector<double>& fz);

// Pair evaluations done by gather divided by those done by the triangular loop.
// Triangular does n(n-1)/2; gather does n(n-1). Return the exact ratio.
double gatherArithmeticRatio(std::size_t n);

}  // namespace ex03
#endif
