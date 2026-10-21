#include "lj_potential.h"

#include <cmath>
#include <iostream>

LJPotential::LJPotential() {
    std::cout << "[LJPotential constructed, cutoff = " << cutoff << "]" << std::endl;
}

LJPotential::~LJPotential() {
    std::cout << "[LJPotential destroyed]" << std::endl;
}

void LJPotential::addAtomParameters(const std::string& atomType, double epsilon, double sigma) {
    atomParameters[atomType] = {epsilon, sigma};
}

double LJPotential::calculateEnergy(const Geometry& geometry) {
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& box = geometry.getBoxDimensions();
    double totalEnergy = 0.0;

    for (std::size_t i = 0; i < atoms.size(); ++i) {
        for (std::size_t j = i + 1; j < atoms.size(); ++j) {
            Vec3 d;
            for (int k = 0; k < 3; ++k) {
                d[k] = positions[j][k] - positions[i][k];
                d[k] -= box[k] * std::round(d[k] / box[k]);
            }
            const double r = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (r > cutoff) continue;

            auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
            totalEnergy += calculatePairEnergy(epsilon, sigma, r);
        }
    }
    return totalEnergy;
}

void LJPotential::calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) {
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& box = geometry.getBoxDimensions();
    forces.assign(atoms.size(), Vec3{0.0, 0.0, 0.0});

    for (std::size_t i = 0; i < atoms.size(); ++i) {
        for (std::size_t j = i + 1; j < atoms.size(); ++j) {
            Vec3 d;
            for (int k = 0; k < 3; ++k) {
                d[k] = positions[j][k] - positions[i][k];
                d[k] -= box[k] * std::round(d[k] / box[k]);
            }
            const double r = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (r > cutoff) continue;

            auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
            const Vec3 direction = {d[0] / r, d[1] / r, d[2] / r};
            calculatePairForce(epsilon, sigma, r, direction, forces[i], forces[j]);
        }
    }
}

std::pair<double, double> LJPotential::getMixedParameters(const std::string& type1, const std::string& type2) {
    const AtomParameters& p1 = atomParameters[type1];
    const AtomParameters& p2 = atomParameters[type2];
    return {std::sqrt(p1.epsilon * p2.epsilon), std::sqrt(p1.sigma * p2.sigma)};
}

double LJPotential::calculatePairEnergy(double epsilon, double sigma, double distance) {
    const double sr6 = std::pow(sigma / distance, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

void LJPotential::calculatePairForce(double epsilon, double sigma, double distance,
                                     const Vec3& direction, Vec3& force1, Vec3& force2) {
    const double sr6 = std::pow(sigma / distance, 6);
    const double forceMagnitude = 24.0 * epsilon * (2.0 * sr6 * sr6 - sr6) / distance;
    for (int k = 0; k < 3; ++k) {
        force1[k] -= forceMagnitude * direction[k];
        force2[k] += forceMagnitude * direction[k];
    }
}
