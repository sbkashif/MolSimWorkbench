// Integrator and Thermostat plugs: each one is tested on its own, on a small
// Lennard-Jones liquid, through the same MDState view MolecularDynamics uses.

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>
#include "../../src/builder/geometry.h"
#include "../../src/builder/lj_particle_box_builder.h"
#include "../../src/engine/force_calculator.h"
#include "../../src/engine/md_state.h"
#include "../../src/engine/molecular_dynamics.h"
#include "../../src/engine/potentials/lj_potential.h"
#include "../../src/engine/integrators/integrator.h"
#include "../../src/engine/integrators/velocity_verlet_integrator.h"
#include "../../src/engine/integrators/leapfrog_integrator.h"
#include "../../src/engine/thermostats/thermostat.h"
#include "../../src/engine/thermostats/velocity_rescale_thermostat.h"
#include "../../src/engine/thermostats/langevin_thermostat.h"

namespace {

using Vec3 = std::array<double, 3>;

// A self-contained system: 125 LJ atoms at density 0.8 (L = 5, cutoff 2.5),
// Maxwell-Boltzmann velocities at temperature T, no centre-of-mass motion.
struct LJSystem {
    Geometry geometry;
    std::vector<Vec3> velocities;
    std::vector<Vec3> forces;
    double potentialEnergy = 0.0;
    std::shared_ptr<ForceCalculator> forceCalculator = std::make_shared<ForceCalculator>();
    SimulationBox box;

    explicit LJSystem(double temperature, unsigned int seed = 7) {
        LJBoxConfig config;
        config.numParticles = 125;
        config.density = 0.8;
        std::array<double, 3> boxSize;
        geometry = LJParticleBoxBuilder::createCubicLattice(config, boxSize);
        geometry.setBoxDimensions(boxSize);
        box = SimulationBox(boxSize[0], boxSize[1], boxSize[2]);

        auto lj = std::make_shared<LJPotential>();
        lj->addAtomParameters("LJ", 1.0, 1.0);
        lj->setCutoff(2.5);
        forceCalculator->addPotential(lj);

        const size_t n = geometry.getNumberOfAtoms();
        velocities.assign(n, {0.0, 0.0, 0.0});
        forces.assign(n, {0.0, 0.0, 0.0});

        std::mt19937 rng(seed);
        std::normal_distribution<double> gauss(0.0, std::sqrt(temperature));
        Vec3 vcom = {0.0, 0.0, 0.0};
        for (auto& v : velocities) {
            for (int d = 0; d < 3; ++d) { v[d] = gauss(rng); vcom[d] += v[d]; }
        }
        for (auto& v : velocities) {
            for (int d = 0; d < 3; ++d) { v[d] -= vcom[d] / n; }
        }
        MDState s = state();
        const double scale = std::sqrt(temperature / s.temperature());
        for (auto& v : velocities) {
            for (int d = 0; d < 3; ++d) { v[d] *= scale; }
        }
        s.computeForces();
    }

    MDState state() {
        return MDState{geometry, velocities, forces, potentialEnergy, *forceCalculator, true, box};
    }

    double totalEnergy() { return potentialEnergy + state().kineticEnergy(); }

    // Melt the starting lattice into a liquid at temperature T.
    void equilibrate(double temperature, int steps, double dt) {
        VelocityVerletIntegrator vv;
        VelocityRescaleThermostat rescale;
        MDState s = state();
        for (int i = 0; i < steps; ++i) {
            vv.step(s, dt);
            rescale.afterStep(s, dt, temperature);
        }
    }
};

// Largest |E(t) - E(0)| / |E(0)| over an NVE run.
double maxRelativeEnergyDeviation(LJSystem& sys, Integrator& integrator, int steps, double dt) {
    MDState s = sys.state();
    const double e0 = sys.totalEnergy();
    double maxDev = 0.0;
    for (int i = 0; i < steps; ++i) {
        integrator.step(s, dt);
        maxDev = std::max(maxDev, std::abs(sys.totalEnergy() - e0));
    }
    return maxDev / std::abs(e0);
}

// Mean temperature over the last `average` of `steps` thermostatted steps.
double thermostattedMeanTemperature(LJSystem& sys, Thermostat& thermostat,
                                    double target, int steps, int average, double dt) {
    VelocityVerletIntegrator vv;
    MDState s = sys.state();
    double sum = 0.0;
    for (int i = 0; i < steps; ++i) {
        thermostat.beforeStep(s, dt, target);
        vv.step(s, dt);
        thermostat.afterStep(s, dt, target);
        if (i >= steps - average) sum += s.temperature();
    }
    return sum / average;
}

}  // namespace

