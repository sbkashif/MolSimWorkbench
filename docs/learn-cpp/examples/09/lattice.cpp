#include "lattice.h"

#include <cmath>
#include <random>

Geometry makeCubicLattice(std::size_t numAtoms, double density, unsigned seed) {
    const double box = std::cbrt(numAtoms / density);
    const int perSide = static_cast<int>(std::cbrt(numAtoms) + 0.5);
    const double spacing = box / perSide;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> jitter(-0.05 * spacing, 0.05 * spacing);

    Geometry geometry;
    geometry.setBoxDimensions({box, box, box});
    for (int i = 0; i < perSide && geometry.getNumberOfAtoms() < numAtoms; ++i)
        for (int j = 0; j < perSide && geometry.getNumberOfAtoms() < numAtoms; ++j)
            for (int k = 0; k < perSide && geometry.getNumberOfAtoms() < numAtoms; ++k) {
                double x = (i + 0.5) * spacing - box / 2.0 + jitter(rng);
                double y = (j + 0.5) * spacing - box / 2.0 + jitter(rng);
                double z = (k + 0.5) * spacing - box / 2.0 + jitter(rng);
                geometry.addAtom("LJ", {x, y, z});
            }
    return geometry;
}
