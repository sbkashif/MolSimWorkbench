// Where does one MD step spend its time? Times the force step, the energy
// evaluation and a whole step of the real engine at several system sizes.
// With the argument `openmp`, the LJ potential uses the OpenMPPrivate backend.
//
//   OMP=$(brew --prefix libomp)        # macOS; on Linux drop the -L part
//   clang++ -std=c++17 -O3 -I <repo>/src step_split.cpp \
//       <repo>/build/libEngineLib.a <repo>/build/libBuilderLib.a \
//       -L $OMP/lib -lomp -o step_split
//   ./step_split            # serial (the engine's default backend)
//   ./step_split openmp     # private-buffer OpenMP, all cores
#include "builder/lj_particle_box_builder.h"
#include "engine/force_calculator.h"
#include "engine/molecular_dynamics.h"
#include "engine/potentials/lj_potential.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

// An energy function that does no work: zero energy, zero forces. A run with
// it measures everything in a step EXCEPT the pair loops.
class NullPotential : public Potential {
public:
    double calculateEnergy(const Geometry&) override { return 0.0; }
    void calculateForces(const Geometry& g, std::vector<std::array<double, 3>>& f) override {
        f.assign(g.getNumberOfAtoms(), {0.0, 0.0, 0.0});
    }
    std::string getName() const override { return "null"; }
};

// Fastest of `reps` runs, in milliseconds, after one warm-up run.
template <typename F>
double best_ms(F&& f, int reps) {
    f();
    double best = 1e300;
    for (int r = 0; r < reps; ++r) {
        auto t0 = Clock::now();
        f();
        auto t1 = Clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return best;
}

int main(int argc, char** argv) {
    const bool openmp = argc > 1 && std::string(argv[1]) == "openmp";
    std::printf("%6s %10s %10s %10s %10s\n", "N", "step_ms", "forces_ms", "energy_ms", "rest_ms");
    for (int side : {5, 7, 9, 11, 13, 16}) {
        LJBoxConfig cfg;
        cfg.numParticles = static_cast<std::size_t>(side) * side * side;
        cfg.density = 0.8;
        cfg.randomSeed = 42;
        std::array<double, 3> box{};
        Geometry geometry = LJParticleBoxBuilder::createCubicLattice(cfg, box);

        auto lj = std::make_shared<LJPotential>();
        lj->addAtomParameters("LJ", 1.0, 1.0);
        lj->setCutoff(2.5);
        if (openmp) lj->setBackend(LJBackend::OpenMPPrivate);
        auto fc = std::make_shared<ForceCalculator>();
        fc->addPotential(lj);

        std::vector<std::array<double, 3>> f(geometry.getNumberOfAtoms());
        const int reps = 5;
        const double forces = best_ms([&] { fc->calculateForces(geometry, f); }, reps);
        const double energy = best_ms([&] { fc->calculateEnergy(geometry); }, reps);

        // Real runs of the engine's own loop, output silenced: once with LJ,
        // once with the null energy function.
        auto nullfc = std::make_shared<ForceCalculator>();
        nullfc->addPotential(std::make_shared<NullPotential>());
        const int steps = 50;
        SimulationParameters p;
        p.timeStep = 0.001;
        p.totalTime = steps * p.timeStep;
        p.temperature = 1.0;
        p.useMinimization = false;
        p.usePBC = true;
        p.randomSeed = 42;
        p.box = SimulationBox(box[0], box[1], box[2]);
        auto per_step = [&](std::shared_ptr<ForceCalculator> calc) {
            std::ostringstream sink;
            auto* old = std::cout.rdbuf(sink.rdbuf());
            MolecularDynamics md;
            md.setGeometry(geometry);
            md.setForceCalculator(calc);
            md.setSimulationParameters(p);
            md.initialize();
            const double ms = best_ms([&] { md.runSimulation(); }, 3);
            std::cout.rdbuf(old);
            return ms / steps;
        };
        const double step = per_step(fc);
        const double rest = per_step(nullfc);
        std::printf("%6zu %10.4f %10.4f %10.4f %10.4f\n", geometry.getNumberOfAtoms(),
                    step, forces, energy, rest);
    }
    return 0;
}
