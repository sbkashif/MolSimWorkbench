#include "velocity_rescale_thermostat.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

VelocityRescaleThermostat::VelocityRescaleThermostat(double tau) : tau(tau) {
    if (tau < 0.0) {
        throw std::invalid_argument("Thermostat tau must be >= 0");
    }
}

void VelocityRescaleThermostat::afterStep(MDState& state, double dt, double targetTemperature) {
    // Current temperature (kB = 1, mass = 1)
    const double currentTemp = state.temperature();
    if (currentTemp <= 1e-10) {
        return;  // Nothing to scale; avoids dividing by zero
    }

    double scaleFactor;
    if (tau > 0.0) {
        // Berendsen: relax towards the target with time constant tau. Clamp at
        // zero so a huge dt/tau cannot ask for the square root of a negative.
        const double lambdaSquared = 1.0 + (dt / tau) * (targetTemperature / currentTemp - 1.0);
        scaleFactor = std::sqrt(std::max(0.0, lambdaSquared));
    } else {
        // Rescale exactly onto the target
        scaleFactor = std::sqrt(targetTemperature / currentTemp);
    }

    for (auto& v : state.velocities) {
        v[0] *= scaleFactor;
        v[1] *= scaleFactor;
        v[2] *= scaleFactor;
    }
}
