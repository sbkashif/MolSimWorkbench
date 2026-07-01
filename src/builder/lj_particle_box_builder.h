#ifndef LJ_PARTICLE_BOX_BUILDER_H
#define LJ_PARTICLE_BOX_BUILDER_H

#include <string>
#include "geometry.h"

// Configuration for LJ particle box initialization
struct LJBoxConfig {
    std::size_t numParticles = 125;         // Number of particles
    double density = 0.8;                   // Density in reduced units (particles/volume)
    std::string initialization = "cubic_lattice"; // Initialization method
    unsigned int randomSeed = 42;           // Random seed for reproducibility
    double temperature = 1.0;               // Initial temperature in Kelvin/reduced units
    
    // Phase control
    std::size_t equilibrationSteps = 1000;  // Equilibration steps (optional, for NVE can be 0)
    std::size_t productionSteps = 5000;     // Production steps
};

// Builder class for LJ particle boxes
class LJParticleBoxBuilder {
public:
    // Create a geometry containing particles initialized on cubic lattice
    // with Maxwell-Boltzmann velocities set to target temperature
    static Geometry createCubicLattice(
        const LJBoxConfig& config,
        std::array<double, 3>& boxSize
    );
    
private:
    // Helper to compute box size from density
    static double computeBoxSize(std::size_t numParticles, double density);
};

#endif // LJ_PARTICLE_BOX_BUILDER_H
