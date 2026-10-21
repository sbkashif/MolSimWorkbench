// How long does it take just to start and finish T threads that do nothing?
//
//   clang++ -std=c++17 -O3 thread_cost.cpp -o thread_cost
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

int main() {
    for (int T : {1, 2, 4, 8, 10}) {
        double best = 1e300;
        for (int rep = 0; rep < 200; ++rep) {
            auto t0 = std::chrono::steady_clock::now();
            std::vector<std::thread> pool;
            for (int t = 0; t < T; ++t) pool.emplace_back([] {});
            for (auto& th : pool) th.join();
            auto t1 = std::chrono::steady_clock::now();
            best = std::min(best, std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
        std::printf("%2d threads: %6.1f microseconds to start and join\n", T, best);
    }
    return 0;
}
