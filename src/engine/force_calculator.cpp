#include "force_calculator.h"
#include <vector>
#include <memory>

ForceCalculator::ForceCalculator() {}

ForceCalculator::~ForceCalculator() {}

void ForceCalculator::addPotential(std::shared_ptr<Potential> potential) {
    potentials.push_back(potential);
}

double ForceCalculator::calculateEnergy(const Geometry& geometry) {
    double totalEnergy = 0.0;
    for (const auto& potential : potentials) {
        totalEnergy += potential->calculateEnergy(geometry);
    }
    return totalEnergy;
}

void ForceCalculator::calculateForces(const Geometry& geometry, 
                                   std::vector<std::array<double, 3>>& forces) {
    // Initialize forces to zero
    size_t numAtoms = geometry.getNumberOfAtoms();
    forces.resize(numAtoms, {0.0, 0.0, 0.0});
    for (auto& force : forces) {
        force = {0.0, 0.0, 0.0};
    }
    
    // Calculate forces from each potential
    for (const auto& potential : potentials) {
        // Create a temporary forces vector for this potential
        std::vector<std::array<double, 3>> potentialForces(numAtoms, {0.0, 0.0, 0.0});
        
        // Calculate forces for this potential
        potential->calculateForces(geometry, potentialForces);
        
        // Add to total forces
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                forces[i][d] += potentialForces[i][d];
            }
        }
    }
}

void ForceCalculator::clearPotentials() {
    potentials.clear();
}