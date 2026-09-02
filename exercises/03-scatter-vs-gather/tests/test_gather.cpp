#include <gtest/gtest.h>

#include <cmath>

#include "exercise_support.h"
#include "gather.h"

TEST(Ex03Gather, ForcesMatchTheReference) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    std::vector<double> fx, fy, fz;
    ex03::computeGather(b.x, b.y, b.z, 4.0, 1.0,
                        b.box[0], b.box[1], b.box[2], b.cutoff, fx, fy, fz);
    ASSERT_EQ(fx.size(), b.n);
    EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9);
}

// The classic gather mistake is forgetting the factor of 1/2.
TEST(Ex03Gather, EnergyIsNotDoubleCounted) {
    const ex::Box b = ex::makeBox(600);
    const double ref = ex::referenceEnergy(b);
    std::vector<double> fx, fy, fz;
    const double e = ex03::computeGather(b.x, b.y, b.z, 4.0, 1.0,
                                         b.box[0], b.box[1], b.box[2], b.cutoff,
                                         fx, fy, fz);
    ASSERT_LT(ref, 0.0);
    EXPECT_NEAR(e, ref, std::fabs(ref) * 1e-9)
        << "got " << e << ", reference " << ref
        << " (ratio " << e / ref << " -- 2.0 means you forgot to halve it)";
}

TEST(Ex03Gather, NetForceIsZero) {
    const ex::Box b = ex::makeBox(500);
    std::vector<double> fx, fy, fz;
    ex03::computeGather(b.x, b.y, b.z, 4.0, 1.0,
                        b.box[0], b.box[1], b.box[2], b.cutoff, fx, fy, fz);
    double sx = 0, sy = 0, sz = 0, mag = 0;
    for (std::size_t i = 0; i < b.n; ++i) {
        sx += fx[i]; sy += fy[i]; sz += fz[i];
        mag += std::fabs(fx[i]) + std::fabs(fy[i]) + std::fabs(fz[i]);
    }
    ASSERT_GT(mag, 0.0);
    EXPECT_LT(std::fabs(sx)/mag, 1e-12);
    EXPECT_LT(std::fabs(sy)/mag, 1e-12);
    EXPECT_LT(std::fabs(sz)/mag, 1e-12);
}

// Two isolated atoms: forces must be equal and opposite.
TEST(Ex03Gather, TwoAtomsObeyNewtonsThirdLaw) {
    std::vector<double> x{0.0, 1.2}, y{0.0, 0.0}, z{0.0, 0.0}, fx, fy, fz;
    const double e = ex03::computeGather(x, y, z, 4.0, 1.0, 0, 0, 0, 2.5, fx, fy, fz);
    ASSERT_EQ(fx.size(), 2u);
    EXPECT_NEAR(fx[0], -fx[1], 1e-12);
    EXPECT_LT(e, 0.0);
    EXPECT_FALSE(std::isnan(e));
}

TEST(Ex03Gather, SelfInteractionIsSkipped) {
    std::vector<double> x{0.0}, y{0.0}, z{0.0}, fx, fy, fz;
    const double e = ex03::computeGather(x, y, z, 4.0, 1.0, 0, 0, 0, 2.5, fx, fy, fz);
    EXPECT_DOUBLE_EQ(e, 0.0);
    ASSERT_EQ(fx.size(), 1u);
    EXPECT_DOUBLE_EQ(fx[0], 0.0) << "an atom must not interact with itself";
    EXPECT_FALSE(std::isnan(fx[0]));
}

TEST(Ex03Gather, ArithmeticRatioIsExactlyTwo) {
    for (std::size_t n : {2u, 10u, 4096u}) {
        EXPECT_NEAR(ex03::gatherArithmeticRatio(n), 2.0, 1e-12) << "n=" << n;
    }
}
