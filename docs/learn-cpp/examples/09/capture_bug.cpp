// The chapter 9 thread loop with ONE change: [&] captures i0 and i1 by reference.
// Broken on purpose. Compile with the mini_lj files:
//   c++ -std=c++17 -Wall -O2 capture_bug.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/capture_bug
#include <iostream>
#include <thread>
#include <vector>

#include "lattice.h"
#include "lj_potential.h"

int main() {
    const Geometry geometry = makeCubicLattice(4096, 0.8, 42);
    const std::size_t n = geometry.getNumberOfAtoms();
    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);

    const int numThreads = 4;
    std::cout.precision(12);
    std::cout << "correct E = " << lj.calculateEnergy(geometry) << std::endl;
    for (int run = 0; run < 3; ++run) {
        std::vector<double> partial(numThreads, 0.0);
        std::vector<std::thread> pool;
        for (int t = 0; t < numThreads; ++t) {
            const std::size_t i0 = n * t / numThreads;
            const std::size_t i1 = n * (t + 1) / numThreads;
            pool.emplace_back([&]() {                       // BUG: i0, i1 and t by reference
                partial[t] = lj.calculateEnergyRange(geometry, i0, i1);
            });
        }   // i0, i1 (and, after the loop, t) stop existing; the threads may still be reading them
        for (std::thread& th : pool) th.join();
        double e = 0.0;
        for (double p : partial) e += p;
        std::cout << "run " << run << ":   E = " << e << std::endl;
    }
    return 0;
}
