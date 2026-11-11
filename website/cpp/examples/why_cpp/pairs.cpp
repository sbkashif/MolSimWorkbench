// The same pass over every pair of 512 LJ atoms, in C++.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    const int N = 512;
    const double L = 8.6177, rc2 = 2.5 * 2.5;  // density 0.8, cutoff 2.5
    const double a = L / 8;                    // an 8 x 8 x 8 grid
    std::vector<double> x, y, z;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            for (int k = 0; k < 8; ++k) {
                x.push_back((i + 0.5) * a);
                y.push_back((j + 0.5) * a);
                z.push_back((k + 0.5) * a);
            }

    double best = 1e9, e = 0.0;
    for (int rep = 0; rep < 20; ++rep) {
        auto t0 = std::chrono::steady_clock::now();
        e = 0.0;
        for (int i = 0; i < N; ++i)
            for (int j = i + 1; j < N; ++j) {
                double dx = x[j] - x[i], dy = y[j] - y[i], dz = z[j] - z[i];
                dx -= L * std::round(dx / L);
                dy -= L * std::round(dy / L);
                dz -= L * std::round(dz / L);
                double r2 = dx * dx + dy * dy + dz * dz;
                if (r2 > rc2) continue;
                double sr6 = 1.0 / (r2 * r2 * r2);
                e += 4.0 * (sr6 * sr6 - sr6);
            }
        double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        if (t < best) best = t;
    }
    std::printf("c++:    U = %.6f   %.2f ms per pass\n", e, best * 1e3);
}
