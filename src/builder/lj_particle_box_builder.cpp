#include "lj_particle_box_builder.h"
#include <cmath>
#include <random>
#include <iostream>
#include <array>

double LJParticleBoxBuilder::computeBoxSize(std::size_t numParticles, double density) {
    // V = N / rho  =>  L = (N / rho)^(1/3)
    double volume = static_cast<double>(numParticles) / density;
    return std::cbrt(volume);
}

Geometry LJParticleBoxBuilder::createCubicLattice(
    const LJBoxConfig& config,
    std::array<double, 3>& boxSize
) {
    // Compute box dimensions
    double boxLength = computeBoxSize(config.numParticles, config.density);
    boxSize = {boxLength, boxLength, boxLength};
    
    std::cout << "Creating LJ particle box: N=" << config.numParticles 
              << ", rho=" << config.density 
              << ", L=" << boxLength << std::endl;
    
    Geometry geometry;
    geometry.setBoxDimensions(boxSize);
    
    // Calculate lattice spacing and particles per dimension
    // Smallest n with n^3 >= N, so every particle gets a lattice site. Rounding
    // cbrt(N) to nearest would give too few sites for N = 130 (5^3 = 125).
    int particlesPerDim = static_cast<int>(std::cbrt(static_cast<double>(config.numParticles)));
    while (static_cast<std::size_t>(particlesPerDim) * particlesPerDim * particlesPerDim
           < config.numParticles) {
        ++particlesPerDim;
    }
    double spacing = boxLength / particlesPerDim;
    
    // Random number generator for small displacements to break symmetry
    std::mt19937 rng(config.randomSeed);
    std::uniform_real_distribution<double> smallDisp(-0.05 * spacing, 0.05 * spacing);
    
    // Place particles on cubic lattice with small random offsets
    std::size_t particlesAdded = 0;
    for (int i = 0; i < particlesPerDim && particlesAdded < config.numParticles; ++i) {
        for (int j = 0; j < particlesPerDim && particlesAdded < config.numParticles; ++j) {
            for (int k = 0; k < particlesPerDim && particlesAdded < config.numParticles; ++k) {
                // Position on lattice, centered in box
                double x = (i + 0.5) * spacing - boxLength / 2.0 + smallDisp(rng);
                double y = (j + 0.5) * spacing - boxLength / 2.0 + smallDisp(rng);
                double z = (k + 0.5) * spacing - boxLength / 2.0 + smallDisp(rng);
                
                geometry.addAtom("LJ", {x, y, z});
                ++particlesAdded;
            }
        }
    }
    
    std::cout << "Placed " << particlesAdded << " particles on cubic lattice" << std::endl;
    return geometry;
}
