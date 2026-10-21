// Chapter 7's {&lj, &tether}, moved into a function. Compiles without a warning. Broken on purpose.
//   c++ -std=c++17 -Wall -O2 dangling_potentials.cpp geometry.cpp lattice.cpp lj_potential.cpp tether.cpp -o out/dangling_potentials
#include <iostream>
#include <vector>

#include "lattice.h"
#include "lj_potential.h"
#include "tether.h"

std::vector<Potential*> buildPotentials() {
    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);
    HarmonicTether tether(0.1);
    std::vector<Potential*> potentials = {&lj, &tether};
    return potentials;   // the vector of addresses is copied out...
}                        // ...and lj and tether are destroyed here

int main() {
    const Geometry geometry = makeCubicLattice(125, 0.8, 42);
    std::vector<Potential*> potentials = buildPotentials();
    std::cout << "[back in main, holding " << potentials.size() << " addresses]" << std::endl;
    for (Potential* p : potentials)
        std::cout << p->getName() << ": " << p->calculateEnergy(geometry) << std::endl;
    return 0;
}
