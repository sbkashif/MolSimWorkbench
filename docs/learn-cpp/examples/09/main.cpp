// mini_lj, chapter 9: time the energy loop with a lambda, then split it across threads.
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "lattice.h"
#include "lj_potential.h"
#include "timing.h"

int main(int argc, char* argv[]) {
    const std::size_t numAtoms = (argc >= 2) ? std::stoul(argv[1]) : 4096;
    const int numThreads = (argc >= 3) ? std::stoi(argv[2]) : 2;
    const Geometry geometry = makeCubicLattice(numAtoms, 0.8, 42);
    const std::size_t n = geometry.getNumberOfAtoms();

    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);

    // --- one thread
    double serialEnergy = 0.0;
    const double serialMs = timeMs([&]() { serialEnergy = lj.calculateEnergy(geometry); }, 5);

    // --- numThreads threads, each taking an equal slice of i
    std::vector<double> partial(numThreads, 0.0);
    auto runThreads = [&]() {
        std::vector<std::thread> pool;
        for (int t = 0; t < numThreads; ++t) {
            const std::size_t i0 = n * t / numThreads;
            const std::size_t i1 = n * (t + 1) / numThreads;
            // i0, i1, t are copied into the lambda; partial and lj are shared.
            pool.emplace_back([&lj, &geometry, &partial, i0, i1, t]() {
                partial[t] = lj.calculateEnergyRange(geometry, i0, i1);
            });
        }
        for (std::thread& th : pool) th.join();
    };
    const double threadedMs = timeMs(runThreads, 5);
    double threadedEnergy = 0.0;
    for (double p : partial) threadedEnergy += p;

    std::cout.precision(12);
    std::cout << "N = " << n << ", threads = " << numThreads << std::endl;
    std::cout << "serial   E = " << serialEnergy << "   " << serialMs << " ms" << std::endl;
    std::cout << "threaded E = " << threadedEnergy << "   " << threadedMs << " ms" << std::endl;
    std::cout.precision(3);
    std::cout << "speedup  = " << serialMs / threadedMs << "x" << std::endl;

    // Why not numThreads x? Count the pairs each slice owns: row i has (n - 1 - i) partners.
    for (int t = 0; t < numThreads; ++t) {
        const std::size_t i0 = n * t / numThreads, i1 = n * (t + 1) / numThreads;
        std::size_t pairs = 0;
        for (std::size_t i = i0; i < i1; ++i) pairs += n - 1 - i;
        std::cout << "thread " << t << ": atoms [" << i0 << ", " << i1 << ")  pairs = " << pairs << std::endl;
    }
    return 0;
}
