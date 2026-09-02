#ifndef EX02_REFACTOR_H
#define EX02_REFACTOR_H

#include <cstddef>
#include <string>
#include <vector>

namespace ex02 {

struct MixingTable {
    std::vector<double> eps4;    // ntypes*ntypes, row-major, holds 4*epsilon_ab
    std::vector<double> sigma2;  // ntypes*ntypes, row-major, holds sigma_ab^2
    int ntypes = 0;
};

// Apply the mixing rule ONCE per ordered type pair and store the results flat.
// rule is "geometric" (sqrt(a*b)) or "arithmetic" ((a+b)/2); anything else
// yields zeros.
MixingTable buildMixingTable(const std::vector<double>& epsilons,
                             const std::vector<double>& sigmas,
                             const std::string& rule);

// Triangular (i<j) LJ kernel with no sqrt, no pow, and no per-pair table build.
// Zeroes fx/fy/fz, accumulates forces, and returns the total potential energy.
// A box length of 0 means "no periodicity on that axis".
double computeTriangularFast(const std::vector<double>& x,
                             const std::vector<double>& y,
                             const std::vector<double>& z,
                             const std::vector<int>& type,
                             const MixingTable& table,
                             double bx, double by, double bz,
                             double cutoff,
                             std::vector<double>& fx,
                             std::vector<double>& fy,
                             std::vector<double>& fz);

}  // namespace ex02
#endif
