#ifndef FORCE_CALCULATOR_H
#define FORCE_CALCULATOR_H

#include <memory>
#include <vector>
#include "potential.h"

// Sums energy and forces over every potential it holds. Same shape as src/engine/force_calculator.h.
class ForceCalculator {
public:
    void addPotential(std::shared_ptr<Potential> potential) { potentials.push_back(potential); }
    double calculateEnergy(const Geometry& geometry);
    void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces);

private:
    std::vector<std::shared_ptr<Potential>> potentials;
};

#endif
