// ---------------------------------------------------------------------------
// Correctness tests for the lj_kernels backends.
//
// The contract every backend must satisfy: produce the same forces and the
// same energy as LJPotential's ORIGINAL triangular implementation, which is
// preserved as calculateForcesReference / calculateEnergyReference.
//
// Note the tolerance. The kernels are NOT bitwise identical to the reference,
// and that is expected, not a bug: the reference computes (sigma/r)^6 via
// std::pow, while the kernels compute (sigma^2/r^2)^3 with two multiplies.
// Those are algebraically equal but round differently, because floating-point
// arithmetic is not associative. Measured max relative deviation is ~1.5e-11,
// roughly 1e5 times larger than a single ULP and roughly 1e5 times smaller
// than anything that could affect a trajectory.
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <vector>

#include "builder/lj_particle_box_builder.h"
#include "engine/potentials/lj_potential.h"
#include "utils/yaml_parser.h"

namespace {

constexpr double kRelTol = 1e-9;   // generous: actual deviation is ~1.5e-11

struct Fixture {
    Geometry geometry;
    std::array<double, 3> box{};
    LJPotential lj;

    explicit Fixture(std::size_t n, double density = 0.8, double cutoff = 2.5) {
        LJBoxConfig cfg;
        cfg.numParticles = n;
        cfg.density = density;
        cfg.randomSeed = 42;
        geometry = LJParticleBoxBuilder::createCubicLattice(cfg, box);
        lj.addAtomParameters("LJ", 1.0, 1.0);
        lj.setCutoff(cutoff);
    }
};

void expectForcesMatch(const std::vector<std::array<double, 3>>& expected,
                       const std::vector<std::array<double, 3>>& actual,
                       const char* what) {
    ASSERT_EQ(expected.size(), actual.size()) << what;
    double maxRel = 0.0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        for (int k = 0; k < 3; ++k) {
            const double scale = std::max(std::fabs(expected[i][k]), 1e-30);
            maxRel = std::max(maxRel, std::fabs(expected[i][k] - actual[i][k]) / scale);
        }
    }
    EXPECT_LT(maxRel, kRelTol)
        << what << ": max relative force deviation " << maxRel << " exceeds " << kRelTol;
}

const LJBackend kAllBackends[] = {
    LJBackend::SerialTriangular,
    LJBackend::Serial,
    LJBackend::Threads,
    LJBackend::OpenMP,
    LJBackend::OpenMPPrivate,
};

const char* name(LJBackend b) {
    switch (b) {
        case LJBackend::SerialTriangular: return "SerialTriangular";
        case LJBackend::Serial: return "Serial";
        case LJBackend::Threads: return "Threads";
        case LJBackend::OpenMP: return "OpenMP";
        case LJBackend::OpenMPPrivate: return "OpenMPPrivate";
    }
    return "?";
}

}  // namespace

// Every backend must reproduce the reference forces.
TEST(LJKernels, AllBackendsMatchReferenceForces) {
    Fixture fx(500);
    std::vector<std::array<double, 3>> reference;
    fx.lj.calculateForcesReference(fx.geometry, reference);

    for (LJBackend b : kAllBackends) {
        fx.lj.setBackend(b);
        std::vector<std::array<double, 3>> actual;
        fx.lj.calculateForces(fx.geometry, actual);
        expectForcesMatch(reference, actual, name(b));
    }
}

// ... and the reference energy.
TEST(LJKernels, AllBackendsMatchReferenceEnergy) {
    Fixture fx(500);
    const double reference = fx.lj.calculateEnergyReference(fx.geometry);
    ASSERT_LT(reference, 0.0) << "sanity: a dense LJ box should be net attractive";

    for (LJBackend b : kAllBackends) {
        fx.lj.setBackend(b);
        const double actual = fx.lj.calculateEnergy(fx.geometry);
        EXPECT_NEAR(actual, reference, std::fabs(reference) * kRelTol) << name(b);
    }
}

