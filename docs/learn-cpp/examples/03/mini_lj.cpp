// mini_lj, chapter 3: two atoms with real positions in a periodic box.
// Functions receive their inputs by const reference and write results through references.
#include <array>
#include <cmath>
#include <iostream>

using Vec3 = std::array<double, 3>;   // a new name for "three doubles"

// d is changed in place: the caller sees the result.
void applyMinimumImage(Vec3& d, double box) {
    for (int k = 0; k < 3; ++k) {
        d[k] -= box * std::round(d[k] / box);
    }
}

// a and b are read, never copied, never changed. The result is returned by value.
Vec3 displacement(const Vec3& a, const Vec3& b) {
    return {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
}

// Two outputs, so two reference parameters instead of a return value.
void pairForce(double epsilon, double sigma, const Vec3& d, Vec3& f1, Vec3& f2) {
    const double r2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    const double sr2 = sigma * sigma / r2;
    const double sr6 = sr2 * sr2 * sr2;
    const double sr12 = sr6 * sr6;
    const double fOverR = 24.0 * epsilon * (2.0 * sr12 - sr6) / r2;
    for (int k = 0; k < 3; ++k) {
        f1[k] -= fOverR * d[k];
        f2[k] += fOverR * d[k];
    }
}

int main() {
    const double box = 5.0;
    Vec3 a = {0.2, 0.0, 0.0};
    Vec3 b = {4.0, 0.0, 0.0};

    Vec3 d = displacement(a, b);
    std::cout << "raw dx      = " << d[0] << std::endl;
    applyMinimumImage(d, box);
    std::cout << "imaged dx   = " << d[0] << std::endl;

    Vec3 fa = {0.0, 0.0, 0.0};
    Vec3 fb = {0.0, 0.0, 0.0};
    pairForce(1.0, 1.0, d, fa, fb);
    std::cout << "force on a  = " << fa[0] << std::endl;
    std::cout << "force on b  = " << fb[0] << std::endl;
    return 0;
}
