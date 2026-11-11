#include "velocity_verlet_integrator.h"

void VelocityVerletIntegrator::step(MDState& state, double dt) {
    Geometry& geometry = state.geometry;
    auto& velocities = state.velocities;
    auto& forces = state.forces;
    const double dtSquaredHalf = 0.5 * dt * dt;

    // First half: new positions from f(t), velocities half-kicked by f(t)
    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        // r(t+dt) = r(t) + v(t)*dt + 0.5*f(t)*dt^2
        std::array<double, 3> pos = geometry.getCoordinates(i);
        std::array<double, 3> newPos = {
            pos[0] + velocities[i][0] * dt + forces[i][0] * dtSquaredHalf,
            pos[1] + velocities[i][1] * dt + forces[i][1] * dtSquaredHalf,
            pos[2] + velocities[i][2] * dt + forces[i][2] * dtSquaredHalf
        };
        geometry.setCoordinates(i, newPos);

        // v(t+dt/2) = v(t) + 0.5*f(t)*dt
        velocities[i][0] += forces[i][0] * 0.5 * dt;
        velocities[i][1] += forces[i][1] * 0.5 * dt;
        velocities[i][2] += forces[i][2] * 0.5 * dt;
    }

    state.wrapPositions();

    // f(t+dt) at the new positions
    state.computeForces();

    // Second half: v(t+dt) = v(t+dt/2) + 0.5*f(t+dt)*dt
    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        velocities[i][0] += forces[i][0] * 0.5 * dt;
        velocities[i][1] += forces[i][1] * 0.5 * dt;
        velocities[i][2] += forces[i][2] * 0.5 * dt;
    }
}
