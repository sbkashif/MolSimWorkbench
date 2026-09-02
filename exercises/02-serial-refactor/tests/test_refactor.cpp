#include <gtest/gtest.h>

#include <cmath>

#include "exercise_support.h"
#include "refactor.h"

TEST(Ex02Refactor, MixingTableGeometric) {
    const auto t = ex02::buildMixingTable({1.0, 4.0}, {1.0, 9.0}, "geometric");
    ASSERT_EQ(t.ntypes, 2);
    ASSERT_EQ(t.eps4.size(), 4u);

    EXPECT_DOUBLE_EQ(t.eps4[0], 4.0 * 1.0);                 // (0,0)
    EXPECT_DOUBLE_EQ(t.eps4[3], 4.0 * 4.0);                 // (1,1)
    EXPECT_DOUBLE_EQ(t.eps4[1], 4.0 * std::sqrt(1.0 * 4.0));// (0,1) mixed
    EXPECT_DOUBLE_EQ(t.eps4[2], t.eps4[1]) << "table must be symmetric";

    EXPECT_DOUBLE_EQ(t.sigma2[0], 1.0);
    EXPECT_DOUBLE_EQ(t.sigma2[3], 81.0);
    const double mixedSigma = std::sqrt(1.0 * 9.0);
    EXPECT_DOUBLE_EQ(t.sigma2[1], mixedSigma * mixedSigma);
}

TEST(Ex02Refactor, MixingTableArithmetic) {
    const auto t = ex02::buildMixingTable({1.0, 3.0}, {2.0, 4.0}, "arithmetic");
    ASSERT_EQ(t.ntypes, 2);
    EXPECT_DOUBLE_EQ(t.eps4[1], 4.0 * 2.0);   // (1+3)/2 = 2
    EXPECT_DOUBLE_EQ(t.sigma2[1], 9.0);       // ((2+4)/2)^2 = 9
}

TEST(Ex02Refactor, MixingTableUnknownRuleIsZero) {
    const auto t = ex02::buildMixingTable({1.0}, {1.0}, "nonsense");
    ASSERT_EQ(t.eps4.size(), 1u);
    EXPECT_DOUBLE_EQ(t.eps4[0], 0.0);
    EXPECT_DOUBLE_EQ(t.sigma2[0], 0.0);
}

TEST(Ex02Refactor, ForcesMatchTheReference) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);

    const auto table = ex02::buildMixingTable({b.epsilon}, {b.sigma}, "geometric");
    std::vector<double> fx, fy, fz;
    ex02::computeTriangularFast(b.x, b.y, b.z, b.type, table,
                                b.box[0], b.box[1], b.box[2], b.cutoff, fx, fy, fz);

    ASSERT_EQ(fx.size(), b.n);
    EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9);
}

TEST(Ex02Refactor, EnergyMatchesTheReference) {
    const ex::Box b = ex::makeBox(600);
    const double ref = ex::referenceEnergy(b);

    const auto table = ex02::buildMixingTable({b.epsilon}, {b.sigma}, "geometric");
    std::vector<double> fx, fy, fz;
    const double e = ex02::computeTriangularFast(b.x, b.y, b.z, b.type, table,
                                                 b.box[0], b.box[1], b.box[2],
                                                 b.cutoff, fx, fy, fz);
    ASSERT_LT(ref, 0.0);
    EXPECT_NEAR(e, ref, std::fabs(ref) * 1e-9);
}

// A box length of 0 means no periodicity on that axis -- and must not produce
// NaN from a division by zero.
TEST(Ex02Refactor, HandlesNonPeriodicAxes) {
    const auto table = ex02::buildMixingTable({1.0}, {1.0}, "geometric");
    std::vector<double> x{0.0, 1.1}, y{0.0, 0.0}, z{0.0, 0.0};
    std::vector<int> type{0, 0};
    std::vector<double> fx, fy, fz;

    const double e = ex02::computeTriangularFast(x, y, z, type, table,
                                                 0.0, 0.0, 0.0, 2.5, fx, fy, fz);
    EXPECT_FALSE(std::isnan(e));
    for (std::size_t i = 0; i < 2; ++i) {
        EXPECT_FALSE(std::isnan(fx[i]));
        EXPECT_FALSE(std::isnan(fy[i]));
        EXPECT_FALSE(std::isnan(fz[i]));
    }
    EXPECT_LT(e, 0.0) << "two atoms at r=1.1 sigma should attract";
    EXPECT_NEAR(fx[0] + fx[1], 0.0, 1e-12) << "Newton's third law";
}

// Forces must sum to zero over the whole periodic system.
TEST(Ex02Refactor, NetForceIsZero) {
    const ex::Box b = ex::makeBox(500);
    const auto table = ex02::buildMixingTable({b.epsilon}, {b.sigma}, "geometric");
    std::vector<double> fx, fy, fz;
    ex02::computeTriangularFast(b.x, b.y, b.z, b.type, table,
                                b.box[0], b.box[1], b.box[2], b.cutoff, fx, fy, fz);

    double sx = 0, sy = 0, sz = 0, mag = 0;
    for (std::size_t i = 0; i < b.n; ++i) {
        sx += fx[i]; sy += fy[i]; sz += fz[i];
        mag += std::fabs(fx[i]) + std::fabs(fy[i]) + std::fabs(fz[i]);
    }
    ASSERT_GT(mag, 0.0) << "kernel produced no forces at all";
    EXPECT_LT(std::fabs(sx)/mag, 1e-12);
    EXPECT_LT(std::fabs(sy)/mag, 1e-12);
    EXPECT_LT(std::fabs(sz)/mag, 1e-12);
}
