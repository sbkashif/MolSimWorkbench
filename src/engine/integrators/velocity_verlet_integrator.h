#ifndef VELOCITY_VERLET_INTEGRATOR_H
#define VELOCITY_VERLET_INTEGRATOR_H

#include "integrator.h"

// Velocity Verlet, the engine's default integrator.
//
//   r(t+dt)   = r(t) + v(t) dt + 1/2 f(t) dt^2
//   v(t+dt/2) = v(t) + 1/2 f(t) dt
//   f(t+dt)   = forces at r(t+dt)
//   v(t+dt)   = v(t+dt/2) + 1/2 f(t+dt) dt
//
// One force evaluation per step; time-reversible and symplectic, so the total
// energy wobbles but does not drift (error O(dt^2)).
class VelocityVerletIntegrator : public Integrator {
public:
    void step(MDState& state, double dt) override;
    std::string getName() const override { return "velocity_verlet"; }
};

#endif // VELOCITY_VERLET_INTEGRATOR_H