// ---------------------------------------------------------------------------
// Integrators
// ---------------------------------------------------------------------------

// The NVE tolerance is loose-ish (0.5%) because the LJ potential is not shifted
// at the cutoff (a known limitation): each pair crossing r_c makes the energy
// jump by ~0.016, whatever dt is. A real drift would blow well past it.
TEST(IntegratorTest, VelocityVerletConservesEnergyNVE) {
    LJSystem sys(1.0);
    sys.equilibrate(1.0, 300, 0.002);
    VelocityVerletIntegrator vv;
    const double drift = maxRelativeEnergyDeviation(sys, vv, 2000, 0.002);
    EXPECT_LT(drift, 5e-3) << "velocity Verlet NVE energy deviation " << drift;
}

TEST(IntegratorTest, LeapfrogConservesEnergyNVE) {
    LJSystem sys(1.0);
    sys.equilibrate(1.0, 300, 0.002);
    LeapfrogIntegrator lf;
    const double drift = maxRelativeEnergyDeviation(sys, lf, 2000, 0.002);
    EXPECT_LT(drift, 5e-3) << "leapfrog NVE energy deviation " << drift;
}

TEST(IntegratorTest, LeapfrogAgreesWithVelocityVerletToOrderDtSquared) {
    const double dt = 0.002;
    const int steps = 200;
    LJSystem a(1.0), b(1.0);
    a.equilibrate(1.0, 300, dt);
    b.equilibrate(1.0, 300, dt);

    VelocityVerletIntegrator vv;
    LeapfrogIntegrator lf;
    MDState sa = a.state(), sb = b.state();
    for (int i = 0; i < steps; ++i) {
        vv.step(sa, dt);
        lf.step(sb, dt);
    }

    // Minimum-image distance, so an atom wrapped in one run but not the other
    // still counts as being in the same place.
    const double L[3] = {a.box.a, a.box.b, a.box.c};
    double maxDr = 0.0, maxDv = 0.0;
    for (size_t i = 0; i < a.geometry.getNumberOfAtoms(); ++i) {
        const Vec3 ra = a.geometry.getCoordinates(i), rb = b.geometry.getCoordinates(i);
        for (int d = 0; d < 3; ++d) {
            double dr = ra[d] - rb[d];
            dr -= L[d] * std::round(dr / L[d]);
            maxDr = std::max(maxDr, std::abs(dr));
            maxDv = std::max(maxDv, std::abs(a.velocities[i][d] - b.velocities[i][d]));
        }
    }
    // The two recipes are the same map in exact arithmetic, so they agree far
    // better than dt^2; dt^2 is the bound the method itself promises.
    EXPECT_LT(maxDr, dt * dt);
    EXPECT_LT(maxDv, dt * dt);
}

