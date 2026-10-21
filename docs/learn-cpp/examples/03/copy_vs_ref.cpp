// Chapter 3 toy: assignment copies; a reference is a second name for the same variable.
#include <array>
#include <iostream>

int main() {
    // 1. Plain assignment copies.
    double a = 1.0;
    double b = a;        // b is a new variable holding a copy of a's value
    b = 2.0;
    std::cout << "copy:      a = " << a << ", b = " << b << std::endl;

    // 2. A reference is a second name for the same storage.
    double& c = a;       // c IS a
    c = 3.0;
    std::cout << "reference: a = " << a << ", c = " << c << std::endl;

    // 3. Same for a whole array of three numbers: assignment copies all three.
    std::array<double, 3> p = {1.0, 2.0, 3.0};
    std::array<double, 3> q = p;
    q[0] = 99.0;
    std::cout << "array copy: p[0] = " << p[0] << ", q[0] = " << q[0] << std::endl;

    std::array<double, 3>& r = p;
    r[0] = 99.0;
    std::cout << "array ref:  p[0] = " << p[0] << std::endl;

    // 4. Assigning to a reference never re-points it: it writes through.
    double other = 7.0;
    c = other;           // a becomes 7; c still names a
    other = 8.0;
    std::cout << "c = other:  a = " << a << ", other = " << other << std::endl;
    return 0;
}
