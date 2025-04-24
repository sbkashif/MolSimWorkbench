#ifndef STEEPEST_DESCENT_MINIMIZER_H
#define STEEPEST_DESCENT_MINIMIZER_H

#include "energy_minimizer.h"
#include <cmath>

class SteepestDescentMinimizer : public EnergyMinimizer {
public:
    SteepestDescentMinimizer(const MinimizationParameters& params = MinimizationParameters())
        : EnergyMinimizer(params), currentStepSize(params.initialStepSize) {}
    
    // Perform a single steepest descent step
    void step() override {
        if (!geometry || !forceCalculator) {
            std::cerr << "Error: Minimizer not initialized properly" << std::endl;
            return;
        }
        
        size_t numAtoms = geometry->getNumberOfAtoms();
        
        // Move atoms along the direction of the forces
        for (size_t i = 0; i < numAtoms; ++i) {
            std::array<double, 3> coords = geometry->getCoordinates(i);
            
            // Calculate displacement from forces
            for (int d = 0; d < 3; ++d) {
                // Move in the direction of the force (negative gradient)
                coords[d] += currentStepSize * forces[i][d];
            }
            
            // Update coordinates
            geometry->setCoordinates(i, coords);
        }
        
        // Adjust step size if adaptive step size is enabled
        if (params.adaptiveStepSize) {
            adjustStepSize();
        }
    }
    
    // Get current step size
    double getStepSize() const { return currentStepSize; }
    
    // Set step size
    void setStepSize(double stepSize) { currentStepSize = stepSize; }

private:
    double currentStepSize;
    
    // Adjust step size based on energy change
    void adjustStepSize() {
        // This is a simple adjustment strategy:
        // - If energy decreases, increase step size slightly
        // - If energy increases, decrease step size significantly
        
        // For now, we'll keep the current step size the same
        // A more sophisticated implementation would look at energy changes
        // between iterations and adjust accordingly
        
        // Ensure step size stays within bounds
        currentStepSize = std::min(currentStepSize, params.maxStepSize);
    }
};

#endif // STEEPEST_DESCENT_MINIMIZER_H