TEST(IntegratorTest, FactoryBuildsByName) {
    EXPECT_EQ(makeIntegrator("velocity_verlet")->getName(), "velocity_verlet");
    EXPECT_EQ(makeIntegrator("leapfrog")->getName(), "leapfrog");
    EXPECT_THROW(makeIntegrator("euler"), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Thermostats
// ---------------------------------------------------------------------------

TEST(ThermostatTest, VelocityRescaleHitsTargetEveryStep) {
    LJSystem sys(0.5);
    VelocityRescaleThermostat rescale;  // tau = 0
    VelocityVerletIntegrator vv;
    MDState s = sys.state();
    for (int i = 0; i < 50; ++i) {
        vv.step(s, 0.002);
        rescale.afterStep(s, 0.002, 1.5);
        EXPECT_NEAR(s.temperature(), 1.5, 1e-9);
    }
}

TEST(ThermostatTest, BerendsenRelaxesToTarget) {
    LJSystem sys(0.5);
    sys.equilibrate(0.5, 300, 0.002);
    VelocityRescaleThermostat berendsen(0.05);
    EXPECT_EQ(berendsen.getName(), "berendsen");
    // 2000 steps of 0.002 = 80 tau
    const double meanT = thermostattedMeanTemperature(sys, berendsen, 1.5, 2000, 500, 0.002);
    EXPECT_NEAR(meanT, 1.5, 0.03 * 1.5);
}

TEST(ThermostatTest, LangevinDrivesTemperatureToTarget) {
    LJSystem sys(0.5);
    sys.equilibrate(0.5, 300, 0.002);
    LangevinThermostat langevin(2.0, 12345);
    // Kinetic energy relaxes in ~1/(2 gamma) = 0.25; run 10 time units and
    // average the second half, where the canonical fluctuations average out.
    const double meanT = thermostattedMeanTemperature(sys, langevin, 1.5, 5000, 2500, 0.002);
    EXPECT_NEAR(meanT, 1.5, 0.05 * 1.5);
}

TEST(ThermostatTest, LangevinIsReproducibleWithSeed) {
    LJSystem a(1.0), b(1.0);
    LangevinThermostat la(1.0, 99), lb(1.0, 99);
    thermostattedMeanTemperature(a, la, 1.0, 50, 1, 0.002);
    thermostattedMeanTemperature(b, lb, 1.0, 50, 1, 0.002);
    EXPECT_EQ(a.velocities, b.velocities);
}

TEST(ThermostatTest, FactoryBuildsByName) {
    EXPECT_EQ(makeThermostat("velocity_rescale", 0.0, 1.0, 0)->getName(), "velocity_rescale");
    EXPECT_EQ(makeThermostat("berendsen", 0.1, 1.0, 0)->getName(), "berendsen");
    EXPECT_EQ(makeThermostat("langevin", 0.0, 1.0, 1)->getName(), "langevin");
    EXPECT_EQ(makeThermostat("none", 0.0, 1.0, 0), nullptr);
    EXPECT_THROW(makeThermostat("berendsen", 0.0, 1.0, 0), std::invalid_argument);
    EXPECT_THROW(makeThermostat("langevin", 0.0, 0.0, 0), std::invalid_argument);
    EXPECT_THROW(makeThermostat("nose_hoover", 0.0, 1.0, 0), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// MolecularDynamics picks plugs from SimulationParameters
// ---------------------------------------------------------------------------

TEST(MolecularDynamicsPlugTest, DefaultsAreVelocityVerletAndRescale) {
    MolecularDynamics md;
    EXPECT_EQ(md.getIntegrator()->getName(), "velocity_verlet");
    EXPECT_EQ(md.getThermostat()->getName(), "velocity_rescale");
}

TEST(MolecularDynamicsPlugTest, ParametersChoosePlugsByName) {
    MolecularDynamics md;
    SimulationParameters params;
    params.integrator = "leapfrog";
    params.thermostat = "langevin";
    params.langevinFriction = 0.5;
    md.setSimulationParameters(params);
    EXPECT_EQ(md.getIntegrator()->getName(), "leapfrog");
    EXPECT_EQ(md.getThermostat()->getName(), "langevin");

    params.thermostat = "none";
    md.setSimulationParameters(params);
    EXPECT_EQ(md.getThermostat(), nullptr);

    params.integrator = "rk4";
    EXPECT_THROW(md.setSimulationParameters(params), std::invalid_argument);
}

TEST(MolecularDynamicsPlugTest, ExplicitPlugSurvivesNewParameters) {
    MolecularDynamics md;
    md.setIntegrator(std::make_shared<LeapfrogIntegrator>());
    md.setThermostat(std::make_shared<LangevinThermostat>(1.0, 3));
    md.setSimulationParameters(SimulationParameters{});
    EXPECT_EQ(md.getIntegrator()->getName(), "leapfrog");
    EXPECT_EQ(md.getThermostat()->getName(), "langevin");
}

TEST(MolecularDynamicsPlugTest, LeapfrogLangevinRunsEndToEnd) {
    LJSystem sys(1.0);
    MolecularDynamics md;
    md.setGeometry(sys.geometry);
    md.setForceCalculator(sys.forceCalculator);

    SimulationParameters params;
    params.timeStep = 0.002;
    params.temperature = 1.2;
    params.totalTime = 200 * 0.002;
    params.usePBC = true;
    params.useMinimization = false;
    params.box = sys.box;
    params.randomSeed = 5;
    params.integrator = "leapfrog";
    params.thermostat = "langevin";
    params.langevinFriction = 2.0;
    md.setSimulationParameters(params);
    md.initialize();
    md.runSimulation();

    const double e = md.getCurrentEnergy();
    EXPECT_TRUE(std::isfinite(e));
}
