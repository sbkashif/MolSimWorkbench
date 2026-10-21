// Two functions may share a name if their parameter lists differ.
#include <array>
#include <cmath>
#include <iostream>

using Vec3 = std::array<double, 3>;

double pairEnergy(double r) {                      // takes a distance
    const double sr6 = std::pow(1.0 / r, 6);
    return 4.0 * (sr6 * sr6 - sr6);
}

double pairEnergy(const Vec3& d) {                 // takes a separation vector
    return pairEnergy(std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
}

int main() {
    const Vec3 d = {1.2, 0.0, 0.0};
    std::cout << "pairEnergy(1.2) = " << pairEnergy(1.2) << std::endl;
    std::cout << "pairEnergy(d)   = " << pairEnergy(d) << std::endl;
    return 0;
}
