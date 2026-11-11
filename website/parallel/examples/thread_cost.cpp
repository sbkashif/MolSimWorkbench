// How long does it take just to start and finish T threads that do nothing?
// Compiled with OpenMP, it also times an OpenMP parallel region that does
// almost nothing.
//
//   clang++ -std=c++17 -O3 thread_cost.cpp -o thread_cost
//   OMP=$(brew --prefix libomp)        # macOS; on Linux use -fopenmp alone
//   clang++ -std=c++17 -O3 -Xclang -fopenmp -I $OMP/include thread_cost.cpp \
//       -L $OMP/lib -lomp -o thread_cost
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

using Clock = std::chrono::steady_clock;

int main() {
    for (int T : {1, 2, 4, 8, 10}) {
        double best = 1e300;
        for (int rep = 0; rep < 200; ++rep) {
            auto t0 = Clock::now();
            std::vector<std::thread> pool;
            for (int t = 0; t < T; ++t) pool.emplace_back([] {});
            for (auto& th : pool) th.join();
            auto t1 = Clock::now();
            best = std::min(best, std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
        std::printf("%2d threads: %6.1f microseconds to start and join\n", T, best);
    }
#ifdef _OPENMP
    // Each thread writes one number, so the compiler cannot drop the region.
    std::vector<int> mark(16, 0);
    for (int T : {1, 2, 4, 8, 10}) {
        double best = 1e300;
        for (int rep = 0; rep < 200; ++rep) {
            auto t0 = Clock::now();
            #pragma omp parallel num_threads(T)
            mark[omp_get_thread_num()] += 1;
            auto t1 = Clock::now();
            best = std::min(best, std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
        std::printf("%2d threads: %6.1f microseconds for an OpenMP region\n", T, best);
    }
#endif
    return 0;
}
