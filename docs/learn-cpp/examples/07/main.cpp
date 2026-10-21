// mini_lj, chapter 7: two different potentials behind one interface.
#include <iostream>
#include <string>
#include <vector>

#include "lattice.h"
#include "lj_potential.h"
#include "tether.h"

int main(int argc, char* argv[]) {
    const std::size_t numAtoms = (argc >= 2) ? std::stoul(argv[1]) : 125;
    const Geometry geometry = makeCubicLattice(numAtoms, 0.8, 42);

    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);
    HarmonicTether tether(0.1);

    // One list, two different classes. The loop below only knows "Potential".
    std::vector<Potential*> potentials = {&lj, &tether};

    std::cout.precision(12);
    double total = 0.0;
    for (Potential* p : potentials) {
        const double e = p->calculateEnergy(geometry);   // which function runs is decided at run time
        std::cout << p->getName() << ": " << e << std::endl;
        total += e;
    }
    std::cout << "total: " << total << std::endl;
    return 0;
}
