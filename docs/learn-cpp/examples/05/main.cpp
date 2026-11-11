// mini_lj, chapter 5: the loop body moves into named functions in their own files.
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "lattice.h"
#include "lj_math.h"

int main(int argc, char* argv[]) {
    const std::size_t numAtoms = (argc >= 2) ? std::stoul(argv[1]) : 125;
    const double cutoff = 2.5;

    double box = 0.0;
    const std::vector<Vec3> positions = makeCubicLattice(numAtoms, 0.8, 42, box);
    const std::vector<std::string> types(positions.size(), "LJ");
    const std::map<std::string, AtomParameters> params = {{"LJ", {1.0, 1.0}}};

    double energy = 0.0;
    std::vector<Vec3> forces(positions.size(), Vec3{0.0, 0.0, 0.0});

    for (std::size_t i = 0; i < positions.size(); ++i) {
        for (std::size_t j = i + 1; j < positions.size(); ++j) {
            Vec3 d;
            for (int k = 0; k < 3; ++k) {
                d[k] = positions[j][k] - positions[i][k];
                d[k] -= box * std::round(d[k] / box);
            }
            const double r = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (r > cutoff) continue;

            auto [epsilon, sigma] = getMixedParameters(params, types[i], types[j]);
            energy += calculatePairEnergy(epsilon, sigma, r);
            const Vec3 direction = {d[0] / r, d[1] / r, d[2] / r};
            calculatePairForce(epsilon, sigma, r, direction, forces[i], forces[j]);
        }
    }

    Vec3 net = {0.0, 0.0, 0.0};
    for (const Vec3& f : forces)
        for (int k = 0; k < 3; ++k) net[k] += f[k];

    std::cout.precision(12);
    std::cout << "N = " << positions.size() << std::endl;
    std::cout << "E = " << energy << std::endl;
    std::cout << "force on atom 0 = (" << forces[0][0] << ", " << forces[0][1] << ", " << forces[0][2] << ")" << std::endl;
    std::cout << "net force       = (" << net[0] << ", " << net[1] << ", " << net[2] << ")" << std::endl;
    return 0;
}
