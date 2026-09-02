// Reproduce lesson 6's scheduling table on YOUR machine.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

#include "exercise_support.h"
#include "omp_kernel.h"

int main(int argc, char** argv) {
    const std::size_t N = (argc > 1) ? std::stoul(argv[1]) : 4096;
    const ex::Box b = ex::makeBox(N);

    auto timeIt = [&](int T, ex06::Schedule s) {
        std::vector<double> fx, fy, fz;
        ex06::ompGather(b.x, b.y, b.z, 4.0, 1.0, b.box[0], b.box[1], b.box[2],
                        b.cutoff, fx, fy, fz, T, s);          // warm-up
        if (fx.size() != b.n) return -1.0;
        double best = 1e300;
        for (int r = 0; r < 5; ++r) {
            const auto t0 = std::chrono::high_resolution_clock::now();
            ex06::ompGather(b.x, b.y, b.z, 4.0, 1.0, b.box[0], b.box[1], b.box[2],
                            b.cutoff, fx, fy, fz, T, s);
            const auto t1 = std::chrono::high_resolution_clock::now();
            best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        return best;
    };

    printf("\nN = %zu   OpenMP: %s   max threads: %d\n\n", b.n,
           ex06::openMPEnabled() ? "yes" : "NO (results will be serial)",
           ex06::maxThreads());
    printf("%8s %12s %12s %12s %12s\n", "threads", "static", "static,16",
           "dynamic,32", "guided");

    const int maxT = std::max(8, ex06::maxThreads());
    for (int T = 1; T <= maxT; ++T) {
        printf("%8d", T);
        for (ex06::Schedule s : {ex06::Schedule::Static, ex06::Schedule::StaticChunk,
                                 ex06::Schedule::Dynamic, ex06::Schedule::Guided}) {
            const double ms = timeIt(T, s);
            if (ms < 0) { printf("%12s", "n/i"); } else { printf("%12.2f", ms); }
        }
        printf("\n");
    }
    printf("\nLook for a thread count where `static` gets WORSE than the row\n"
           "above it. If you find one, compare it to your performance-core\n"
           "count, then write your answers into src/omp_kernel.cpp.\n\n");
    return 0;
}
