// Produce the measurements for project/REPORT.md.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

#include "exercise_support.h"
#include "student_kernel.h"

namespace {
struct View {
    std::vector<double> fx, fy, fz, eps4{4.0}, sigma2{1.0};
    LJKernelView v;
};
View makeView(const ex::Box& b) {
    View w;
    w.fx.assign(b.n, 0.0); w.fy.assign(b.n, 0.0); w.fz.assign(b.n, 0.0);
    w.v.x = b.x.data(); w.v.y = b.y.data(); w.v.z = b.z.data();
    w.v.type = b.type.data();
    w.v.fx = w.fx.data(); w.v.fy = w.fy.data(); w.v.fz = w.fz.data();
    w.v.n = b.n; w.v.eps4 = w.eps4.data(); w.v.sigma2 = w.sigma2.data();
    w.v.ntypes = 1;
    w.v.bx = b.box[0]; w.v.by = b.box[1]; w.v.bz = b.box[2];
    w.v.cutoff2 = b.cutoff * b.cutoff;
    return w;
}
}  // namespace

int main(int argc, char** argv) {
    const std::size_t N = (argc > 1) ? std::stoul(argv[1]) : 4096;
    const ex::Box b = ex::makeBox(N);
    const auto ref = ex::referenceForces(b);

    auto run = [&](StudentBackend backend, int T) {
        View w = makeView(b);
        studentCompute(w.v, backend, T);
        double best = 1e300;
        for (int r = 0; r < 5; ++r) {
            const auto t0 = std::chrono::high_resolution_clock::now();
            studentCompute(w.v, backend, T);
            const auto t1 = std::chrono::high_resolution_clock::now();
            best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        return std::make_pair(best, ex::maxRelError(ref, w.fx, w.fy, w.fz));
    };

    printf("\nCAPSTONE REPORT   N = %zu   rho = 0.8   cutoff = 2.5\n\n", b.n);
    printf("%-24s %10s %12s %14s\n", "backend", "ms/call", "threads", "max rel err");

    const auto tri = run(StudentBackend::Triangular, 1);
    printf("%-24s %10.3f %12d %14.3e\n", "Triangular", tri.first, 1, tri.second);
    const auto gat = run(StudentBackend::Gather, 1);
    printf("%-24s %10.3f %12d %14.3e\n", "Gather", gat.first, 1, gat.second);
    const auto par = run(StudentBackend::Parallel, 8);
    printf("%-24s %10.3f %12d %14.3e\n", "Parallel", par.first, 8, par.second);
    const auto det = run(StudentBackend::ParallelDeterministic, 8);
    printf("%-24s %10.3f %12d %14.3e\n", "ParallelDeterministic", det.first, 8, det.second);

    if (tri.first > 0 && par.first > 0) {
        printf("\nHonest speedup (Parallel@8 vs your Triangular@1): %.2fx  [bar: 2.00x]\n",
               tri.first / par.first);
    }

    printf("\nStrong scaling of Parallel:\n");
    printf("%8s %10s %10s %12s\n", "threads", "ms", "speedup", "efficiency");
    const double t1 = run(StudentBackend::Parallel, 1).first;
    for (int T = 1; T <= 8; ++T) {
        const double ms = run(StudentBackend::Parallel, T).first;
        if (ms <= 0 || ms > 1e299) { printf("%8d %10s\n", T, "n/i"); continue; }
        printf("%8d %10.3f %9.2fx %11.0f%%\n", T, ms, t1 / ms, 100.0 * (t1 / ms) / T);
    }
    printf("\nCopy these into project/REPORT.md and explain the shape of the curve.\n\n");
    return 0;
}
