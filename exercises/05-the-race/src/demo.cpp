// Run the race and look at it. Not a test -- a microscope.
#include <chrono>
#include <cstdio>
#include <vector>

#include "exercise_support.h"
#include "race.h"

int main() {
    const ex::Box b = ex::makeBox(4096);
    const auto ref = ex::referenceForces(b);
    const double refE = ex::referenceEnergy(b);

    auto show = [&](const char* label, bool racy, int T) {
        std::vector<double> fx, fy, fz;
        auto t0 = std::chrono::high_resolution_clock::now();
        const double e = racy
            ? ex05::racyScatter(b.x, b.y, b.z, 4.0, 1.0, b.box[0], b.box[1],
                                b.box[2], b.cutoff, fx, fy, fz, T)
            : ex05::privateBufferScatter(b.x, b.y, b.z, 4.0, 1.0, b.box[0],
                                         b.box[1], b.box[2], b.cutoff, fx, fy, fz, T);
        auto t1 = std::chrono::high_resolution_clock::now();
        if (fx.size() != b.n) { printf("%-28s  not implemented yet\n", label); return; }

        const std::size_t wrong = ex05::countWrongAtoms(ref, fx, fy, fz, 1e-9);
        double netx = 0.0;
        for (double v : fx) netx += v;
        printf("%-28s %8.2f ms  %6zu/%zu wrong (%.1f%%)  maxrel %9.2e  "
               "E %+.6f (ref %+.6f)  netFx %+.2e\n",
               label, std::chrono::duration<double, std::milli>(t1 - t0).count(),
               wrong, b.n, 100.0 * wrong / b.n,
               ex::maxRelError(ref, fx, fy, fz), e, refE, netx);
    };

    printf("\nN = %zu, reference energy = %.6f\n\n", b.n, refE);
    show("private buffers, 1 thread", false, 1);
    show("private buffers, 8 threads", false, 8);
    show("RACY, 1 thread", true, 1);
    for (int r = 0; r < 5; ++r) show("RACY, 8 threads", true, 8);
    printf("\nNote how the racy runs differ from each other, how few atoms are\n"
           "actually wrong, and how the total energy and net force still look\n"
           "entirely reasonable. That is why aggregate checks miss races.\n\n");
    return 0;
}
