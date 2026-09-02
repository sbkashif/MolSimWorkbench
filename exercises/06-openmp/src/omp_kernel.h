#ifndef EX06_OMP_KERNEL_H
#define EX06_OMP_KERNEL_H

#include <cstddef>
#include <vector>

namespace ex06 {

enum class Schedule { Static, StaticChunk, Dynamic, Guided };

// Gather kernel under #pragma omp parallel for, with reduction(+:) energy.
double ompGather(const std::vector<double>& x,
                 const std::vector<double>& y,
                 const std::vector<double>& z,
                 double eps4, double sigma2,
                 double bx, double by, double bz, double cutoff,
                 std::vector<double>& fx, std::vector<double>& fy,
                 std::vector<double>& fz, int numThreads, Schedule s);

// Same, but with a hand-rolled fixed-order energy reduction so that repeated
// runs are BITWISE identical. reduction(+:) does not guarantee that.
double deterministicGather(const std::vector<double>& x,
                           const std::vector<double>& y,
                           const std::vector<double>& z,
                           double eps4, double sigma2,
                           double bx, double by, double bz, double cutoff,
                           std::vector<double>& fx, std::vector<double>& fy,
                           std::vector<double>& fz, int numThreads);

bool openMPEnabled();
int maxThreads();

}  // namespace ex06
#endif
