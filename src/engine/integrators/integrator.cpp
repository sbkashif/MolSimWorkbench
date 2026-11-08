#include "integrator.h"
#include <stdexcept>
#include "velocity_verlet_integrator.h"
#include "leapfrog_integrator.h"

std::shared_ptr<Integrator> makeIntegrator(const std::string& name) {
    if (name == "velocity_verlet") {
        return std::make_shared<VelocityVerletIntegrator>();
    }
    if (name == "leapfrog") {
        return std::make_shared<LeapfrogIntegrator>();
    }
    throw std::invalid_argument("Unknown integrator '" + name +
                                "' (expected \"velocity_verlet\" or \"leapfrog\")");
}