// The whole point of the gather formulation is that thread count cannot change
// the answer. If this test ever fails, there is a race.
TEST(LJKernels, ResultIsIndependentOfThreadCount) {
    Fixture fx(600);
    fx.lj.setBackend(LJBackend::Serial);
    std::vector<std::array<double, 3>> reference;
    fx.lj.calculateForces(fx.geometry, reference);

    for (int threads : {1, 2, 3, 4, 5, 8, 16}) {
        for (LJBackend b : {LJBackend::Threads, LJBackend::OpenMP, LJBackend::OpenMPPrivate}) {
            fx.lj.setBackend(b);
            fx.lj.setNumThreads(threads);
            std::vector<std::array<double, 3>> actual;
            fx.lj.calculateForces(fx.geometry, actual);
            expectForcesMatch(reference, actual,
                              (std::string(name(b)) + " with " +
                               std::to_string(threads) + " threads").c_str());
        }
    }
}

// Running the same backend twice must give byte-identical output. A race often
// shows up here first, as a result that changes between runs.
TEST(LJKernels, RepeatedRunsAreDeterministic) {
    Fixture fx(600);
    for (LJBackend b : kAllBackends) {
        fx.lj.setBackend(b);
        fx.lj.setNumThreads(8);
        std::vector<std::array<double, 3>> first, second;
        fx.lj.calculateForces(fx.geometry, first);
        fx.lj.calculateForces(fx.geometry, second);
        for (std::size_t i = 0; i < first.size(); ++i) {
            for (int k = 0; k < 3; ++k) {
                EXPECT_DOUBLE_EQ(first[i][k], second[i][k])
                    << name(b) << " atom " << i << " component " << k;
            }
        }
    }
}

// Newton's third law: with periodic boundaries and no external field, the net
// force on the whole system must vanish. This catches sign and PBC errors that
// a comparison against the reference would miss if BOTH were wrong.
TEST(LJKernels, NetForceIsZero) {
    Fixture fx(500);
    for (LJBackend b : kAllBackends) {
        fx.lj.setBackend(b);
        std::vector<std::array<double, 3>> f;
        fx.lj.calculateForces(fx.geometry, f);

        std::array<double, 3> net{0.0, 0.0, 0.0};
        double magnitude = 0.0;
        for (const auto& fi : f) {
            for (int k = 0; k < 3; ++k) {
                net[k] += fi[k];
                magnitude += std::fabs(fi[k]);
            }
        }
        // Compare against the summed magnitude, not against zero: cancelling
        // ~10^4 numbers of size ~10^1 leaves rounding noise, not exact zero.
        for (int k = 0; k < 3; ++k) {
            EXPECT_LT(std::fabs(net[k]) / magnitude, 1e-12) << name(b) << " axis " << k;
        }
    }
}

// Degenerate inputs must not crash or produce NaN.
TEST(LJKernels, HandlesEmptyAndSingleAtom) {
    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);

    for (LJBackend b : kAllBackends) {
        lj.setBackend(b);

        Geometry empty;
        std::vector<std::array<double, 3>> f;
        EXPECT_DOUBLE_EQ(lj.calculateEnergy(empty), 0.0) << name(b);
        lj.calculateForces(empty, f);
        EXPECT_TRUE(f.empty()) << name(b);

        Geometry single;
        single.setBoxDimensions({10.0, 10.0, 10.0});
        single.addAtom("LJ", {0.0, 0.0, 0.0});
        EXPECT_DOUBLE_EQ(lj.calculateEnergy(single), 0.0) << name(b);
        lj.calculateForces(single, f);
        ASSERT_EQ(f.size(), 1u) << name(b);
        for (int k = 0; k < 3; ++k) EXPECT_DOUBLE_EQ(f[0][k], 0.0) << name(b);
    }
}

// An atom type with no registered parameters must contribute nothing, matching
// the reference implementation's {0,0} fallback -- not crash on a -1 type id.
TEST(LJKernels, UnparameterisedTypeContributesNothing) {
    Geometry g;
    g.setBoxDimensions({10.0, 10.0, 10.0});
    g.addAtom("LJ", {0.0, 0.0, 0.0});
    g.addAtom("MYSTERY", {1.1, 0.0, 0.0});

    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);

    const double reference = lj.calculateEnergyReference(g);
    for (LJBackend b : kAllBackends) {
        lj.setBackend(b);
        EXPECT_DOUBLE_EQ(lj.calculateEnergy(g), reference) << name(b);
    }
}

// ---------------------------------------------------------------------------
// Long-range dispersion correction (lesson 8).
// ---------------------------------------------------------------------------

