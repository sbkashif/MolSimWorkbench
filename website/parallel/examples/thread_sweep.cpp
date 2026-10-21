// Times the engine's force backends at several system sizes and thread
// counts, and checks every result against the serial triangular kernel.
//
//   OMP=$(brew --prefix libomp)        # macOS; on Linux drop the -I/-L parts
//   clang++ -std=c++17 -O3 -Xclang -fopenmp -I $OMP/include -I <repo>/src \
//       thread_sweep.cpp <repo>/build/libEngineLib.a <repo>/build/libBuilderLib.a \
//       -L $OMP/lib -lomp -o thread_sweep
#include "builder/lj_particle_box_builder.h"
#include "engine/potentials/lj_potential.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using Clock = std::chrono::steady_clock;
using Forces = std::vector<std::array<double, 3>>;

template <typename F>
double best_ms(F&& f, int reps) {
    f();
    double best = 1e300;
    for (int r = 0; r < reps; ++r) {
        auto t0 = Clock::now();
        f();
        auto t1 = Clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return best;
}

double max_diff(const Forces& a, const Forces& b) {
    double d = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
        for (int k = 0; k < 3; ++k) d = std::max(d, std::fabs(a[i][k] - b[i][k]));
    return d;
}

int main() {
    std::printf("%6s %-18s %7s %10s %12s\n", "N", "backend", "threads", "ms", "max|df|");
    for (int side : {5, 8, 10, 13, 16}) {
        LJBoxConfig cfg;
        cfg.numParticles = static_cast<std::size_t>(side) * side * side;
        cfg.density = 0.8;
        cfg.randomSeed = 42;
        std::array<double, 3> box{};
        Geometry g = LJParticleBoxBuilder::createCubicLattice(cfg, box);
        const std::size_t n = g.getNumberOfAtoms();
        const int reps = n < 2000 ? 20 : 7;

        LJPotential lj;
        lj.addAtomParameters("LJ", 1.0, 1.0);
        lj.setCutoff(2.5);

        Forces ref(n);
        lj.setBackend(LJBackend::SerialTriangular);
        const double tri = best_ms([&] { lj.calculateForces(g, ref); }, reps);
        std::printf("%6zu %-18s %7d %10.4f %12s\n", n, "serial-triangular", 1, tri, "-");

        Forces f(n);
        lj.setBackend(LJBackend::Serial);
        const double gat = best_ms([&] { lj.calculateForces(g, f); }, reps);
        std::printf("%6zu %-18s %7d %10.4f %12.3e\n", n, "serial-gather", 1, gat, max_diff(ref, f));

        const struct { LJBackend backend; const char* name; } parallel[] = {
            {LJBackend::Threads, "std::thread-gather"},
            {LJBackend::OpenMP, "openmp-gather"},
            {LJBackend::OpenMPPrivate, "openmp-private"},
        };
        for (const auto& p : parallel) {
            lj.setBackend(p.backend);
            for (int t : {1, 2, 4, 6, 8, 10}) {
                lj.setNumThreads(t);
                const double ms = best_ms([&] { lj.calculateForces(g, f); }, reps);
                std::printf("%6zu %-18s %7d %10.4f %12.3e\n", n, p.name, t, ms, max_diff(ref, f));
            }
        }
    }
    return 0;
}
