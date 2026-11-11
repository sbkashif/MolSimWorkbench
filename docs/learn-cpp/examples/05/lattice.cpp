#include "lattice.h"

#include <cmath>
#include <random>

std::vector<Vec3> makeCubicLattice(std::size_t numAtoms, double density, unsigned seed, double& box) {
    box = std::cbrt(numAtoms / density);
    const int perSide = static_cast<int>(std::cbrt(numAtoms) + 0.5);
    const double spacing = box / perSide;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> jitter(-0.05 * spacing, 0.05 * spacing);

    std::vector<Vec3> positions;
    for (int i = 0; i < perSide && positions.size() < numAtoms; ++i)
        for (int j = 0; j < perSide && positions.size() < numAtoms; ++j)
            for (int k = 0; k < perSide && positions.size() < numAtoms; ++k) {
                double x = (i + 0.5) * spacing - box / 2.0 + jitter(rng);
                double y = (j + 0.5) * spacing - box / 2.0 + jitter(rng);
                double z = (k + 0.5) * spacing - box / 2.0 + jitter(rng);
                positions.push_back({x, y, z});
            }
    return positions;
}
