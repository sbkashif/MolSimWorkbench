#include <gtest/gtest.h>

#include <cmath>
#include <set>

#include "exercise_support.h"
#include "pool.h"

// The partition must tile [0,n) exactly -- no gaps, no overlaps, nothing lost.
TEST(Ex04Pool, PartitionTilesTheRangeExactly) {
    for (std::size_t n : {1u, 7u, 10u, 4096u}) {
        for (int T : {1, 2, 3, 5, 7, 8}) {
            std::vector<char> covered(n, 0);
            for (int t = 0; t < T; ++t) {
                const auto [b, e] = ex04::partitionRange(n, T, t);
                ASSERT_LE(b, e) << "n=" << n << " T=" << T << " t=" << t;
                ASSERT_LE(e, n);
                for (std::size_t i = b; i < e; ++i) {
                    EXPECT_EQ(covered[i], 0) << "index " << i << " covered twice";
                    covered[i] = 1;
                }
            }
            for (std::size_t i = 0; i < n; ++i)
                EXPECT_EQ(covered[i], 1)
                    << "index " << i << " never covered (n=" << n << " T=" << T
                    << ") -- did you distribute the remainder?";
        }
    }
}

// n=4096, T=7 is the case the naive split gets wrong.
TEST(Ex04Pool, PartitionIsBalanced) {
    const std::size_t n = 4096;
    const int T = 7;
    std::size_t total = 0, smallest = n, largest = 0;
    for (int t = 0; t < T; ++t) {
        const auto [b, e] = ex04::partitionRange(n, T, t);
        const std::size_t c = e - b;
        total += c;
        smallest = std::min(smallest, c);
        largest = std::max(largest, c);
    }
    EXPECT_EQ(total, n);
    EXPECT_LE(largest - smallest, 1u) << "chunks differ by more than one item";
}

TEST(Ex04Pool, ForcesMatchTheReference) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    std::vector<double> fx, fy, fz;
    ex04::parallelGather(b.x, b.y, b.z, 4.0, 1.0,
                         b.box[0], b.box[1], b.box[2], b.cutoff, fx, fy, fz, 4);
    ASSERT_EQ(fx.size(), b.n);
    EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9);
}

// Thread count must not change the answer. If it does, there is a race.
TEST(Ex04Pool, ResultIsIndependentOfThreadCount) {
    const ex::Box b = ex::makeBox(600);
    std::vector<double> rx, ry, rz;
    const double refE = ex04::parallelGather(b.x, b.y, b.z, 4.0, 1.0,
                                             b.box[0], b.box[1], b.box[2],
                                             b.cutoff, rx, ry, rz, 1);
    EX_REQUIRE_IMPLEMENTED(rx, ry, rz);
    for (int T : {2, 3, 5, 8, 16}) {
        std::vector<double> fx, fy, fz;
        const double e = ex04::parallelGather(b.x, b.y, b.z, 4.0, 1.0,
                                              b.box[0], b.box[1], b.box[2],
                                              b.cutoff, fx, fy, fz, T);
        EXPECT_NEAR(e, refE, std::fabs(refE) * 1e-12) << "T=" << T;
        for (std::size_t i = 0; i < b.n; ++i) {
            ASSERT_NEAR(fx[i], rx[i], 1e-10) << "T=" << T << " atom " << i;
            ASSERT_NEAR(fy[i], ry[i], 1e-10) << "T=" << T << " atom " << i;
            ASSERT_NEAR(fz[i], rz[i], 1e-10) << "T=" << T << " atom " << i;
        }
    }
}

// Fixed-order reduction => bitwise reproducibility.
TEST(Ex04Pool, RepeatedRunsAreBitwiseIdentical) {
    const ex::Box b = ex::makeBox(600);
    std::vector<double> ax, ay, az, bx2, by2, bz2;
    const double e1 = ex04::parallelGather(b.x, b.y, b.z, 4.0, 1.0,
                                           b.box[0], b.box[1], b.box[2],
                                           b.cutoff, ax, ay, az, 8);
    EX_REQUIRE_IMPLEMENTED(ax, ay, az);
    const double e2 = ex04::parallelGather(b.x, b.y, b.z, 4.0, 1.0,
                                           b.box[0], b.box[1], b.box[2],
                                           b.cutoff, bx2, by2, bz2, 8);
    EXPECT_DOUBLE_EQ(e1, e2) << "sum partial[] in index order, not completion order";
    for (std::size_t i = 0; i < b.n; ++i) EXPECT_DOUBLE_EQ(ax[i], bx2[i]);
}

TEST(Ex04Pool, EnergyMatchesTheReference) {
    const ex::Box b = ex::makeBox(600);
    const double ref = ex::referenceEnergy(b);
    std::vector<double> fx, fy, fz;
    const double e = ex04::parallelGather(b.x, b.y, b.z, 4.0, 1.0,
                                          b.box[0], b.box[1], b.box[2],
                                          b.cutoff, fx, fy, fz, 8);
    ASSERT_LT(ref, 0.0);
    EXPECT_NEAR(e, ref, std::fabs(ref) * 1e-9);
}
