#include "force_calculator.h"

double ForceCalculator::calculateEnergy(const Geometry& geometry) {
    double total = 0.0;
    for (const auto& potential : potentials) total += potential->calculateEnergy(geometry);
    return total;
}

void ForceCalculator::calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) {
    forces.assign(geometry.getNumberOfAtoms(), Vec3{0.0, 0.0, 0.0});
    std::vector<Vec3> part;
    for (const auto& potential : potentials) {
        potential->calculateForces(geometry, part);
        for (std::size_t i = 0; i < forces.size(); ++i)
            for (int k = 0; k < 3; ++k) forces[i][k] += part[i][k];
    }
}
