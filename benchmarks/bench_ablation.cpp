// ---------------------------------------------------------------------------
// bench_ablation — reproduces the two measurements lesson 2 depends on.
//
//   PART 1  Ablation: strip one thing at a time from the original kernel and
//           attribute each delta. Serial and triangular throughout.
//
//   PART 2  Type-count sweep: how the std::map<std::string> lookup and the flat
//           table behave as the number of atom types grows. The repo's example
//           has ONE type, which is the map's best case; a real mixture is not.
//
// Every variant computes the same physics, so the printed forces must agree.
//
//   ./build/benchmarks/bench_ablation [N]
// ---------------------------------------------------------------------------
#include "builder/lj_particle_box_builder.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
using AoS = std::vector<std::array<double, 3>>;

namespace {

struct Params { double epsilon, sigma; };

template <typename F>
double timeMs(F&& f, int reps = 5) {
    f();
    double best = 1e300;
    for (int r = 0; r < reps; ++r) {
        const auto t0 = Clock::now();
        f();
        const auto t1 = Clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return best;
}

// ---- the shared inner physics, so every variant computes the same thing ----
#define MIC_SLOW \
    if (b[0] > 0) dx -= b[0] * std::round(dx / b[0]); \
    if (b[1] > 0) dy -= b[1] * std::round(dy / b[1]); \
    if (b[2] > 0) dz -= b[2] * std::round(dz / b[2]);
#define MIC_FAST \
    dx -= b[0] * std::round(dx * ib0); \
    dy -= b[1] * std::round(dy * ib1); \
    dz -= b[2] * std::round(dz * ib2);

// V0: the original -- text keys, std::map, std::pow, sqrt, AoS.
double v0(const std::vector<std::string>& t, const AoS& p, const std::array<double,3>& b,
          double rc, std::map<std::string, Params>& tbl, AoS& f) {
    for (auto& x : f) x = {0, 0, 0};
    double e = 0.0;
    for (std::size_t i = 0; i < t.size(); ++i)
        for (std::size_t j = i + 1; j < t.size(); ++j) {
            double dx = p[j][0]-p[i][0], dy = p[j][1]-p[i][1], dz = p[j][2]-p[i][2];
            MIC_SLOW
            const double d2 = dx*dx + dy*dy + dz*dz, d = std::sqrt(d2);
            if (d > rc || d < 1e-10) continue;
            const auto ia = tbl.find(t[i]), ib = tbl.find(t[j]);
            if (ia == tbl.end() || ib == tbl.end()) continue;
            const double eps = std::sqrt(ia->second.epsilon * ib->second.epsilon);
            const double sig = std::sqrt(ia->second.sigma  * ib->second.sigma);
            const double sr = sig/d, sr6 = std::pow(sr, 6), sr12 = sr6*sr6;
            e += 4.0*eps*(sr12 - sr6);
            const double fm = 24.0*eps*(2*sr12 - sr6)/d;
            const double ux = dx/d, uy = dy/d, uz = dz/d;
            f[i][0]-=fm*ux; f[i][1]-=fm*uy; f[i][2]-=fm*uz;
            f[j][0]+=fm*ux; f[j][1]+=fm*uy; f[j][2]+=fm*uz;
        }
    return e;
}

// V1: integer type ids + flat pre-mixed table. Everything else identical to V0.
double v1(const std::vector<int>& t, const AoS& p, const std::array<double,3>& b,
          double rc, const std::vector<double>& E, const std::vector<double>& S,
          int nt, AoS& f) {
    for (auto& x : f) x = {0, 0, 0};
    double e = 0.0;
    for (std::size_t i = 0; i < t.size(); ++i)
        for (std::size_t j = i + 1; j < t.size(); ++j) {
            double dx = p[j][0]-p[i][0], dy = p[j][1]-p[i][1], dz = p[j][2]-p[i][2];
            MIC_SLOW
            const double d2 = dx*dx + dy*dy + dz*dz, d = std::sqrt(d2);
            if (d > rc || d < 1e-10) continue;
            const double eps = E[t[i]*nt + t[j]], sig = S[t[i]*nt + t[j]];
            const double sr = sig/d, sr6 = std::pow(sr, 6), sr12 = sr6*sr6;
            e += 4.0*eps*(sr12 - sr6);
            const double fm = 24.0*eps*(2*sr12 - sr6)/d;
            const double ux = dx/d, uy = dy/d, uz = dz/d;
            f[i][0]-=fm*ux; f[i][1]-=fm*uy; f[i][2]-=fm*uz;
            f[j][0]+=fm*ux; f[j][1]+=fm*uy; f[j][2]+=fm*uz;
        }
    return e;
}

// V1b: + std::pow replaced by a multiply chain.
double v1b(const std::vector<int>& t, const AoS& p, const std::array<double,3>& b,
           double rc, const std::vector<double>& E, const std::vector<double>& S,
           int nt, AoS& f) {
    for (auto& x : f) x = {0, 0, 0};
    double e = 0.0;
    for (std::size_t i = 0; i < t.size(); ++i)
        for (std::size_t j = i + 1; j < t.size(); ++j) {
            double dx = p[j][0]-p[i][0], dy = p[j][1]-p[i][1], dz = p[j][2]-p[i][2];
            MIC_SLOW
            const double d2 = dx*dx + dy*dy + dz*dz, d = std::sqrt(d2);
            if (d > rc || d < 1e-10) continue;
            const double eps = E[t[i]*nt + t[j]], sig = S[t[i]*nt + t[j]];
            const double sr = sig/d, sr3 = sr*sr*sr, sr6 = sr3*sr3, sr12 = sr6*sr6;
            e += 4.0*eps*(sr12 - sr6);
            const double fm = 24.0*eps*(2*sr12 - sr6)/d;
            const double ux = dx/d, uy = dy/d, uz = dz/d;
            f[i][0]-=fm*ux; f[i][1]-=fm*uy; f[i][2]-=fm*uz;
            f[j][0]+=fm*ux; f[j][1]+=fm*uy; f[j][2]+=fm*uz;
        }
    return e;
}

// V2: + r^2 arithmetic. No sqrt, no normalisation, reciprocal box lengths.
double v2(const std::vector<int>& t, const AoS& p, const std::array<double,3>& b,
          double rc, const std::vector<double>& E4, const std::vector<double>& S2,
          int nt, AoS& f) {
    for (auto& x : f) x = {0, 0, 0};
    const double rc2 = rc*rc;
    const double ib0 = b[0]>0?1/b[0]:0, ib1 = b[1]>0?1/b[1]:0, ib2 = b[2]>0?1/b[2]:0;
    double e = 0.0;
    for (std::size_t i = 0; i < t.size(); ++i) {
        const double* e4r = &E4[t[i]*nt];
        const double* s2r = &S2[t[i]*nt];
        for (std::size_t j = i + 1; j < t.size(); ++j) {
            double dx = p[j][0]-p[i][0], dy = p[j][1]-p[i][1], dz = p[j][2]-p[i][2];
            MIC_FAST
            const double r2 = dx*dx + dy*dy + dz*dz;
            if (r2 > rc2 || r2 < 1e-20) continue;
            const double e4 = e4r[t[j]], s2 = s2r[t[j]];
            const double sr2 = s2/r2, sr6 = sr2*sr2*sr2, sr12 = sr6*sr6;
            e += e4*(sr12 - sr6);
            const double fo = 6.0*e4*(2*sr12 - sr6)/r2;
            f[i][0]-=fo*dx; f[i][1]-=fo*dy; f[i][2]-=fo*dz;
            f[j][0]+=fo*dx; f[j][1]+=fo*dy; f[j][2]+=fo*dz;
        }
    }
    return e;
}

// V3: + structure of arrays.
double v3(const std::vector<int>& t, const std::vector<double>& X,
          const std::vector<double>& Y, const std::vector<double>& Z,
          const std::array<double,3>& b, double rc, const std::vector<double>& E4,
          const std::vector<double>& S2, int nt,
          std::vector<double>& FX, std::vector<double>& FY, std::vector<double>& FZ) {
    const std::size_t n = t.size();
    std::fill(FX.begin(), FX.end(), 0.0);
    std::fill(FY.begin(), FY.end(), 0.0);
    std::fill(FZ.begin(), FZ.end(), 0.0);
    const double rc2 = rc*rc;
    const double ib0 = b[0]>0?1/b[0]:0, ib1 = b[1]>0?1/b[1]:0, ib2 = b[2]>0?1/b[2]:0;
    double e = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double xi = X[i], yi = Y[i], zi = Z[i];
        const double* e4r = &E4[t[i]*nt];
        const double* s2r = &S2[t[i]*nt];
        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = X[j]-xi, dy = Y[j]-yi, dz = Z[j]-zi;
            MIC_FAST
            const double r2 = dx*dx + dy*dy + dz*dz;
            if (r2 > rc2 || r2 < 1e-20) continue;
            const double e4 = e4r[t[j]], s2 = s2r[t[j]];
            const double sr2 = s2/r2, sr6 = sr2*sr2*sr2, sr12 = sr6*sr6;
            e += e4*(sr12 - sr6);
            const double fo = 6.0*e4*(2*sr12 - sr6)/r2;
            FX[i]-=fo*dx; FY[i]-=fo*dy; FZ[i]-=fo*dz;
            FX[j]+=fo*dx; FY[j]+=fo*dy; FZ[j]+=fo*dz;
        }
    }
    return e;
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t N = (argc > 1) ? std::stoul(argv[1]) : 4096;

