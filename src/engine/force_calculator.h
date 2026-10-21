#ifndef FORCE_CALCULATOR_H
#define FORCE_CALCULATOR_H

#include <vector>
#include <memory>
#include <string>
#include <array>
#include "../builder/geometry.h"

// Forward declaration of Potential class
class Potential;

class ForceCalculator {
public:
    ForceCalculator();
    ~ForceCalculator();

    // Add potentials to the simulation
    void addPotential(std::shared_ptr<Potential> potential);
    
    // Calculate total energy for the system
    double calculateEnergy(const Geometry& geometry);
    
    // Calculate forces for all atoms in the system
    void calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces);

    // Forces and total energy from one pass over each potential
    double calculateForcesAndEnergy(const Geometry& geometry,
                                    std::vector<std::array<double, 3>>& forces);
    
    // Clear all potentials
    void clearPotentials();

private:
    std::vector<std::shared_ptr<Potential>> potentials;
};

// Base class for all interaction potentials
class Potential {
public:
    virtual ~Potential() = default;
    
    // Calculate energy for a given geometry
    virtual double calculateEnergy(const Geometry& geometry) = 0;
    
    // Calculate forces for a given geometry
    virtual void calculateForces(const Geometry& geometry, 
                                std::vector<std::array<double, 3>>& forces) = 0;
    
    // Forces and energy together. The default makes two calls; a potential whose
    // kernel yields both in one pass (LJ) overrides this to halve the cost.
    virtual double calculateForcesAndEnergy(const Geometry& geometry,
                                            std::vector<std::array<double, 3>>& forces) {
        calculateForces(geometry, forces);
        return calculateEnergy(geometry);
    }

    // Get the name of the potential
    virtual std::string getName() const = 0;
};

#endif // FORCE_CALCULATOR_H