// The headline claim: enabling the correction must not perturb a single force.
TEST(LJTailCorrection, DoesNotChangeAnyForce) {
    Fixture fx(1000);
    fx.lj.setBackend(LJBackend::Serial);

    std::vector<std::array<double, 3>> without, with;
    fx.lj.setTailCorrection(false);
    fx.lj.calculateForces(fx.geometry, without);
    fx.lj.setTailCorrection(true);
    fx.lj.calculateForces(fx.geometry, with);

    ASSERT_EQ(without.size(), with.size());
    for (std::size_t i = 0; i < without.size(); ++i) {
        for (int k = 0; k < 3; ++k) {
            // Bitwise, not approximately: the correction never touches forces.
            EXPECT_DOUBLE_EQ(without[i][k], with[i][k]) << "atom " << i << " comp " << k;
        }
    }
}

// It must, however, lower the energy -- the truncated tail is attractive.
TEST(LJTailCorrection, LowersTheEnergy) {
    Fixture fx(1000);
    fx.lj.setTailCorrection(false);
    const double plain = fx.lj.calculateEnergy(fx.geometry);
    fx.lj.setTailCorrection(true);
    const double corrected = fx.lj.calculateEnergy(fx.geometry);

    EXPECT_LT(corrected, plain) << "the dispersion tail is attractive";
    EXPECT_LT(fx.lj.tailCorrectionEnergy(fx.geometry), 0.0);
}

// Check the closed form against the textbook single-component expression:
//   E_tail = (8/3) pi N rho eps sigma^3 [ (1/3)(sigma/rc)^9 - (sigma/rc)^3 ]
TEST(LJTailCorrection, MatchesTheAnalyticFormula) {
    Fixture fx(1000);
    fx.lj.setTailCorrection(true);

    const double n = static_cast<double>(fx.geometry.getNumberOfAtoms());
    const double volume = fx.box[0] * fx.box[1] * fx.box[2];
    const double rho = n / volume;
    const double eps = 1.0, sigma = 1.0, rc = 2.5;
    const double sr3 = std::pow(sigma / rc, 3);
    const double sr9 = sr3 * sr3 * sr3;
    const double expected =
        (8.0 / 3.0) * M_PI * n * rho * eps * sigma * sigma * sigma * (sr9 / 3.0 - sr3);

    EXPECT_NEAR(fx.lj.tailCorrectionEnergy(fx.geometry), expected, std::fabs(expected) * 1e-12);
}

// Off by default, and inert without a box (no density => no uniform-fluid model).
TEST(LJTailCorrection, OffByDefaultAndNeedsABox) {
    Fixture fx(200);
    EXPECT_FALSE(fx.lj.getTailCorrection());
    EXPECT_DOUBLE_EQ(fx.lj.tailCorrectionEnergy(fx.geometry), 0.0);

    Geometry noBox;
    noBox.addAtom("LJ", {0.0, 0.0, 0.0});
    noBox.addAtom("LJ", {1.1, 0.0, 0.0});
    fx.lj.setTailCorrection(true);
    EXPECT_DOUBLE_EQ(fx.lj.tailCorrectionEnergy(noBox), 0.0);
}

// The `tail_correction:` key in a potential YAML file reaches the potential.
// The loader hook stands in for the YAML file, so no Python is needed.
TEST(LJTailCorrection, YamlKeyTurnsItOnAndOff) {
    auto parseWith = [](const std::string& tailLine) {
        YAMLParser::setLoader([tailLine](const std::string&) {
            return std::string(R"({"potential_type": "classical", "components": [{)"
                               R"("type": "lennard_jones", "cutoff": 2.5,)"
                               R"("atom_parameters": {"LJ": {"epsilon": 1.0, "sigma": 1.0}})")
                   + tailLine + "}]}";
        });
        YAMLParser parser;
        auto lj = std::dynamic_pointer_cast<LJPotential>(parser.parsePotential("test.yaml"));
        YAMLParser::setLoader(nullptr);
        return lj;
    };

    auto on = parseWith(R"(, "tail_correction": true)");
    auto off = parseWith(R"(, "tail_correction": false)");
    auto absent = parseWith("");
    ASSERT_TRUE(on && off && absent);
    EXPECT_TRUE(on->getTailCorrection());
    EXPECT_FALSE(off->getTailCorrection());
    EXPECT_FALSE(absent->getTailCorrection());
}
