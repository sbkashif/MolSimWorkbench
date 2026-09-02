#include <gtest/gtest.h>

#include <cmath>

#include "scaling.h"

TEST(Ex07Scaling, AmdahlSpeedupKnownValues) {
    EXPECT_NEAR(ex07::amdahlSpeedup(0.0, 8), 8.0, 1e-12);      // perfect
    EXPECT_NEAR(ex07::amdahlSpeedup(1.0, 8), 1.0, 1e-12);      // all serial
    EXPECT_NEAR(ex07::amdahlSpeedup(0.5, 2), 1.0 / 0.75, 1e-12);
    EXPECT_NEAR(ex07::amdahlSpeedup(0.1, 4), 1.0 / (0.1 + 0.9 / 4), 1e-12);
    EXPECT_NEAR(ex07::amdahlSpeedup(0.3, 1), 1.0, 1e-12);      // one processor
}

TEST(Ex07Scaling, SerialFractionInvertsSpeedup) {
    for (double s : {0.0, 0.01, 0.024, 0.1, 0.5, 0.9}) {
        for (int n : {2, 4, 8, 64}) {
            const double sp = ex07::amdahlSpeedup(s, n);
            EXPECT_NEAR(ex07::amdahlSerialFraction(sp, n), s, 1e-12)
                << "s=" << s << " n=" << n;
        }
    }
}

// The repo's own measurement: 3.74x on 4 threads.
TEST(Ex07Scaling, RecoversRepoSerialFraction) {
    const double s = ex07::amdahlSerialFraction(3.74, 4);
    EXPECT_NEAR(s, 0.0238, 5e-4);
    EXPECT_GT(ex07::maxSpeedup(s), 30.0)
        << "with s this small, Amdahl is not what limits this program";
}

TEST(Ex07Scaling, SerialFractionIsClampedAndGuarded) {
    // A normal case must work first, otherwise a stub returning 1.0 for
    // everything would satisfy the guards below.
    ASSERT_NEAR(ex07::amdahlSerialFraction(ex07::amdahlSpeedup(0.1, 4), 4), 0.1, 1e-9)
        << "not implemented yet";

    EXPECT_DOUBLE_EQ(ex07::amdahlSerialFraction(4.0, 1), 1.0);   // no information
    const double s = ex07::amdahlSerialFraction(100.0, 4);       // superlinear
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

TEST(Ex07Scaling, MaxSpeedup) {
    EXPECT_NEAR(ex07::maxSpeedup(0.02), 50.0, 1e-9);
    EXPECT_NEAR(ex07::maxSpeedup(0.5), 2.0, 1e-9);
    EXPECT_TRUE(std::isinf(ex07::maxSpeedup(0.0)));
}

TEST(Ex07Scaling, Efficiency) {
    EXPECT_NEAR(ex07::efficiency(80.0, 10.0, 8), 1.00, 1e-12);
    EXPECT_NEAR(ex07::efficiency(28.92, 7.74, 4), 0.934, 1e-3);   // repo, 4 threads
    EXPECT_NEAR(ex07::efficiency(28.92, 6.84, 8), 0.529, 1e-3);   // repo, 8 threads
    EXPECT_DOUBLE_EQ(ex07::efficiency(10.0, 0.0, 4), 0.0);
    EXPECT_DOUBLE_EQ(ex07::efficiency(10.0, 5.0, 0), 0.0);
}

// The heart of the lesson: the baseline is the fastest SERIAL entry.
TEST(Ex07Scaling, BaselineIsFastestSerialNotFastestOverall) {
    const std::vector<ex07::Timing> t{
        {"reference", 34.85, 1},
        {"serial-triangular", 15.22, 1},   // <-- the honest baseline
        {"serial-gather", 28.29, 1},
        {"openmp-private", 3.77, 8},       // fastest overall, but parallel
    };
    const auto base = ex07::chooseBaseline(t);
    EXPECT_EQ(base.label, "serial-triangular");
    EXPECT_NEAR(base.ms, 15.22, 1e-12);
    EXPECT_NEAR(base.ms / 3.77, 4.037, 1e-3) << "the number you may quote";
}

TEST(Ex07Scaling, BaselineHandlesNoSerialEntry) {
    // Prove chooseBaseline finds a serial entry when one exists, so this test
    // cannot pass on a stub that always returns an empty Timing.
    const std::vector<ex07::Timing> ok{{"serial", 9.0, 1}, {"omp", 3.0, 8}};
    ASSERT_EQ(ex07::chooseBaseline(ok).label, "serial") << "not implemented yet";

    const std::vector<ex07::Timing> t{{"omp", 3.0, 8}, {"threads", 4.0, 4}};
    EXPECT_TRUE(ex07::chooseBaseline(t).label.empty());
}
