#ifndef POTENTIAL_H
#define POTENTIAL_H

#include <string>
#include <vector>
#include "geometry.h"

// The interface every potential promises. Same as `class Potential` in src/engine/force_calculator.h, minus calculateForcesAndEnergy.
class Potential {
public:
    virtual ~Potential() = default;

    virtual double calculateEnergy(const Geometry& geometry) = 0;
    virtual void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) = 0;
    virtual std::string getName() const = 0;
};

#endif
