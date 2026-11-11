#include "lj_math.h"

#include <cmath>

std::pair<double, double> getMixedParameters(const std::map<std::string, AtomParameters>& params,
                                             const std::string& type1,
                                             const std::string& type2) {
    const AtomParameters& p1 = params.at(type1);
    const AtomParameters& p2 = params.at(type2);
    return {std::sqrt(p1.epsilon * p2.epsilon), std::sqrt(p1.sigma * p2.sigma)};
}

double calculatePairEnergy(double epsilon, double sigma, double distance) {
    const double sr6 = std::pow(sigma / distance, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

void calculatePairForce(double epsilon, double sigma, double distance,
                        const Vec3& direction, Vec3& force1, Vec3& force2) {
    const double sr6 = std::pow(sigma / distance, 6);
    const double sr12 = sr6 * sr6;
    const double forceMagnitude = 24.0 * epsilon * (2.0 * sr12 - sr6) / distance;
    for (int k = 0; k < 3; ++k) {
        force1[k] -= forceMagnitude * direction[k];
        force2[k] += forceMagnitude * direction[k];
    }
}
