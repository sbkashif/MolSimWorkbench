#include "lj_potential.h"
#include <cmath>
#include <iostream>

LJPotential::LJPotential() {
    // Constructor
}

LJPotential::~LJPotential() {
    // Destructor
}

void LJPotential::addAtomParameters(const std::string& atomType, double epsilon, double sigma) {
    atomParameters[atomType] = {epsilon, sigma};
}

double LJPotential::calculateEnergy(const Geometry& geometry) {
    double totalEnergy = 0.0;
    
    // Get atom types and positions
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& boxDim = geometry.getBoxDimensions();
    
    // Loop through all atom pairs
    for (size_t i = 0; i < atoms.size(); ++i) {
        for (size_t j = i + 1; j < atoms.size(); ++j) {
            // Calculate distance between atoms with minimum image convention (for PBC)
            double dx = positions[j][0] - positions[i][0];
            double dy = positions[j][1] - positions[i][1];
            double dz = positions[j][2] - positions[i][2];
            
            // Apply minimum image convention if box dimensions are set
            if (boxDim[0] > 0.0) {
                dx = dx - boxDim[0] * std::round(dx / boxDim[0]);
            }
            if (boxDim[1] > 0.0) {
                dy = dy - boxDim[1] * std::round(dy / boxDim[1]);
            }
            if (boxDim[2] > 0.0) {
                dz = dz - boxDim[2] * std::round(dz / boxDim[2]);
            }
            
            double distanceSquared = dx*dx + dy*dy + dz*dz;
            double distance = std::sqrt(distanceSquared);
            
            // Skip if beyond cutoff or too close (numerical safety)
            if (distance > cutoff || distance < 1e-10) continue;
            
            // Get mixed LJ parameters for this pair
            auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
            
            // Calculate pair energy
            totalEnergy += calculatePairEnergy(epsilon, sigma, distance);
        }
    }
    
    return totalEnergy;
}

void LJPotential::calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) {
    // Initialize forces if not already done
    if (forces.size() != geometry.getAtoms().size()) {
        forces.resize(geometry.getAtoms().size(), {0.0, 0.0, 0.0});
    } else {
        // Zero out forces
        for (auto& force : forces) {
            force = {0.0, 0.0, 0.0};
        }
    }
    
    // Get atom types and positions
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& boxDim = geometry.getBoxDimensions();
    
    // Loop through all atom pairs
    for (size_t i = 0; i < atoms.size(); ++i) {
        for (size_t j = i + 1; j < atoms.size(); ++j) {
            // Calculate distance vector between atoms with minimum image convention
            double dx = positions[j][0] - positions[i][0];
            double dy = positions[j][1] - positions[i][1];
            double dz = positions[j][2] - positions[i][2];
            
            // Apply minimum image convention if box dimensions are set
            if (boxDim[0] > 0.0) {
                dx = dx - boxDim[0] * std::round(dx / boxDim[0]);
            }
            if (boxDim[1] > 0.0) {
                dy = dy - boxDim[1] * std::round(dy / boxDim[1]);
            }
            if (boxDim[2] > 0.0) {
                dz = dz - boxDim[2] * std::round(dz / boxDim[2]);
            }
            
            double distanceSquared = dx*dx + dy*dy + dz*dz;
            double distance = std::sqrt(distanceSquared);
            
            // Skip if beyond cutoff or too close (numerical safety)
            if (distance > cutoff || distance < 1e-10) continue;
            
            // Get mixed LJ parameters for this pair
            auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
            
            // Direction vector (normalized)
            std::array<double, 3> direction = {dx/distance, dy/distance, dz/distance};
            
            // Calculate and add forces
            calculatePairForce(epsilon, sigma, distance, direction, forces[i], forces[j]);
        }
    }
}

std::pair<double, double> LJPotential::getMixedParameters(const std::string& type1, const std::string& type2) {
    // Check if both atom types have parameters
    if (atomParameters.find(type1) == atomParameters.end() ||
        atomParameters.find(type2) == atomParameters.end()) {
        std::cerr << "Warning: Missing parameters for atom types: " << type1 << " or " << type2 << std::endl;
        return {0.0, 0.0};  // Return zero parameters if missing
    }
    
    const auto& params1 = atomParameters[type1];
    const auto& params2 = atomParameters[type2];
    
    double mixedEpsilon, mixedSigma;
    
    // Apply mixing rule
    if (mixingRule == "geometric") {
        mixedEpsilon = std::sqrt(params1.epsilon * params2.epsilon);
        mixedSigma = std::sqrt(params1.sigma * params2.sigma);
    } else if (mixingRule == "arithmetic") {
        mixedEpsilon = 0.5 * (params1.epsilon + params2.epsilon);
        mixedSigma = 0.5 * (params1.sigma + params2.sigma);
    } else {
        // Default to no mixing (for custom parameters)
        mixedEpsilon = 0.0;
        mixedSigma = 0.0;
    }
    
    return {mixedEpsilon, mixedSigma};
}

double LJPotential::calculatePairEnergy(double epsilon, double sigma, double distance) {
    // Avoid division by zero
    if (distance <= 0.0) return 0.0;
    
    // Calculate LJ terms
    double sigma_r = sigma / distance;
    double sigma_r6 = std::pow(sigma_r, 6);
    double sigma_r12 = sigma_r6 * sigma_r6;
    
    // Standard 12-6 LJ potential: 4ε[(σ/r)^12 - (σ/r)^6]
    return 4.0 * epsilon * (sigma_r12 - sigma_r6);
}

void LJPotential::calculatePairForce(double epsilon, double sigma, double distance,
                                    const std::array<double, 3>& direction,
                                    std::array<double, 3>& force1,
                                    std::array<double, 3>& force2) {
    // Avoid division by zero
    if (distance <= 0.0) return;
    
    // Calculate LJ force magnitude
    double sigma_r = sigma / distance;
    double sigma_r6 = std::pow(sigma_r, 6);
    double sigma_r12 = sigma_r6 * sigma_r6;
    
    // Force magnitude: -dE/dr = 24ε/r * [(2σ^12/r^12) - (σ^6/r^6)]
    double forceMagnitude = 24.0 * epsilon * (2.0 * sigma_r12 - sigma_r6) / distance;
    
    // Apply force in opposite directions
    for (int i = 0; i < 3; ++i) {
        force1[i] -= forceMagnitude * direction[i];
        force2[i] += forceMagnitude * direction[i];
    }
}