#ifndef LJ_POTENTIAL_H
#define LJ_POTENTIAL_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "geometry.h"

// Same public shape as src/engine/potentials/lj_potential.h (before the parallel work).
class LJPotential {
public:
    LJPotential();

    double calculateEnergy(const Geometry& geometry);
    void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces);
    std::string getName() const { return "Lennard-Jones"; }

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
