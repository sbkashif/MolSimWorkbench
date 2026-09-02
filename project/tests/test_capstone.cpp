// ---------------------------------------------------------------------------
// Capstone acceptance tests. One or more per functional requirement in
// project/README.md. All of these fail against the scaffold.
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <vector>

#include "exercise_support.h"
#include "student_kernel.h"

namespace {

// Holds the arrays an LJKernelView points into.
struct View {
    std::vector<double> fx, fy, fz;
    std::vector<double> eps4{4.0}, sigma2{1.0};
    LJKernelView v;
};

View makeView(const ex::Box& b) {
    View w;
    w.fx.assign(b.n, 0.0);
    w.fy.assign(b.n, 0.0);
    w.fz.assign(b.n, 0.0);
    w.v.x = b.x.data();  w.v.y = b.y.data();  w.v.z = b.z.data();
    w.v.type = b.type.data();
    w.v.fx = w.fx.data(); w.v.fy = w.fy.data(); w.v.fz = w.fz.data();
    w.v.n = b.n;
    w.v.eps4 = w.eps4.data();
    w.v.sigma2 = w.sigma2.data();
    w.v.ntypes = 1;
    w.v.bx = b.box[0]; w.v.by = b.box[1]; w.v.bz = b.box[2];
    w.v.cutoff2 = b.cutoff * b.cutoff;
    return w;
}

const StudentBackend kAll[] = {StudentBackend::Triangular, StudentBackend::Gather,
                               StudentBackend::Parallel,
                               StudentBackend::ParallelDeterministic};
const char* name(StudentBackend b) {
    switch (b) {
        case StudentBackend::Triangular: return "Triangular";
        case StudentBackend::Gather: return "Gather";
        case StudentBackend::Parallel: return "Parallel";
        case StudentBackend::ParallelDeterministic: return "ParallelDeterministic";
    }
    return "?";
}

double timeMs(StudentBackend backend, const ex::Box& b, int threads, int reps = 5) {
    View w = makeView(b);
    studentCompute(w.v, backend, threads);  // warm-up
    double best = 1e300;
    for (int r = 0; r < reps; ++r) {
        const auto t0 = std::chrono::high_resolution_clock::now();
        studentCompute(w.v, backend, threads);
        const auto t1 = std::chrono::high_resolution_clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return best;
}

}  // namespace

// R2: forces match the reference, every backend.
TEST(Capstone, AllBackendsMatchReferenceForces) {
    const ex::Box b = ex::makeBox(600);
    const auto ref = ex::referenceForces(b);
    for (StudentBackend backend : kAll) {
        View w = makeView(b);
        studentCompute(w.v, backend, 4);
        EXPECT_LT(ex::maxRelError(ref, w.fx, w.fy, w.fz), 1e-9) << name(backend);
    }
}

// R3: energy, each pair counted once.
TEST(Capstone, AllBackendsMatchReferenceEnergy) {
    const ex::Box b = ex::makeBox(600);
    const double ref = ex::referenceEnergy(b);
    ASSERT_LT(ref, 0.0);
    for (StudentBackend backend : kAll) {
        View w = makeView(b);
        const double e = studentCompute(w.v, backend, 4);
        EXPECT_NEAR(e, ref, std::fabs(ref) * 1e-9)
            << name(backend) << " (ratio " << e / ref
            << " -- 2.0 or 0.5 means a double-counting error)";
    }
}

// R4: thread count must not change the answer.
TEST(Capstone, ParallelIsIndependentOfThreadCount) {
    const ex::Box b = ex::makeBox(800);
    View base = makeView(b);
    studentCompute(base.v, StudentBackend::Parallel, 1);

    for (int T : {2, 3, 5, 8, 16}) {
        View w = makeView(b);
        studentCompute(w.v, StudentBackend::Parallel, T);
        for (std::size_t i = 0; i < b.n; ++i) {
            ASSERT_NEAR(w.fx[i], base.fx[i], 1e-10) << "T=" << T << " atom " << i;
            ASSERT_NEAR(w.fy[i], base.fy[i], 1e-10) << "T=" << T << " atom " << i;
            ASSERT_NEAR(w.fz[i], base.fz[i], 1e-10) << "T=" << T << " atom " << i;
        }
    }
}

// R5: bitwise reproducibility for the deterministic backend.
TEST(Capstone, DeterministicBackendIsBitwiseReproducible) {
    const ex::Box b = ex::makeBox(800);
    View first = makeView(b);
    const double e1 = studentCompute(first.v, StudentBackend::ParallelDeterministic, 8);

    for (int run = 0; run < 5; ++run) {
        View w = makeView(b);
        const double e2 = studentCompute(w.v, StudentBackend::ParallelDeterministic, 8);
        ASSERT_DOUBLE_EQ(e1, e2) << "run " << run
            << ": use a fixed partition AND a fixed-order reduction";
        for (std::size_t i = 0; i < b.n; ++i) {
            ASSERT_DOUBLE_EQ(w.fx[i], first.fx[i]) << "run " << run << " atom " << i;
        }
    }
}

// R7: empty system.
TEST(Capstone, HandlesEmptySystem) {
    ex::Box b;
    b.n = 0;
    View w = makeView(b);
    for (StudentBackend backend : kAll) {
        EXPECT_DOUBLE_EQ(studentCompute(w.v, backend, 4), 0.0) << name(backend);
    }
}

// R7: a single atom has no partners and must not self-interact.
TEST(Capstone, HandlesSingleAtom) {
    ex::Box b;
    b.n = 1;
    b.x = {0.0}; b.y = {0.0}; b.z = {0.0}; b.type = {0};
    b.box = {10.0, 10.0, 10.0};
    for (StudentBackend backend : kAll) {
        View w = makeView(b);
        const double e = studentCompute(w.v, backend, 4);
        EXPECT_DOUBLE_EQ(e, 0.0) << name(backend);
        EXPECT_DOUBLE_EQ(w.fx[0], 0.0) << name(backend);
        EXPECT_FALSE(std::isnan(w.fx[0])) << name(backend);
    }
}

// R7: box length 0 means no periodicity, and must not divide by zero.
TEST(Capstone, HandlesNonPeriodicAxes) {
    ex::Box b;
    b.n = 2;
    b.x = {0.0, 1.2}; b.y = {0.0, 0.0}; b.z = {0.0, 0.0}; b.type = {0, 0};
    b.box = {0.0, 0.0, 0.0};
    for (StudentBackend backend : kAll) {
        View w = makeView(b);
        const double e = studentCompute(w.v, backend, 4);
        EXPECT_FALSE(std::isnan(e)) << name(backend);
        EXPECT_LT(e, 0.0) << name(backend) << ": two atoms at 1.2 sigma attract";
        EXPECT_NEAR(w.fx[0], -w.fx[1], 1e-12) << name(backend);
    }
}

// R8: net force vanishes over the periodic system.
TEST(Capstone, NetForceIsZero) {
    const ex::Box b = ex::makeBox(500);
    for (StudentBackend backend : kAll) {
        View w = makeView(b);
        studentCompute(w.v, backend, 4);
        double sx = 0, sy = 0, sz = 0, mag = 0;
        for (std::size_t i = 0; i < b.n; ++i) {
            sx += w.fx[i]; sy += w.fy[i]; sz += w.fz[i];
            mag += std::fabs(w.fx[i]) + std::fabs(w.fy[i]) + std::fabs(w.fz[i]);
        }
        ASSERT_GT(mag, 0.0) << name(backend) << " produced no forces at all";
        EXPECT_LT(std::fabs(sx) / mag, 1e-12) << name(backend);
        EXPECT_LT(std::fabs(sy) / mag, 1e-12) << name(backend);
        EXPECT_LT(std::fabs(sz) / mag, 1e-12) << name(backend);
    }
}

// Integration: backends must agree with EACH OTHER, not just the reference.
TEST(Capstone, BackendsAgreeWithEachOther) {
    const ex::Box b = ex::makeBox(700);
    View tri = makeView(b);
    studentCompute(tri.v, StudentBackend::Triangular, 1);
    for (StudentBackend backend : {StudentBackend::Gather, StudentBackend::Parallel,
                                   StudentBackend::ParallelDeterministic}) {
        View w = makeView(b);
        studentCompute(w.v, backend, 8);
        for (std::size_t i = 0; i < b.n; ++i) {
            const double scale = std::max(std::fabs(tri.fx[i]), 1e-30);
            ASSERT_LT(std::fabs(w.fx[i] - tri.fx[i]) / scale, 1e-9)
                << name(backend) << " disagrees with Triangular at atom " << i;
        }
    }
}

// R9: the performance bar. 2x over your OWN triangular kernel at 8 threads.
TEST(Capstone, ParallelIsAtLeastTwiceTheSerialTriangular) {
    const ex::Box b = ex::makeBox(4096);
    const double tri = timeMs(StudentBackend::Triangular, b, 1);
    const double par = timeMs(StudentBackend::Parallel, b, 8);
    ASSERT_GT(tri, 0.0);
    ASSERT_GT(par, 0.0);
    const double speedup = tri / par;
    EXPECT_GE(speedup, 2.0)
        << "Parallel at 8 threads: " << par << " ms vs Triangular " << tri
        << " ms = " << speedup << "x. The bar is 2.0x "
        << "(the repo's shipped kernel manages 4.03x).";
}

// R10: the tail correction, checked against the closed form.
TEST(Capstone, TailEnergyMatchesTheClosedForm) {
    const double v = 4096.0 / 0.8;
    const double rho = 4096.0 / v;
    const double sr3 = std::pow(1.0 / 2.5, 3.0);
    const double sr9 = sr3 * sr3 * sr3;
    const double expected =
        (8.0 / 3.0) * M_PI * 4096.0 * rho * 1.0 * 1.0 * (sr9 / 3.0 - sr3);
    EXPECT_NEAR(studentTailEnergy(4096, v, 1.0, 1.0, 2.5), expected, 1e-9);
    EXPECT_LT(studentTailEnergy(4096, v, 1.0, 1.0, 2.5), 0.0);
}

TEST(Capstone, TailEnergyGuardsNonPhysicalInput) {
    EXPECT_DOUBLE_EQ(studentTailEnergy(0, 100.0, 1.0, 1.0, 2.5), 0.0);
    EXPECT_DOUBLE_EQ(studentTailEnergy(100, 0.0, 1.0, 1.0, 2.5), 0.0);
    EXPECT_DOUBLE_EQ(studentTailEnergy(100, 100.0, 1.0, 1.0, 0.0), 0.0);
}

// R10: and it must never touch a force.
TEST(Capstone, TailCorrectionDoesNotEnterTheKernel) {
    const ex::Box b = ex::makeBox(500);
    View w = makeView(b);
    studentCompute(w.v, StudentBackend::Triangular, 1);
    const auto ref = ex::referenceForces(b);
    // If the tail correction had leaked into the kernel, the forces would no
    // longer match the reference, which has no tail correction at all.
    EXPECT_LT(ex::maxRelError(ref, w.fx, w.fy, w.fz), 1e-9)
        << "the tail correction must contribute exactly zero force";
}
