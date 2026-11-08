#ifndef LEAPFROG_INTEGRATOR_H
#define LEAPFROG_INTEGRATOR_H

#include "integrator.h"

// Leapfrog, written as kick-drift-kick.
//
//   v(t+dt/2) = v(t) + 1/2 f(t) dt         kick
//   r(t+dt)   = r(t) + v(t+dt/2) dt        drift, using the half-step velocity
//   f(t+dt)   = forces at r(t+dt)
//   v(t+dt)   = v(t+dt/2) + 1/2 f(t+dt) dt kick
//
// Velocities and positions "leap" over each other: positions move with the
// velocity from half a step ahead. The two half kicks bring the velocity back
// onto the full step so kinetic and potential energy belong to the same time.
//
// In exact arithmetic this gives the same trajectory as velocity Verlet (the
// drift expands to r + v dt + 1/2 f dt^2). It is a separate plug because it
// is a different recipe: the position update reads the half-kicked velocity
// instead of v(t) and f(t), so rounding differs and the two only agree to
// floating-point precision, not bit for bit.
class LeapfrogIntegrator : public Integrator {
public:
    void step(MDState& state, double dt) override;
    std::string getName() const override { return "leapfrog"; }
};

#endif // LEAPFROG_INTEGRATOR_H
