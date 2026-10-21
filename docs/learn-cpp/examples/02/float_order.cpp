// Chapter 2 toy: floating-point numbers are not exact, and the order of additions matters.
#include <iostream>

int main() {
    std::cout.precision(17);
    double a = 0.1 + 0.2;
    std::cout << "0.1 + 0.2        = " << a << std::endl;
    std::cout << "(0.1 + 0.2 == 0.3) is " << (a == 0.3 ? "true" : "false") << std::endl;

    double big = 1e16;
    std::cout << "(1e16 + 1) - 1e16 = " << (big + 1.0) - big << std::endl;
    std::cout << "(1e16 - 1e16) + 1 = " << (big - big) + 1.0 << std::endl;
    return 0;
}
