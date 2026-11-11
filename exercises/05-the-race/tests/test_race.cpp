#include <gtest/gtest.h>

#include <cmath>

#include "exercise_support.h"
#include "race.h"

TEST(Ex05Race, CountWrongAtomsWorks) {
    std::vector<std::array<double, 3>> ref{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    std::vector<double> fx{1.0, 4.0}, fy{2.0, 5.0}, fz{3.0, 6.0};
    EXPECT_EQ(ex05::countWrongAtoms(ref, fx, fy, fz, 1e-9), 0u);
    fy[1] = 5.5;                                    // 10% off
    EXPECT_EQ(ex05::countWrongAtoms(ref, fx, fy, fz, 1e-9), 1u);
    fx[0] = 1.5;
    EXPECT_EQ(ex05::countWrongAtoms(ref, fx, fy, fz, 1e-9), 2u);
}

TEST(Ex05Race, PrivateBufferMatchesTheReference) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    for (int T : {1, 2, 4, 8}) {
        std::vector<double> fx, fy, fz;
        ex05::privateBufferScatter(b.x, b.y, b.z, 4.0, 1.0,
                                   b.box[0], b.box[1], b.box[2], b.cutoff,
                                   fx, fy, fz, T);
        ASSERT_EQ(fx.size(), b.n) << "T=" << T;
        EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9) << "T=" << T;
        EXPECT_EQ(ex05::countWrongAtoms(ref, fx, fy, fz, 1e-9), 0u) << "T=" << T;
    }
}

TEST(Ex05Race, PrivateBufferEnergyMatches) {
    const ex::Box b = ex::makeBox(600);
    const double ref = ex::referenceEnergy(b);
    std::vector<double> fx, fy, fz;
    const double e = ex05::privateBufferScatter(b.x, b.y, b.z, 4.0, 1.0,
                                                b.box[0], b.box[1], b.box[2],
                                                b.cutoff, fx, fy, fz, 8);
    ASSERT_LT(ref, 0.0);
    EXPECT_NEAR(e, ref, std::fabs(ref) * 1e-9);
}

// schedule(static,16), not dynamic -- otherwise the reduction reorders.
TEST(Ex05Race, PrivateBufferIsBitwiseReproducible) {
    const ex::Box b = ex::makeBox(800);
    std::vector<double> ax, ay, az, bx2, by2, bz2;
    const double e1 = ex05::privateBufferScatter(b.x, b.y, b.z, 4.0, 1.0,
                                                 b.box[0], b.box[1], b.box[2],
                                                 b.cutoff, ax, ay, az, 8);
    EX_REQUIRE_IMPLEMENTED(ax, ay, az);
    for (int run = 0; run < 5; ++run) {
        const double e2 = ex05::privateBufferScatter(b.x, b.y, b.z, 4.0, 1.0,
                                                     b.box[0], b.box[1], b.box[2],
                                                     b.cutoff, bx2, by2, bz2, 8);
        ASSERT_DOUBLE_EQ(e1, e2)
            << "run " << run << ": use schedule(static,16), not dynamic";
        for (std::size_t i = 0; i < b.n; ++i) ASSERT_DOUBLE_EQ(ax[i], bx2[i]);
    }
}

// The specimen must actually be broken. On one thread it is fine (no other
// thread to collide with), which is itself worth noticing.
TEST(Ex05Race, RacyVersionIsCorrectOnOneThread) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    std::vector<double> fx, fy, fz;
    ex05::racyScatter(b.x, b.y, b.z, 4.0, 1.0,
                      b.box[0], b.box[1], b.box[2], b.cutoff, fx, fy, fz, 1);
    ASSERT_EQ(fx.size(), b.n);
    EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9)
        << "with one thread there is nothing to race against";
}

#ifdef _OPENMP
TEST(Ex05Race, RacyVersionIsBrokenOnManyThreads) {
    const ex::Box b = ex::makeBox(4096);
    const auto ref = ex::referenceForces(b);

    // Races are probabilistic; try several times before concluding anything.
    std::size_t worst = 0;
    for (int attempt = 0; attempt < 10; ++attempt) {
        std::vector<double> fx, fy, fz;
        ex05::racyScatter(b.x, b.y, b.z, 4.0, 1.0,
                          b.box[0], b.box[1], b.box[2], b.cutoff, fx, fy, fz, 8);
        ASSERT_EQ(fx.size(), b.n);
        worst = std::max(worst, ex05::countWrongAtoms(ref, fx, fy, fz, 1e-9));
    }
    EXPECT_GT(worst, 0u)
        << "the racy kernel produced correct results in 10 attempts -- did you "
           "actually parallelise the outer loop and leave f[j] unguarded?";
}
#endif
