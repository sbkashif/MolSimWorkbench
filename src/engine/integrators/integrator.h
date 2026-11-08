#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <memory>
#include <string>
#include "../md_state.h"

// Base class for every way of moving the atoms forward one time step.
//
// This is the second socket in the engine, next to Potential. MolecularDynamics
// owns the loop; each step it calls step() on whichever Integrator is plugged
// in. On entry, state.forces and state.potentialEnergy belong to the current
// positions; on return, positions, velocities and forces all belong to t + dt.
class Integrator {
public:
    virtual ~Integrator() = default;

    // Advance positions and velocities by one time step dt.
    virtual void step(MDState& state, double dt) = 0;

    // Name used in YAML and in Python, e.g. "velocity_verlet".
    virtual std::string getName() const = 0;
};

// Builds an integrator from its YAML name: "velocity_verlet" or "leapfrog".
// Throws std::invalid_argument for any other name.
std::shared_ptr<Integrator> makeIntegrator(const std::string& name);

#endif // INTEGRATOR_H
