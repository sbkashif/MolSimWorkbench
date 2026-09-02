#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "timing.h"

// The warm-up must happen, and must not be counted.
TEST(Ex01Timing, CallsFunctionRepsPlusOneTimes) {
    for (int reps : {1, 3, 7}) {
        int calls = 0;
        ex01::timeMs([&] { ++calls; }, reps);
        EXPECT_EQ(calls, reps + 1)
            << "expected " << reps << " timed calls plus one warm-up";
    }
}

// reps < 1 still means one timed call (plus the warm-up).
TEST(Ex01Timing, ClampsRepsToAtLeastOne) {
    for (int reps : {0, -5}) {
        int calls = 0;
        ex01::timeMs([&] { ++calls; }, reps);
        EXPECT_EQ(calls, 2) << "reps=" << reps << " should behave as reps=1";
    }
}

// The returned value must be the MINIMUM, not the mean or the last.
// The first timed call sleeps far longer than the rest, so a mean would be
// dragged well above the minimum.
TEST(Ex01Timing, ReturnsMinimumNotMean) {
    int call = 0;
    const double ms = ex01::timeMs(
        [&] {
            ++call;
            if (call == 2) std::this_thread::sleep_for(std::chrono::milliseconds(40));
            else std::this_thread::sleep_for(std::chrono::milliseconds(2));
        },
        4);
    EXPECT_GT(ms, 0.0);
    EXPECT_LT(ms, 20.0) << "returned " << ms << " ms; a mean would be ~11 ms, "
                        << "the minimum should be ~2 ms";
}

TEST(Ex01Timing, MeasuresRealElapsedTime) {
    const double ms = ex01::timeMs(
        [] { std::this_thread::sleep_for(std::chrono::milliseconds(15)); }, 2);
    EXPECT_GT(ms, 10.0);
    EXPECT_LT(ms, 100.0);
}

TEST(Ex01Timing, Speedup) {
    EXPECT_DOUBLE_EQ(ex01::speedup(10.0, 2.0), 5.0);
    EXPECT_DOUBLE_EQ(ex01::speedup(4.0, 8.0), 0.5);
    EXPECT_DOUBLE_EQ(ex01::speedup(10.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(ex01::speedup(10.0, -1.0), 0.0);
}

TEST(Ex01Timing, ParallelEfficiency) {
    EXPECT_DOUBLE_EQ(ex01::parallelEfficiency(80.0, 10.0, 8), 1.0);   // perfect
    EXPECT_DOUBLE_EQ(ex01::parallelEfficiency(80.0, 20.0, 8), 0.5);   // half
    EXPECT_DOUBLE_EQ(ex01::parallelEfficiency(80.0, 0.0, 8), 0.0);
    EXPECT_DOUBLE_EQ(ex01::parallelEfficiency(80.0, 10.0, 0), 0.0);
}
