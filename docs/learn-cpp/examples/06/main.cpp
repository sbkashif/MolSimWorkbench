// mini_lj, chapter 6: the data and the loop each move into a class.
#include <iostream>
#include <string>
#include <vector>

#include "lattice.h"
#include "lj_potential.h"

int main(int argc, char* argv[]) {
    const std::size_t numAtoms = (argc >= 2) ? std::stoul(argv[1]) : 125;

    const Geometry geometry = makeCubicLattice(numAtoms, 0.8, 42);

    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);

    std::vector<Vec3> forces;
    lj.calculateForces(geometry, forces);

    std::cout.precision(12);
    std::cout << lj.getName() << ", N = " << geometry.getNumberOfAtoms() << std::endl;
    std::cout << "E = " << lj.calculateEnergy(geometry) << std::endl;
    std::cout << "force on atom 0 = (" << forces[0][0] << ", " << forces[0][1] << ", " << forces[0][2] << ")" << std::endl;
    return 0;
}
