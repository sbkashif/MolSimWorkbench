#ifndef LJ_MATH_H
#define LJ_MATH_H

#include <array>
#include <map>
#include <string>
#include <utility>

using Vec3 = std::array<double, 3>;

struct AtomParameters {
    double epsilon;
    double sigma;
};

// Declarations only. The bodies are in lj_math.cpp.
std::pair<double, double> getMixedParameters(const std::map<std::string, AtomParameters>& params,
                                             const std::string& type1,
                                             const std::string& type2);
double calculatePairEnergy(double epsilon, double sigma, double distance);
void calculatePairForce(double epsilon, double sigma, double distance,
                        const Vec3& direction, Vec3& force1, Vec3& force2);

#endif
