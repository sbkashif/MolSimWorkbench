// Time each thread's slice ALONE, on one thread. The threaded run cannot finish before its biggest slice.
//   c++ -std=c++17 -Wall -O2 slices.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/slices
#include <iostream>

#include "lattice.h"
#include "lj_potential.h"
#include "timing.h"

int main() {
    const Geometry geometry = makeCubicLattice(4096, 0.8, 42);
    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);

    double e = 0.0;
    std::cout << "all rows [0, 4096):     " << timeMs([&]() { e = lj.calculateEnergyRange(geometry, 0, 4096); }, 5) << " ms" << std::endl;
    std::cout << "thread 0 [0, 2048):     " << timeMs([&]() { e = lj.calculateEnergyRange(geometry, 0, 2048); }, 5) << " ms" << std::endl;
    std::cout << "thread 1 [2048, 4096):  " << timeMs([&]() { e = lj.calculateEnergyRange(geometry, 2048, 4096); }, 5) << " ms" << std::endl;
    return 0;
}
