// mini_lj, chapter 4: many atoms. Containers hold them; a double loop visits every pair.
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

using Vec3 = std::array<double, 3>;

// A bundle of two named numbers. Chapter 6 explains struct and class properly.
struct Params {
    double epsilon;
    double sigma;
};

int main(int argc, char* argv[]) {
    const std::size_t numAtoms = (argc >= 2) ? std::stoul(argv[1]) : 125;
    const double density = 0.8;
    const double cutoff = 2.5;

    // --- build a cubic lattice, the same recipe as LJParticleBoxBuilder::createCubicLattice
    const double box = std::cbrt(numAtoms / density);
    const int perSide = static_cast<int>(std::cbrt(numAtoms) + 0.5);
    const double spacing = box / perSide;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> jitter(-0.05 * spacing, 0.05 * spacing);

    std::vector<Vec3> positions;          // one entry per atom
    std::vector<std::string> types;       // one type name per atom
    for (int i = 0; i < perSide && positions.size() < numAtoms; ++i)
        for (int j = 0; j < perSide && positions.size() < numAtoms; ++j)
            for (int k = 0; k < perSide && positions.size() < numAtoms; ++k) {
                double x = (i + 0.5) * spacing - box / 2.0 + jitter(rng);
                double y = (j + 0.5) * spacing - box / 2.0 + jitter(rng);
                double z = (k + 0.5) * spacing - box / 2.0 + jitter(rng);
                positions.push_back({x, y, z});
                types.push_back("LJ");
            }

    // --- parameters per atom type
    std::map<std::string, Params> params;
    params["LJ"] = {1.0, 1.0};

    // --- every pair once: i < j
    double energy = 0.0;
    std::size_t inside = 0;
    for (std::size_t i = 0; i < positions.size(); ++i) {
        for (std::size_t j = i + 1; j < positions.size(); ++j) {
            Vec3 d;
            for (int k = 0; k < 3; ++k) {
                d[k] = positions[j][k] - positions[i][k];
                d[k] -= box * std::round(d[k] / box);   // minimum image
            }
            const double r = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (r > cutoff) continue;
            ++inside;

            const Params& pi = params[types[i]];
            const Params& pj = params[types[j]];
            const double eps = std::sqrt(pi.epsilon * pj.epsilon);   // geometric mixing
            const double sig = std::sqrt(pi.sigma * pj.sigma);
            const double sr6 = std::pow(sig / r, 6);
            energy += 4.0 * eps * (sr6 * sr6 - sr6);
        }
    }

    const std::size_t pairs = positions.size() * (positions.size() - 1) / 2;
    std::cout.precision(12);
    std::cout << "N = " << positions.size() << ", box = " << box << std::endl;
    std::cout << "pairs = " << pairs << ", inside cutoff = " << inside << std::endl;
    std::cout << "E = " << energy << std::endl;
    return 0;
}
