#include "langevin_thermostat.h"
#include <cmath>
#include <stdexcept>

LangevinThermostat::LangevinThermostat(double friction, unsigned int seed)
    : friction(friction), rng(seed != 0 ? seed : std::random_device{}()) {
    if (friction <= 0.0) {
        throw std::invalid_argument("Langevin friction must be > 0");
    }
}

void LangevinThermostat::beforeStep(MDState& state, double dt, double targetTemperature) {
    ornsteinUhlenbeck(state, 0.5 * dt, targetTemperature);
}

void LangevinThermostat::afterStep(MDState& state, double dt, double targetTemperature) {
    ornsteinUhlenbeck(state, 0.5 * dt, targetTemperature);
}

void LangevinThermostat::ornsteinUhlenbeck(MDState& state, double h, double targetTemperature) {
    const double c = std::exp(-friction * h);
    const double noise = std::sqrt((1.0 - c * c) * targetTemperature);  // mass 1, kB 1

    for (auto& v : state.velocities) {
        for (int d = 0; d < 3; ++d) {
            v[d] = c * v[d] + noise * gauss(rng);
        }
    }
}
