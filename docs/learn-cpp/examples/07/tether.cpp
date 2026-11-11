#include "tether.h"

double HarmonicTether::calculateEnergy(const Geometry& geometry) {
    double energy = 0.0;
    for (const Vec3& p : geometry.getPositions())
        energy += 0.5 * k * (p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
    return energy;
}

void HarmonicTether::calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) {
    const auto& positions = geometry.getPositions();
    forces.assign(positions.size(), Vec3{0.0, 0.0, 0.0});
    for (std::size_t i = 0; i < positions.size(); ++i)
        for (int k3 = 0; k3 < 3; ++k3) forces[i][k3] = -k * positions[i][k3];
}
