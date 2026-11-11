// mini_lj, chapter 8: objects created inside a function outlive it, because shared_ptr owns them.
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "force_calculator.h"
#include "lattice.h"
#include "lj_potential.h"
#include "tether.h"

// Builds the potentials here, hands them back, and returns. The objects must survive that return.
std::shared_ptr<ForceCalculator> buildForceCalculator() {
    auto lj = std::make_shared<LJPotential>();
    lj->addAtomParameters("LJ", 1.0, 1.0);
    lj->setCutoff(2.5);

    auto forceCalculator = std::make_shared<ForceCalculator>();
    forceCalculator->addPotential(lj);
    forceCalculator->addPotential(std::make_shared<HarmonicTether>(0.1));
    std::cout << "[leaving buildForceCalculator; lj use_count = " << lj.use_count() << "]" << std::endl;
    return forceCalculator;
}

int main(int argc, char* argv[]) {
    const std::size_t numAtoms = (argc >= 2) ? std::stoul(argv[1]) : 125;
    const Geometry geometry = makeCubicLattice(numAtoms, 0.8, 42);

    auto forceCalculator = buildForceCalculator();

    std::vector<Vec3> forces;
    forceCalculator->calculateForces(geometry, forces);

    std::cout.precision(12);
    std::cout << "total E = " << forceCalculator->calculateEnergy(geometry) << std::endl;
    std::cout << "force on atom 0 = (" << forces[0][0] << ", " << forces[0][1] << ", " << forces[0][2] << ")" << std::endl;
    std::cout << "[end of main]" << std::endl;
    return 0;
}