    LJBoxConfig cfg;
    cfg.numParticles = N;
    cfg.density = 0.8;
    cfg.randomSeed = 42;
    std::array<double, 3> box{};
    Geometry g = LJParticleBoxBuilder::createCubicLattice(cfg, box);
    const std::size_t n = g.getNumberOfAtoms();
    const auto& pos = g.getPositions();

    AoS p(n), f(n);
    std::vector<double> X(n), Y(n), Z(n), FX(n), FY(n), FZ(n);
    for (std::size_t i = 0; i < n; ++i) {
        p[i] = pos[i];
        X[i] = pos[i][0]; Y[i] = pos[i][1]; Z[i] = pos[i][2];
    }

    // Count how many pairs actually survive the cutoff.
    long inside = 0;
    const long total = static_cast<long>(n) * (n - 1) / 2;
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = pos[j][0]-pos[i][0], dy = pos[j][1]-pos[i][1], dz = pos[j][2]-pos[i][2];
            dx -= box[0]*std::round(dx/box[0]);
            dy -= box[1]*std::round(dy/box[1]);
            dz -= box[2]*std::round(dz/box[2]);
            if (dx*dx + dy*dy + dz*dz <= 6.25) ++inside;
        }

    printf("\nN = %zu   rho = 0.8   cutoff = 2.5\n", n);
    printf("pairs evaluated = %ld   inside cutoff = %ld (%.2f%%)   ratio %.0f : 1\n\n",
           total, inside, 100.0 * inside / total, double(total) / inside);

    // =====================================================================
    // PART 1 -- ablation, one atom type
    // =====================================================================
    {
        std::map<std::string, Params> tbl{{"LJ", {1.0, 1.0}}};
        std::vector<std::string> ts(n, "LJ");
        std::vector<int> ti(n, 0);
        std::vector<double> E{1.0}, S{1.0}, E4{4.0}, S2{1.0};

        const double t0 = timeMs([&]{ v0(ts, p, box, 2.5, tbl, f); });
        const double t1 = timeMs([&]{ v1(ti, p, box, 2.5, E, S, 1, f); });
        const double t1b = timeMs([&]{ v1b(ti, p, box, 2.5, E, S, 1, f); });
        const double t2 = timeMs([&]{ v2(ti, p, box, 2.5, E4, S2, 1, f); });
        const double t3 = timeMs([&]{ v3(ti, X, Y, Z, box, 2.5, E4, S2, 1, FX, FY, FZ); });

        printf("PART 1  Ablation (1 atom type, serial, triangular)\n\n");
        printf("  %-44s %8s %9s %9s\n", "variant", "ms", "cumul.", "step");
        printf("  %-44s %8.2f %8.2fx %9s\n", "V0  original (map+text, pow, sqrt, AoS)", t0, 1.0, "-");
        printf("  %-44s %8.2f %8.2fx %8.2fx\n", "V1   + integer type ids, flat table", t1, t0/t1, t0/t1);
        printf("  %-44s %8.2f %8.2fx %8.2fx\n", "V1b  + std::pow -> multiply chain", t1b, t0/t1b, t1/t1b);
        printf("  %-44s %8.2f %8.2fx %8.2fx\n", "V2   + drop sqrt, work in r^2", t2, t0/t2, t1b/t2);
        printf("  %-44s %8.2f %8.2fx %8.2fx\n", "V3   + structure-of-arrays", t3, t0/t3, t2/t3);
        printf("\n  Note V1: the map lookup sits AFTER the cutoff test, so it runs\n");
        printf("  %ld times, not %ld. Saving %.2f ms over %ld calls = %.1f ns each.\n\n",
               inside, total, t0 - t1, inside, (t0 - t1) * 1e6 / inside);
    }

    // =====================================================================
    // PART 2 -- how the lookup behaves as the mixture gets richer
    //
    // sigma is held at 1.0 for every type so the set of pairs inside the
    // cutoff is IDENTICAL at every type count. Only the lookup changes.
    // =====================================================================
    printf("PART 2  Cost of the lookup vs number of atom types\n\n");
    printf("  %8s %14s %14s %12s %14s\n",
           "types", "map+text (ms)", "flat table (ms)", "map cost", "ns per lookup");

    for (int T : {1, 2, 5, 10, 20, 50, 100}) {
        std::map<std::string, Params> tbl;
        std::vector<std::string> names;
        for (int t = 0; t < T; ++t) {
            // Force-field-ish names, all short enough to avoid heap allocation.
            const std::string nm = "AT" + std::to_string(t);
            names.push_back(nm);
            tbl[nm] = {1.0 + 0.1 * t, 1.0};   // sigma fixed => pair set unchanged
        }
        std::vector<std::string> ts(n);
        std::vector<int> ti(n);
        for (std::size_t i = 0; i < n; ++i) {
            ti[i] = static_cast<int>(i % T);
            ts[i] = names[ti[i]];
        }
        std::vector<double> E(std::size_t(T)*T), S(std::size_t(T)*T),
                            E4(std::size_t(T)*T), S2(std::size_t(T)*T);
        for (int a = 0; a < T; ++a)
            for (int b2 = 0; b2 < T; ++b2) {
                const double eps = std::sqrt(tbl[names[a]].epsilon * tbl[names[b2]].epsilon);
                const double sig = std::sqrt(tbl[names[a]].sigma  * tbl[names[b2]].sigma);
                E[a*T + b2] = eps;  S[a*T + b2] = sig;
                E4[a*T + b2] = 4.0*eps;  S2[a*T + b2] = sig*sig;
            }

        const double tmap = timeMs([&]{ v0(ts, p, box, 2.5, tbl, f); });
        const double tflat = timeMs([&]{ v1(ti, p, box, 2.5, E, S, T, f); });
        printf("  %8d %14.2f %14.2f %11.2fx %14.1f\n",
               T, tmap, tflat, tmap/tflat, (tmap - tflat) * 1e6 / inside);
    }
    printf("\n  The flat-table column should stay flat: one integer index, whatever\n");
    printf("  the type count. The map column should climb, because the tree gets\n");
    printf("  deeper and the text keys get longer.\n\n");
    return 0;
}
