// ---------------------------------------------------------------------------
// bench_lj — the measurement harness for docs/learn-parallel/
//
// Compares the reference (original triangular) implementation against the
// three lj_kernels backends, on both CORRECTNESS and SPEED.
//
// Usage:
//   ./build/benchmarks/bench_lj [N] [reps]
//   ./build/benchmarks/bench_lj 4096 5
//
// Lesson 1 is about why every part of this file is here: the warm-up call,
// the repetition count, the min-not-mean, and the correctness check that runs
// BEFORE any timing is reported.
// ---------------------------------------------------------------------------
#include "builder/lj_particle_box_builder.h"
#include "engine/potentials/lj_potential.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
using Forces = std::vector<std::array<double, 3>>;

namespace {

struct Deviation {
    double maxAbs = 0.0;
    double maxRel = 0.0;
    bool bitwiseIdentical = true;
};

Deviation compare(const Forces& a, const Forces& b) {
    Deviation d;
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (int k = 0; k < 3; ++k) {
            if (a[i][k] != b[i][k]) d.bitwiseIdentical = false;
            const double diff = std::fabs(a[i][k] - b[i][k]);
            const double scale = std::max(std::fabs(a[i][k]), 1e-30);
            d.maxAbs = std::max(d.maxAbs, diff);
            d.maxRel = std::max(d.maxRel, diff / scale);
        }
    }
    return d;
}

// Time a callable. Returns the MINIMUM over `reps` runs, not the mean.
//
// Why minimum: the OS, other processes, and CPU frequency scaling can only
// ever make a run SLOWER, never faster. The fastest observed run is therefore
// the closest estimate of the true cost. Means are dragged around by outliers.
template <typename F>
double timeMs(F&& f, int reps) {
    f();  // warm-up: the first call pays page faults, cold caches and thread
          // creation. Timing it would measure startup, not the kernel.
    double best = 1e300;
    for (int r = 0; r < reps; ++r) {
        const auto t0 = Clock::now();
        f();
        const auto t1 = Clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return best;
}

const char* backendName(LJBackend b) {
    switch (b) {
        case LJBackend::SerialTriangular: return "serial-triangular";
        case LJBackend::Serial:  return "serial-gather";
        case LJBackend::Threads: return "std::thread";
        case LJBackend::OpenMP:  return "openmp-gather";
        case LJBackend::OpenMPPrivate: return "openmp-private";
    }
    return "?";
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t N = (argc > 1) ? std::stoul(argv[1]) : 4096;
    const int reps = (argc > 2) ? std::stoi(argv[2]) : 5;

    LJBoxConfig cfg;
    cfg.numParticles = N;
    cfg.density = 0.8;
    cfg.randomSeed = 42;
    std::array<double, 3> box{};
    Geometry geometry = LJParticleBoxBuilder::createCubicLattice(cfg, box);
    const std::size_t n = geometry.getNumberOfAtoms();

    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);

    printf("\n");
    printf("N = %zu   rho = %.2f   L = %.3f   cutoff = 2.5   reps = %d\n",
           n, cfg.density, box[0], reps);
    printf("OpenMP: %s", ljOpenMPAvailable() ? "yes" : "no");
    if (ljOpenMPAvailable()) printf("  (default threads = %d)", ljDefaultThreadCount());
    printf("\n\n");

    Forces refForces(n);
    const double refMs = timeMs([&] { lj.calculateForcesReference(geometry, refForces); }, reps);
    const double refEnergy = lj.calculateEnergyReference(geometry);

    printf("%-18s %10s %9s %16s %12s %12s\n",
           "backend", "ms/call", "vs ref", "energy", "max|df|", "max rel");
    printf("%-18s %10.3f %9s %16.9f %12s %12s\n",
           "reference", refMs, "1.00x", refEnergy, "-", "-");

    // Every backend, measured ONCE each. An earlier version of this harness
    // re-timed the parallel backends at the end to print a second table; the
    // repeat runs came out ~2x slower purely from thermal throttling. Measure
    // once, store, then do arithmetic on the stored numbers.
    const LJBackend backends[] = {LJBackend::SerialTriangular, LJBackend::Serial,
                                  LJBackend::Threads, LJBackend::OpenMP,
                                  LJBackend::OpenMPPrivate};
    const int nBackends = 5;
    double ms[5] = {0, 0, 0, 0, 0};

    for (int k = 0; k < nBackends; ++k) {
        lj.setBackend(backends[k]);
        Forces f(n);
        ms[k] = timeMs([&] { lj.calculateForces(geometry, f); }, reps);
        const double e = lj.calculateEnergy(geometry);
        const Deviation d = compare(refForces, f);

        printf("%-18s %10.3f %8.2fx %16.9f %12.3e %12.3e%s\n",
               backendName(backends[k]), ms[k], refMs / ms[k], e, d.maxAbs, d.maxRel,
               d.bitwiseIdentical ? "  (bitwise ==)" : "");
    }

    // ---- The honest comparison ------------------------------------------
    // The reference is slow for reasons that have nothing to do with
    // parallelism (a std::map<std::string> lookup per pair). Dividing by it
    // credits threading for work that a serial rewrite already did.
    //
    // The denominator that means something is the fastest serial kernel.
    const double bestSerial = std::min(ms[0], ms[1]);
    const char* bestName = (ms[0] <= ms[1]) ? backendName(backends[0]) : backendName(backends[1]);

    printf("\n");
    printf("Best serial kernel: %s at %.3f ms.\n", bestName, bestSerial);
    printf("Speedup measured against THAT (the number you may honestly quote):\n");
    for (int k = 2; k < nBackends; ++k) {
        printf("    %-18s %.2fx   (vs reference it would look like %.2fx)\n",
               backendName(backends[k]), bestSerial / ms[k], refMs / ms[k]);
    }
    printf("\n");
    printf("Note: std::thread and openmp-gather use the GATHER form, which does\n");
    printf("~2x the arithmetic of the triangular kernel -- threading has to win\n");
    printf("that back before showing any profit. openmp-private keeps the\n");
    printf("triangular loop and gives each thread a private force buffer, so it\n");
    printf("avoids that penalty and is the fastest CPU kernel here. It does not\n");
    printf("port to a GPU (see lesson 5). Details in docs/learn-parallel/07.\n");

    printf("\n");
    return 0;
}
