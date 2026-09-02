// Shared helpers for the parallelization exercises. Nothing here needs editing.
#ifndef EXERCISE_SUPPORT_H
#define EXERCISE_SUPPORT_H

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include "builder/lj_particle_box_builder.h"
#include "engine/potentials/lj_potential.h"

namespace ex {

// Structure-of-arrays view of a box of LJ particles, ready to hand to a kernel.
struct Box {
    std::vector<double> x, y, z;
    std::vector<int> type;
    std::array<double, 3> box{};
    std::size_t n = 0;
    double cutoff = 2.5;
    double epsilon = 1.0, sigma = 1.0;

    // Flat 1x1 mixing tables for the single-component case.
    std::vector<double> eps4{4.0}, sigma2{1.0};
    int ntypes = 1;

    Geometry geometry;
};

// Build a cubic-lattice LJ box. Use n >= 4000 for anything timing-related.
inline Box makeBox(std::size_t n, double density = 0.8, unsigned seed = 42) {
    LJBoxConfig cfg;
    cfg.numParticles = n;
    cfg.density = density;
    cfg.randomSeed = seed;

    Box b;
    b.geometry = LJParticleBoxBuilder::createCubicLattice(cfg, b.box);
    b.n = b.geometry.getNumberOfAtoms();
    const auto& pos = b.geometry.getPositions();
    b.x.resize(b.n); b.y.resize(b.n); b.z.resize(b.n); b.type.assign(b.n, 0);
    for (std::size_t i = 0; i < b.n; ++i) {
        b.x[i] = pos[i][0]; b.y[i] = pos[i][1]; b.z[i] = pos[i][2];
    }
    return b;
}

// Known-good forces, from the repo's ORIGINAL implementation.
inline std::vector<std::array<double, 3>> referenceForces(const Box& b) {
    LJPotential lj;
    lj.addAtomParameters("LJ", b.epsilon, b.sigma);
    lj.setCutoff(b.cutoff);
    std::vector<std::array<double, 3>> f;
    lj.calculateForcesReference(b.geometry, f);
    return f;
}

inline double referenceEnergy(const Box& b) {
    LJPotential lj;
    lj.addAtomParameters("LJ", b.epsilon, b.sigma);
    lj.setCutoff(b.cutoff);
    return lj.calculateEnergyReference(b.geometry);
}

// Largest relative deviation between an SoA force result and the reference.
inline double maxRelError(const std::vector<std::array<double, 3>>& ref,
                          const std::vector<double>& fx,
                          const std::vector<double>& fy,
                          const std::vector<double>& fz) {
    double m = 0.0;
    for (std::size_t i = 0; i < ref.size(); ++i) {
        const double c[3] = {fx[i], fy[i], fz[i]};
        for (int k = 0; k < 3; ++k) {
            const double scale = std::max(std::fabs(ref[i][k]), 1e-30);
            m = std::max(m, std::fabs(ref[i][k] - c[k]) / scale);
        }
    }
    return m;
}

// True if the kernel actually wrote something.
//
// An unimplemented stub leaves the force arrays zeroed, which would make
// "is it deterministic?" and "does the thread count matter?" pass VACUOUSLY --
// a kernel that does nothing is perfectly reproducible and perfectly
// thread-independent. Every such test asserts this first.
inline bool producedForces(const std::vector<double>& fx,
                           const std::vector<double>& fy,
                           const std::vector<double>& fz) {
    if (fx.empty()) return false;
    for (std::size_t i = 0; i < fx.size(); ++i) {
        if (fx[i] != 0.0 || fy[i] != 0.0 || fz[i] != 0.0) return true;
    }
    return false;
}

#define EX_REQUIRE_IMPLEMENTED(fx, fy, fz)                                      ASSERT_TRUE(ex::producedForces((fx), (fy), (fz)))                               << "not implemented yet -- the kernel left every force at zero, "              "which would make this test pass without doing anything"

}  // namespace ex
#endif
