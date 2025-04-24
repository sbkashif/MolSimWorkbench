#ifndef VELOCITY_VERLET_INTEGRATOR_H
#define VELOCITY_VERLET_INTEGRATOR_H

#include "md_integrator.h"
#include <iostream>
#include <cmath>
#include <random>

class VelocityVerletIntegrator : public MDIntegrator {
public:
    VelocityVerletIntegrator(const MDParameters& params = MDParameters())
        : MDIntegrator(params) {}
    
    // Implement the Velocity Verlet integration step
    void step() override {
        if (!geometry || !forceCalculator) {
            std::cerr << "Error: Geometry or force calculator not initialized" << std::endl;
            return;
        }
        
        // Store old forces for velocity update
        size_t numAtoms = geometry->getNumberOfAtoms();
        std::vector<std::array<double, 3>> oldForces = forces;
        
        // Step 1: Update positions using current velocities and accelerations (half step)
        // r(t+dt) = r(t) + v(t)*dt + 0.5*a(t)*dt^2
        for (size_t i = 0; i < numAtoms; ++i) {
            std::array<double, 3> newCoords = geometry->getCoordinates(i);
            
            for (int d = 0; d < 3; ++d) {
                // Displacement = v*dt + 0.5*a*dt^2
                // a = F/m (we'll assume mass = 1.0 for simplicity)
                double acceleration = forces[i][d]; // F/m with m=1
                newCoords[d] += velocities[i][d] * params.timeStep + 
                               0.5 * acceleration * params.timeStep * params.timeStep;
            }
            
            // Update coordinates in geometry
            geometry->setCoordinates(i, newCoords);
        }
        
        // Step 2: Calculate new forces at the updated positions
        calculateForces();
        
        // Step 3: Update velocities using old and new forces (full step)
        // v(t+dt) = v(t) + 0.5*[a(t) + a(t+dt)]*dt
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                // Assume mass = 1.0 for simplicity
                double oldAcceleration = oldForces[i][d]; // F/m with m=1
                double newAcceleration = forces[i][d];    // F/m with m=1
                
                // Update velocity
                velocities[i][d] += 0.5 * (oldAcceleration + newAcceleration) * params.timeStep;
            }
        }
        
        // Apply thermostat if needed
        if (params.useConstantTemperature) {
            applyThermostat();
        }
        
        // Increment step counter
        currentStep++;
    }
};

// Implementation of velocity initialization with Maxwell-Boltzmann distribution
inline void MDIntegrator::initializeVelocities(double temperature) {
    if (temperature <= 0.0) {
        temperature = params.temperature;
    }
    
    // Set up random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> distribution(0.0, 1.0);
    
    const double kB = 8.3144621e-3; // Boltzmann constant in kJ/(mol*K)
    size_t numAtoms = geometry->getNumberOfAtoms();
    
    // Initialize velocities from a Gaussian distribution
    for (size_t i = 0; i < numAtoms; ++i) {
        // For simplicity, assume all atoms have the same mass (1.0)
        double mass = 1.0;
        double sigma = std::sqrt(kB * temperature / mass);
        
        for (int d = 0; d < 3; ++d) {
            velocities[i][d] = distribution(gen) * sigma;
        }
    }
    
    // Calculate center of mass velocity
    std::array<double, 3> vCOM = {0.0, 0.0, 0.0};
    double totalMass = 0.0;
    
    for (size_t i = 0; i < numAtoms; ++i) {
        double mass = 1.0; // Again, using mass = 1.0 for simplicity
        totalMass += mass;
        
        for (int d = 0; d < 3; ++d) {
            vCOM[d] += mass * velocities[i][d];
        }
    }
    
    // Normalize by total mass
    for (int d = 0; d < 3; ++d) {
        vCOM[d] /= totalMass;
    }
    
    // Remove center of mass motion
    for (size_t i = 0; i < numAtoms; ++i) {
        for (int d = 0; d < 3; ++d) {
            velocities[i][d] -= vCOM[d];
        }
    }
    
    // Scale velocities to exactly match target temperature
    double kineticEnergy = 0.0;
    
    for (size_t i = 0; i < numAtoms; ++i) {
        double mass = 1.0;
        for (int d = 0; d < 3; ++d) {
            kineticEnergy += 0.5 * mass * velocities[i][d] * velocities[i][d];
        }
    }
    
    // Calculate target kinetic energy: E_k = (3N-3)*k_B*T/2
    double targetKE = (3.0 * numAtoms - 3.0) * kB * temperature / 2.0;
    double scaleFactor = std::sqrt(targetKE / kineticEnergy);
    
    // Scale all velocities
    for (size_t i = 0; i < numAtoms; ++i) {
        for (int d = 0; d < 3; ++d) {
            velocities[i][d] *= scaleFactor;
        }
    }
    
    std::cout << "Initialized velocities to temperature: " << temperature << " K" << std::endl;
}

// Implementation of Berendsen thermostat
inline void MDIntegrator::applyThermostat() {
    size_t numAtoms = geometry->getNumberOfAtoms();
    
    // Calculate current kinetic energy
    double kineticEnergy = 0.0;
    
    for (size_t i = 0; i < numAtoms; ++i) {
        double mass = 1.0; // Assuming mass = 1.0 for simplicity
        for (int d = 0; d < 3; ++d) {
            kineticEnergy += 0.5 * mass * velocities[i][d] * velocities[i][d];
        }
    }
    
    // Calculate current temperature: E_k = (3N-3)*k_B*T/2
    const double kB = 8.3144621e-3; // Boltzmann constant in kJ/(mol*K)
    double currentTemp = 2.0 * kineticEnergy / ((3.0 * numAtoms - 3.0) * kB);
    
    // Calculate Berendsen scaling factor
    double lambda = std::sqrt(1.0 + (params.timeStep / params.couplingConstant) * 
                             (params.temperature / currentTemp - 1.0));
    
    // Scale velocities
    for (size_t i = 0; i < numAtoms; ++i) {
        for (int d = 0; d < 3; ++d) {
            velocities[i][d] *= lambda;
        }
    }
}

#endif // VELOCITY_VERLET_INTEGRATOR_H