#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <memory>
#include <string>
#include "../md_state.h"

// Base class for every way of holding the temperature.
//
// The third socket in the engine, next to Potential and Integrator.
// MolecularDynamics calls beforeStep() and afterStep() around each
// integrator step while the temperature is being held (the whole run, or
// equilibration only when production runs NVE). A thermostat only changes
// velocities; positions and forces stay the integrator's business.
class Thermostat {
public:
    virtual ~Thermostat() = default;

    // Called just before the integrator step. Default: nothing.
    virtual void beforeStep(MDState& state, double dt, double targetTemperature) {
        (void)state; (void)dt; (void)targetTemperature;
    }

    // Called just after the integrator step.
    virtual void afterStep(MDState& state, double dt, double targetTemperature) = 0;

    // Name used in YAML and in Python, e.g. "velocity_rescale".
    virtual std::string getName() const = 0;
};

// Builds a thermostat from its YAML name.
//   "velocity_rescale"  rescale to the target every step (tau ignored unless > 0)
//   "berendsen"         same class, relaxes with time constant tau (tau > 0 required)
//   "langevin"          friction + random kicks; `friction` is gamma, `seed` its RNG
//   "none"              returns nullptr: no temperature control (NVE)
// Throws std::invalid_argument for any other name or a bad parameter.
std::shared_ptr<Thermostat> makeThermostat(const std::string& name,
                                           double tau,
                                           double friction,
                                           unsigned int seed);

#endif // THERMOSTAT_H
