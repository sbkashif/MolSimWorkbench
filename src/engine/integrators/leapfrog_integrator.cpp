#include "leapfrog_integrator.h"

void LeapfrogIntegrator::step(MDState& state, double dt) {
    Geometry& geometry = state.geometry;
    auto& velocities = state.velocities;
    auto& forces = state.forces;
    const double halfDt = 0.5 * dt;

    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        // Kick: v(t+dt/2) = v(t) + 0.5*f(t)*dt
        for (int d = 0; d < 3; ++d) {
            velocities[i][d] += halfDt * forces[i][d];
        }

        // Drift: r(t+dt) = r(t) + v(t+dt/2)*dt
        std::array<double, 3> pos = geometry.getCoordinates(i);
        for (int d = 0; d < 3; ++d) {
            pos[d] += dt * velocities[i][d];
        }
        geometry.setCoordinates(i, pos);
    }

    state.wrapPositions();
    state.computeForces();

    // Kick: v(t+dt) = v(t+dt/2) + 0.5*f(t+dt)*dt
    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        for (int d = 0; d < 3; ++d) {
            velocities[i][d] += halfDt * forces[i][d];
        }
    }
}
