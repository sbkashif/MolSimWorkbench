#ifndef LANGEVIN_THERMOSTAT_H
#define LANGEVIN_THERMOSTAT_H

#include <random>
#include "thermostat.h"

// Langevin thermostat: every atom feels a friction -gamma v and a random kick,
// as if it sat in a bath of invisible solvent particles at temperature T0.
//
// The friction-plus-noise part has an exact solution over a time h, the
// Ornstein-Uhlenbeck update
//
//   v <- c v + sqrt((1 - c^2) T0) xi,   c = exp(-gamma h),  xi ~ N(0, 1)
//
// (mass 1, kB = 1). It runs for h = dt/2 before the integrator step and
// h = dt/2 after it, so a velocity Verlet step sits in the middle; this
// symmetric split is the Bussi-Parrinello Langevin integrator. Unlike
// rescaling, it samples the canonical (NVT) ensemble: the kinetic energy
// fluctuates the way it should at T0.
//
// gamma sets how strongly the bath grips: small gamma barely disturbs the
// dynamics, large gamma pins the temperature quickly but makes motion
// diffusive. Total momentum is not conserved.
class LangevinThermostat : public Thermostat {
public:
    // seed 0 draws a seed from std::random_device (non-reproducible run).
    explicit LangevinThermostat(double friction = 1.0, unsigned int seed = 0);

    void beforeStep(MDState& state, double dt, double targetTemperature) override;
    void afterStep(MDState& state, double dt, double targetTemperature) override;
    std::string getName() const override { return "langevin"; }

    double getFriction() const { return friction; }

private:
    void ornsteinUhlenbeck(MDState& state, double h, double targetTemperature);

    double friction;  // gamma, in 1/time (reduced units)
    std::mt19937 rng;
    std::normal_distribution<double> gauss{0.0, 1.0};
};

#endif // LANGEVIN_THERMOSTAT_H
