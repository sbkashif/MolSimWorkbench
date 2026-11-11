#ifndef LJ_POTENTIAL_H
#define LJ_POTENTIAL_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "potential.h"

// Same public shape as src/engine/potentials/lj_potential.h (before the parallel work).
class LJPotential : public Potential {
public:
    LJPotential();

    double calculateEnergy(const Geometry& geometry) override;
    // Energy of the pairs (i, j>i) for i in [i0, i1). calculateEnergy is the range [0, N).
    double calculateEnergyRange(const Geometry& geometry, std::size_t i0, std::size_t i1);
    void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) override;
    std::string getName() const override { return "Lennard-Jones"; }

    void addAtomParameters(const std::string& atomType, double epsilon, double sigma);
    void setCutoff(double cutoff) { this->cutoff = cutoff; }

private:
    struct AtomParameters {
        double epsilon;
        double sigma;
    };

    std::map<std::string, AtomParameters> atomParameters;
    double cutoff = 10.0;

    std::pair<double, double> getMixedParameters(const std::string& type1, const std::string& type2);
    double calculatePairEnergy(double epsilon, double sigma, double distance);
    void calculatePairForce(double epsilon, double sigma, double distance,
                            const Vec3& direction, Vec3& force1, Vec3& force2);
};

#endif
