#include <cmath>
#include <iostream>

int main() {
    double r = 1.5;
    r = "far";
    double sr6 = std::pow(1.0 / r, 6);
    double energy = 4.0 * (sr6 * sr6 - sr6);
    std::cout << "U(" << r << ") = " << energy << "\n";
    return 0;
}
