#ifndef ENERGY_MINIMIZER_H
#define ENERGY_MINIMIZER_H

#include <vector>
#include <memory>
#include <array>
#include <iostream>
#include "../../builder/geometry.h"
#include "../force_calculator.h"

// Structure to hold minimization parameters
struct MinimizationParameters {
    int maxIterations = 1000;       // Maximum number of iterations
    double tolerance = 1.0e-6;      // Convergence tolerance for energy
    double forceTolerance = 1.0e-4; // Convergence tolerance for forces
    double initialStepSize = 0.01;  // Initial step size
    double maxStepSize = 0.2;       // Maximum step size
    bool adaptiveStepSize = true;   // Whether to use adaptive step size
};

// Base class for energy minimization algorithms
class EnergyMinimizer {
public:
    EnergyMinimizer(const MinimizationParameters& params = MinimizationParameters())
        : params(params), iterationCount(0), energy(0.0), converged(false) {}
    
    virtual ~EnergyMinimizer() = default;
    
    // Initialize the minimizer with geometry and force calculator
    virtual void initialize(Geometry& geom, std::shared_ptr<ForceCalculator> forceCalc) {
        geometry = &geom;
        forceCalculator = forceCalc;
        iterationCount = 0;
        converged = false;
        
        // Initialize forces vector
        size_t numAtoms = geometry->getNumberOfAtoms();
        forces.resize(numAtoms, {0.0, 0.0, 0.0});
        
        // Calculate initial energy and forces
        energy = forceCalculator->calculateEnergy(*geometry);
        forceCalculator->calculateForces(*geometry, forces);
        
        std::cout << "Initial energy: " << energy << std::endl;
    }
    
    // Perform a single minimization step
    virtual void step() = 0;
    
    // Run the minimization until convergence or max iterations
    virtual bool minimize() {
        if (!geometry || !forceCalculator) {
            std::cerr << "Error: Minimizer not initialized properly" << std::endl;
            return false;
        }
        
        for (iterationCount = 0; iterationCount < params.maxIterations; ++iterationCount) {
            // Store previous energy for comparison
            double previousEnergy = energy;
            
            // Take a minimization step
            step();
            
            // Calculate new energy and forces
            energy = forceCalculator->calculateEnergy(*geometry);
            forceCalculator->calculateForces(*geometry, forces);
            
            // Check for convergence
            if (checkConvergence(previousEnergy)) {
                converged = true;
                std::cout << "Minimization converged after " << iterationCount + 1 
                          << " iterations with energy " << energy << std::endl;
                return true;
            }
            
            // Print progress every 10 iterations
            if ((iterationCount + 1) % 10 == 0) {
                std::cout << "Iteration " << iterationCount + 1 
                          << ", Energy: " << energy << std::endl;
            }
        }
        
        std::cout << "Minimization did not converge after " << params.maxIterations 
                  << " iterations. Final energy: " << energy << std::endl;
        return false;
    }
    
    // Get current minimization state
    int getIterationCount() const { return iterationCount; }
    double getEnergy() const { return energy; }
    bool hasConverged() const { return converged; }
    const std::vector<std::array<double, 3>>& getForces() const { return forces; }
    
    // Set/get parameters
    void setParameters(const MinimizationParameters& newParams) { params = newParams; }
    const MinimizationParameters& getParameters() const { return params; }

protected:
    // Common member variables
    MinimizationParameters params;
    Geometry* geometry;
    std::shared_ptr<ForceCalculator> forceCalculator;
    std::vector<std::array<double, 3>> forces;
    int iterationCount;
    double energy;
    bool converged;
    
    // Check if minimization has converged
    virtual bool checkConvergence(double previousEnergy) {
        // Check energy difference convergence
        double energyDiff = std::abs(energy - previousEnergy);
        if (energyDiff < params.tolerance) {
            return true;
        }
        
        // Check max force convergence
        double maxForce = 0.0;
        for (const auto& force : forces) {
            for (int d = 0; d < 3; ++d) {
                maxForce = std::max(maxForce, std::abs(force[d]));
            }
        }
        
        return maxForce < params.forceTolerance;
    }
};

#endif // ENERGY_MINIMIZER_H