#ifndef CONJUGATE_GRADIENT_MINIMIZER_H
#define CONJUGATE_GRADIENT_MINIMIZER_H

#include "energy_minimizer.h"
#include <cmath>

class ConjugateGradientMinimizer : public EnergyMinimizer {
public:
    ConjugateGradientMinimizer(const MinimizationParameters& params = MinimizationParameters())
        : EnergyMinimizer(params), currentStepSize(params.initialStepSize), resetCount(0) {}
    
    // Override initialization to set up CG-specific variables
    void initialize(Geometry& geom, std::shared_ptr<ForceCalculator> forceCalc) override {
        // Call base class initialization
        EnergyMinimizer::initialize(geom, forceCalc);
        
        // Initialize CG-specific variables
        size_t numAtoms = geometry->getNumberOfAtoms();
        previousForces.resize(numAtoms, {0.0, 0.0, 0.0});
        conjugateDirections.resize(numAtoms, {0.0, 0.0, 0.0});
        
        // For first iteration, conjugate direction is simply the force
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                conjugateDirections[i][d] = forces[i][d];
            }
        }
        
        // Reset the periodic counter
        resetCount = 0;
    }
    
    // Perform a single conjugate gradient step
    void step() override {
        if (!geometry || !forceCalculator) {
            std::cerr << "Error: Minimizer not initialized properly" << std::endl;
            return;
        }
        
        size_t numAtoms = geometry->getNumberOfAtoms();
        
        // Every N steps (where N is the dimensionality), reset to steepest descent
        // This helps avoid numerical drift in the conjugate directions
        if (resetCount >= numAtoms * 3) {
            resetConjugateDirections();
            resetCount = 0;
        }
        
        // Move atoms along the conjugate direction
        for (size_t i = 0; i < numAtoms; ++i) {
            std::array<double, 3> coords = geometry->getCoordinates(i);
            
            for (int d = 0; d < 3; ++d) {
                coords[d] += currentStepSize * conjugateDirections[i][d];
            }
            
            geometry->setCoordinates(i, coords);
        }
        
        // Store current forces for next iteration
        previousForces = forces;
        
        // Adjust step size if adaptive step size is enabled
        if (params.adaptiveStepSize) {
            adjustStepSize();
        }
        
        // Increment reset counter
        resetCount++;
    }
    
    // Override to calculate new conjugate directions after energy calculation
    bool minimize() override {
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
            
            // Update conjugate directions using Polak-Ribière formula
            if (iterationCount > 0 && resetCount > 0) {
                updateConjugateDirections();
            }
            
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
    
    // Get current step size
    double getStepSize() const { return currentStepSize; }
    
    // Set step size
    void setStepSize(double stepSize) { currentStepSize = stepSize; }

private:
    double currentStepSize;
    int resetCount;
    std::vector<std::array<double, 3>> previousForces;
    std::vector<std::array<double, 3>> conjugateDirections;
    
    // Reset conjugate directions to current forces (steepest descent)
    void resetConjugateDirections() {
        size_t numAtoms = geometry->getNumberOfAtoms();
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                conjugateDirections[i][d] = forces[i][d];
            }
        }
    }
    
    // Update conjugate directions using Polak-Ribière formula
    void updateConjugateDirections() {
        size_t numAtoms = geometry->getNumberOfAtoms();
        
        // Calculate beta coefficient using Polak-Ribière formula
        double numerator = 0.0;
        double denominator = 0.0;
        
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                // g_k · (g_k - g_{k-1})
                numerator += forces[i][d] * (forces[i][d] - previousForces[i][d]);
                // g_{k-1} · g_{k-1}
                denominator += previousForces[i][d] * previousForces[i][d];
            }
        }
        
        double beta = 0.0;
        if (denominator > 1.0e-10) {
            beta = numerator / denominator;
            // Ensure beta is non-negative (Fletcher-Reeves+ modification)
            beta = std::max(0.0, beta);
        }
        
        // Update conjugate directions: h_k = g_k + beta * h_{k-1}
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                conjugateDirections[i][d] = forces[i][d] + beta * conjugateDirections[i][d];
            }
        }
    }
    
    // Adjust step size based on energy change
    void adjustStepSize() {
        // Keep step size within bounds
        currentStepSize = std::min(currentStepSize, params.maxStepSize);
    }
};

#endif // CONJUGATE_GRADIENT_MINIMIZER_H