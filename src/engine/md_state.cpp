#include "md_state.h"

void MDState::computeForces() {
    potentialEnergy = forceCalculator.calculateForcesAndEnergy(geometry, forces);
}

void MDState::wrapPositions() {
    if (!usePBC) return;

    // Orthorhombic boxes only (90 degree angles); a triclinic box would need
    // a matrix transform. One image shift per step is enough for sane dt.
    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        std::array<double, 3> pos = geometry.getCoordinates(i);
        const double lengths[3] = {box.a, box.b, box.c};
        bool changed = false;

        for (int d = 0; d < 3; ++d) {
            if (pos[d] < 0) {
                pos[d] += lengths[d];
                changed = true;
            } else if (pos[d] >= lengths[d]) {
                pos[d] -= lengths[d];
                changed = true;
            }
        }

        if (changed) {
            geometry.setCoordinates(i, pos);
        }
    }
}

double MDState::kineticEnergy() const {
    double kineticEnergy = 0.0;
    for (const auto& v : velocities) {
        kineticEnergy += v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    }
    kineticEnergy *= 0.5;  // mass = 1 for all atoms
    return kineticEnergy;
}

double MDState::temperature() const {
    return 2.0 * kineticEnergy() / (3 * geometry.getNumberOfAtoms());
}
