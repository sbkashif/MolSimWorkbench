#ifndef EX04_POOL_H
#define EX04_POOL_H

#include <cstddef>
#include <utility>
#include <vector>

namespace ex04 {

// Half-open range [begin, end) of indices belonging to thread t of numThreads,
// splitting [0, n). The first (n % numThreads) threads get one extra index, so
// no thread is ever more than one item behind another and nothing is dropped.
std::pair<std::size_t, std::size_t> partitionRange(std::size_t n,
                                                   int numThreads,
                                                   int t);

// Gather kernel across numThreads std::threads. Returns total potential energy.
// Must be deterministic: same inputs -> bitwise same outputs, every run, for
// any thread count.
double parallelGather(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      double eps4, double sigma2,
                      double bx, double by, double bz,
                      double cutoff,
                      std::vector<double>& fx,
                      std::vector<double>& fy,
                      std::vector<double>& fz,
                      int numThreads);

}  // namespace ex04
#endif
