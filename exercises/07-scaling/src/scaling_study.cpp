// Strong-scaling study of the repo's real kernels.
#include <cstdio>
#include <vector>

#include "engine/potentials/lj_potential.h"
#include "exercise_support.h"
#include "scaling.h"

int main(int argc, char** argv) {
    const std::size_t N = (argc > 1) ? std::stoul(argv[1]) : 4096;
    const ex::Box b = ex::makeBox(N);

    LJPotential lj;
    lj.addAtomParameters("LJ", b.epsilon, b.sigma);
    lj.setCutoff(b.cutoff);
    std::vector<std::array<double, 3>> f;

    auto timeIt = [&](LJBackend backend, int T) {
        lj.setBackend(backend);
        lj.setNumThreads(T);
        lj.calculateForces(b.geometry, f);
        double best = 1e300;
        for (int r = 0; r < 5; ++r) {
            const auto t0 = std::chrono::high_resolution_clock::now();
            lj.calculateForces(b.geometry, f);
            const auto t1 = std::chrono::high_resolution_clock::now();
            best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        return best;
    };

    printf("\nN = %zu\n\n", b.n);

    std::vector<ex07::Timing> serials{
        {"reference", 0.0, 1}, {"serial-triangular", 0.0, 1}, {"serial-gather", 0.0, 1}};
    lj.setBackend(LJBackend::Serial);
    lj.calculateForcesReference(b.geometry, f);
    {
        double best = 1e300;
        for (int r = 0; r < 3; ++r) {
            const auto t0 = std::chrono::high_resolution_clock::now();
            lj.calculateForcesReference(b.geometry, f);
            const auto t1 = std::chrono::high_resolution_clock::now();
            best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        serials[0].ms = best;
    }
    serials[1].ms = timeIt(LJBackend::SerialTriangular, 1);
    serials[2].ms = timeIt(LJBackend::Serial, 1);

    for (const auto& s : serials) printf("  %-20s %8.2f ms\n", s.label.c_str(), s.ms);

    const auto base = ex07::chooseBaseline(serials);
    if (base.label.empty()) {
        printf("\n  chooseBaseline() is not implemented yet -- finish src/scaling.cpp.\n\n");
        return 0;
    }
    printf("\n  honest baseline: %s at %.2f ms\n\n", base.label.c_str(), base.ms);

    const double t1 = timeIt(LJBackend::OpenMP, 1);
    printf("%8s %10s %12s %12s %14s %14s\n", "threads", "ms", "vs baseline",
           "vs 1 thread", "efficiency", "implied s");
    for (int T = 1; T <= 8; ++T) {
        const double ms = timeIt(LJBackend::OpenMP, T);
        const double spVs1 = t1 / ms;
        printf("%8d %10.2f %11.2fx %11.2fx %13.0f%% %14.4f\n", T, ms,
               base.ms / ms, spVs1, 100.0 * ex07::efficiency(t1, ms, T),
               ex07::amdahlSerialFraction(spVs1, T));
    }
    printf("\n  Look for the thread count where efficiency falls off a cliff.\n"
           "  Compare it to your performance-core count.\n\n");
    return 0;
}
