#include <gtest/gtest.h>

#include <cmath>

#include "tail.h"

namespace {
// Independent restatement of the closed form, for cross-checking.
double expectedEnergy(double n, double v, double eps, double sig, double rc) {
    const double rho = n / v;
    const double sr3 = std::pow(sig / rc, 3.0);
    const double sr9 = std::pow(sig / rc, 9.0);
    return (8.0 / 3.0) * M_PI * n * rho * eps * sig * sig * sig * (sr9 / 3.0 - sr3);
}
}  // namespace

TEST(Ex08Tail, EnergyMatchesTheClosedForm) {
    const double v = 4096.0 / 0.8;
    EXPECT_NEAR(ex08::tailEnergy(4096, v, 1.0, 1.0, 2.5),
                expectedEnergy(4096, v, 1.0, 1.0, 2.5), 1e-9);
}

TEST(Ex08Tail, EnergyIsNegative) {
    const double v = 4096.0 / 0.8;
    const double e = ex08::tailEnergy(4096, v, 1.0, 1.0, 2.5);
    EXPECT_LT(e, 0.0) << "the truncated dispersion tail is attractive";
    EXPECT_NEAR(e, -1754.5, 1.0) << "expected roughly -1754.5 at rho=0.8, rc=2.5";
}

// E_tail scales linearly with N at fixed density.
TEST(Ex08Tail, EnergyScalesLinearlyWithNAtFixedDensity) {
    const double rho = 0.8;
    const double e1 = ex08::tailEnergy(1000, 1000.0 / rho, 1.0, 1.0, 2.5);
    const double e2 = ex08::tailEnergy(2000, 2000.0 / rho, 1.0, 1.0, 2.5);
    ASSERT_LT(e1, 0.0);
    EXPECT_NEAR(e2 / e1, 2.0, 1e-9);
}

// P_tail depends only on density and cutoff -- not on N.
TEST(Ex08Tail, PressureIsIndependentOfSystemSize) {
    const double rho = 0.8;
    const double p1 = ex08::tailPressure(512, 512.0 / rho, 1.0, 1.0, 2.5);
    const double p2 = ex08::tailPressure(8000, 8000.0 / rho, 1.0, 1.0, 2.5);
    ASSERT_NE(p1, 0.0);
    EXPECT_NEAR(p1, p2, std::fabs(p1) * 1e-12)
        << "P_tail is a function of rho and rc only -- check for a stray N";
    EXPECT_NEAR(p1, -0.6844, 1e-3);
}

// A larger cutoff truncates less, so the correction shrinks toward zero.
TEST(Ex08Tail, LargerCutoffMeansSmallerCorrection) {
    const double v = 4096.0 / 0.8;
    const double at25 = std::fabs(ex08::tailEnergy(4096, v, 1.0, 1.0, 2.5));
    const double at50 = std::fabs(ex08::tailEnergy(4096, v, 1.0, 1.0, 5.0));
    EXPECT_LT(at50, at25);
    EXPECT_GT(at25, 0.0);
}

TEST(Ex08Tail, GuardsNonPhysicalInput) {
    // Valid input must produce a real answer first; otherwise a stub that
    // returns 0.0 unconditionally would satisfy every assertion below.
    ASSERT_LT(ex08::tailEnergy(100, 125.0, 1.0, 1.0, 2.5), 0.0) << "not implemented yet";
    ASSERT_NE(ex08::tailPressure(100, 125.0, 1.0, 1.0, 2.5), 0.0) << "not implemented yet";

    EXPECT_DOUBLE_EQ(ex08::tailEnergy(0, 100.0, 1.0, 1.0, 2.5), 0.0);
    EXPECT_DOUBLE_EQ(ex08::tailEnergy(100, 0.0, 1.0, 1.0, 2.5), 0.0);
    EXPECT_DOUBLE_EQ(ex08::tailEnergy(100, -5.0, 1.0, 1.0, 2.5), 0.0);
    EXPECT_DOUBLE_EQ(ex08::tailEnergy(100, 100.0, 1.0, 1.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(ex08::tailPressure(100, 0.0, 1.0, 1.0, 2.5), 0.0);
}

// The headline result of lesson 8.
TEST(Ex08Tail, ForceIsExactlyZero) {
    for (std::size_t n : {1u, 100u, 4096u}) {
        for (double rc : {2.0, 2.5, 5.0}) {
            EXPECT_DOUBLE_EQ(ex08::tailForceMagnitude(n, n / 0.8, 1.0, 1.0, rc), 0.0)
                << "n=" << n << " rc=" << rc
                << ": a uniform isotropic medium exerts no net force";
        }
    }
}
