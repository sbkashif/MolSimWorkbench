// A deliberately WRONG parallel force loop: the triangular (i < j) loop split
// across threads, every thread writing into the same force arrays. Runs it a
// few times and compares with the serial result.
//
//   clang++ -std=c++17 -O3 -I <repo>/src naive_scatter.cpp \
//       <repo>/build/libBuilderLib.a -o naive_scatter
#include "builder/lj_particle_box_builder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

struct System {
    std::vector<double> x, y, z;
    double L = 0.0;
};

// The pair loop for rows i0..i1 of the triangle, adding into fx, fy, fz.
void scatter_rows(const System& s, std::size_t i0, std::size_t i1,
                  double* fx, double* fy, double* fz) {
    const std::size_t n = s.x.size();
    const double rc2 = 2.5 * 2.5;
    for (std::size_t i = i0; i < i1; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = s.x[j] - s.x[i], dy = s.y[j] - s.y[i], dz = s.z[j] - s.z[i];
            dx -= s.L * std::round(dx / s.L);
            dy -= s.L * std::round(dy / s.L);
            dz -= s.L * std::round(dz / s.L);
            const double r2 = dx * dx + dy * dy + dz * dz;
            if (r2 > rc2) continue;
            const double sr6 = 1.0 / (r2 * r2 * r2);
            const double f = 24.0 * (2.0 * sr6 * sr6 - sr6) / r2;
            fx[i] -= f * dx; fy[i] -= f * dy; fz[i] -= f * dz;
            fx[j] += f * dx; fy[j] += f * dy; fz[j] += f * dz;   // j may belong to another thread
        }
    }
}

int main() {
    LJBoxConfig cfg;
    cfg.numParticles = 4096;
    cfg.density = 0.8;
    cfg.randomSeed = 42;
    std::array<double, 3> box{};
    Geometry g = LJParticleBoxBuilder::createCubicLattice(cfg, box);
    System s;
    s.L = box[0];
    for (std::size_t i = 0; i < g.getNumberOfAtoms(); ++i) {
        auto p = g.getCoordinates(i);
        s.x.push_back(p[0]); s.y.push_back(p[1]); s.z.push_back(p[2]);
    }
    const std::size_t n = s.x.size();

    std::vector<double> rx(n, 0.0), ry(n, 0.0), rz(n, 0.0);
    scatter_rows(s, 0, n, rx.data(), ry.data(), rz.data());

    const int T = 8;
    std::printf("N = %zu, %d threads\n", n, T);
    std::printf("%4s %14s %16s\n", "run", "max|F - Fserial|", "|sum of forces|");
    for (int run = 1; run <= 5; ++run) {
        std::vector<double> fx(n, 0.0), fy(n, 0.0), fz(n, 0.0);
        std::vector<std::thread> pool;
        for (int t = 0; t < T; ++t) {
            const std::size_t i0 = n * t / T, i1 = n * (t + 1) / T;
            pool.emplace_back(scatter_rows, std::cref(s), i0, i1, fx.data(), fy.data(), fz.data());
        }
        for (auto& th : pool) th.join();

        double worst = 0.0, sx = 0.0, sy = 0.0, sz = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            worst = std::max({worst, std::fabs(fx[i] - rx[i]), std::fabs(fy[i] - ry[i]),
                              std::fabs(fz[i] - rz[i])});
            sx += fx[i]; sy += fy[i]; sz += fz[i];
        }
        std::printf("%4d %14.3e %16.3e\n", run, worst, std::sqrt(sx * sx + sy * sy + sz * sz));
    }
    double sx = 0, sy = 0, sz = 0;
    for (std::size_t i = 0; i < n; ++i) { sx += rx[i]; sy += ry[i]; sz += rz[i]; }
    double big = 0.0;
    for (std::size_t i = 0; i < n; ++i)
        big = std::max({big, std::fabs(rx[i]), std::fabs(ry[i]), std::fabs(rz[i])});
    std::printf("serial: |sum of forces| = %.3e, largest force component = %.3e\n",
                std::sqrt(sx * sx + sy * sy + sz * sz), big);
    return 0;
}
