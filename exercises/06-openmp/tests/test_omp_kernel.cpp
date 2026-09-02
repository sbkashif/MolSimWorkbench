#include <gtest/gtest.h>

#include <cmath>

#include "exercise_support.h"
#include "omp_kernel.h"

namespace {
const ex06::Schedule kAll[] = {ex06::Schedule::Static, ex06::Schedule::StaticChunk,
                               ex06::Schedule::Dynamic, ex06::Schedule::Guided};
const char* schedName(ex06::Schedule s) {
    switch (s) {
        case ex06::Schedule::Static: return "static";
        case ex06::Schedule::StaticChunk: return "static,16";
        case ex06::Schedule::Dynamic: return "dynamic,32";
        case ex06::Schedule::Guided: return "guided";
    }
    return "?";
}
}  // namespace

TEST(Ex06OpenMP, AllSchedulesMatchTheReference) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    for (ex06::Schedule s : kAll) {
        std::vector<double> fx, fy, fz;
        ex06::ompGather(b.x, b.y, b.z, 4.0, 1.0, b.box[0], b.box[1], b.box[2],
                        b.cutoff, fx, fy, fz, 4, s);
        ASSERT_EQ(fx.size(), b.n) << schedName(s);
        EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9) << schedName(s);
    }
}

TEST(Ex06OpenMP, EnergyMatchesTheReference) {
    const ex::Box b = ex::makeBox(600);
    const double ref = ex::referenceEnergy(b);
    ASSERT_LT(ref, 0.0);
    for (ex06::Schedule s : kAll) {
        std::vector<double> fx, fy, fz;
        const double e = ex06::ompGather(b.x, b.y, b.z, 4.0, 1.0, b.box[0],
                                         b.box[1], b.box[2], b.cutoff,
                                         fx, fy, fz, 4, s);
        EXPECT_NEAR(e, ref, std::fabs(ref) * 1e-9)
            << schedName(s) << " (ratio " << e / ref << ")";
    }
}

TEST(Ex06OpenMP, ThreadCountDoesNotChangeForces) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    for (int T : {1, 2, 3, 5, 8}) {
        std::vector<double> fx, fy, fz;
        ex06::ompGather(b.x, b.y, b.z, 4.0, 1.0, b.box[0], b.box[1], b.box[2],
                        b.cutoff, fx, fy, fz, T, ex06::Schedule::Guided);
        EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9) << "T=" << T;
    }
}

TEST(Ex06OpenMP, DeterministicGatherIsBitwiseReproducible) {
    const ex::Box b = ex::makeBox(800);
    std::vector<double> ax, ay, az, bx2, by2, bz2;
    const double e1 = ex06::deterministicGather(b.x, b.y, b.z, 4.0, 1.0, b.box[0],
                                                b.box[1], b.box[2], b.cutoff,
                                                ax, ay, az, 8);
    for (int run = 0; run < 5; ++run) {
        const double e2 = ex06::deterministicGather(b.x, b.y, b.z, 4.0, 1.0,
                                                    b.box[0], b.box[1], b.box[2],
                                                    b.cutoff, bx2, by2, bz2, 8);
        ASSERT_DOUBLE_EQ(e1, e2) << "run " << run
            << ": sum the per-thread partials in index order";
    }
}

TEST(Ex06OpenMP, DeterministicGatherIsAlsoCorrect) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    const double refE = ex::referenceEnergy(b);
    std::vector<double> fx, fy, fz;
    const double e = ex06::deterministicGather(b.x, b.y, b.z, 4.0, 1.0, b.box[0],
                                               b.box[1], b.box[2], b.cutoff,
                                               fx, fy, fz, 8);
    ASSERT_EQ(fx.size(), b.n);
    EXPECT_LT(ex::maxRelError(ref, fx, fy, fz), 1e-9);
    EXPECT_NEAR(e, refE, std::fabs(refE) * 1e-9);
}
