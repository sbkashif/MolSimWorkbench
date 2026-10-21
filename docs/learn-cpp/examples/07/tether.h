#ifndef TETHER_H
#define TETHER_H

#include "potential.h"

// A spring pulling every atom toward the centre of the box: E = 1/2 k |r|^2, F = -k r.
// Not in the repo. It exists so there are two different potentials to stack.
class HarmonicTether : public Potential {
public:
    explicit HarmonicTether(double k) : k(k) {}

    double calculateEnergy(const Geometry& geometry) override;
    void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) override;
    std::string getName() const override { return "Harmonic tether"; }

private:
    double k;
};

#endif
