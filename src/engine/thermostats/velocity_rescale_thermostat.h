#ifndef VELOCITY_RESCALE_THERMOSTAT_H
#define VELOCITY_RESCALE_THERMOSTAT_H

#include "thermostat.h"

// Velocity rescaling, the engine's default thermostat, with an optional
// Berendsen time constant.
//
// After each step every velocity is multiplied by one factor
//
//   lambda = sqrt(1 + (dt / tau) (T0 / T - 1))     Berendsen, tau > 0
//   lambda = sqrt(T0 / T)                          tau = 0: hit T0 exactly
//
// tau = 0 is the limit tau -> dt taken all the way: the temperature is put
// back on the target every step. A larger tau lets it relax towards T0 over
// a time of about tau. Neither produces the canonical spread of kinetic
// energies; both are good for equilibration.
class VelocityRescaleThermostat : public Thermostat {
public:
    explicit VelocityRescaleThermostat(double tau = 0.0);

    void afterStep(MDState& state, double dt, double targetTemperature) override;
    std::string getName() const override { return tau > 0.0 ? "berendsen" : "velocity_rescale"; }

    double getTau() const { return tau; }

private:
    double tau;
};

#endif // VELOCITY_RESCALE_THERMOSTAT_H
