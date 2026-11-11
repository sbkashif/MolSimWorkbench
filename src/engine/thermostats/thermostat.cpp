#include "thermostat.h"
#include <stdexcept>
#include "velocity_rescale_thermostat.h"
#include "langevin_thermostat.h"

std::shared_ptr<Thermostat> makeThermostat(const std::string& name,
                                           double tau,
                                           double friction,
                                           unsigned int seed) {
    if (name == "velocity_rescale") {
        return std::make_shared<VelocityRescaleThermostat>(tau);
    }
    if (name == "berendsen") {
        if (tau <= 0.0) {
            throw std::invalid_argument("Berendsen thermostat needs tau > 0");
        }
        return std::make_shared<VelocityRescaleThermostat>(tau);
    }
    if (name == "langevin") {
        return std::make_shared<LangevinThermostat>(friction, seed);
    }
    if (name == "none") {
        return nullptr;
    }
    throw std::invalid_argument("Unknown thermostat '" + name +
                                "' (expected \"velocity_rescale\", \"berendsen\", "
                                "\"langevin\" or \"none\")");
}
