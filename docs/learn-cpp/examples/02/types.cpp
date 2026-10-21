// Chapter 2 toy: sizes of the basic types, integer division, and printing precision.
#include <cstddef>
#include <iostream>

int main() {
    double x = 1.0;
    int n = 2;
    bool ok = true;
    std::size_t count = 125;

    std::cout << "sizeof(double)      = " << sizeof(x) << " bytes" << std::endl;
    std::cout << "sizeof(int)         = " << sizeof(n) << " bytes" << std::endl;
    std::cout << "sizeof(bool)        = " << sizeof(ok) << " byte" << std::endl;
    std::cout << "sizeof(std::size_t) = " << sizeof(count) << " bytes" << std::endl;

    std::cout << "1 / 2     = " << 1 / 2 << std::endl;
    std::cout << "1.0 / 2   = " << 1.0 / 2 << std::endl;
    std::cout << "7 / 2     = " << 7 / 2 << std::endl;
    std::cout << "7 % 2     = " << 7 % 2 << std::endl;
    std::cout << "-7 / 2    = " << -7 / 2 << std::endl;

    int truncated = 2.9;   // a double squeezed into an int: the .9 is dropped
    std::cout << "int truncated = 2.9 -> " << truncated << std::endl;

    std::size_t zero = 0;
    std::cout << "zero - 1  = " << zero - 1 << std::endl;   // unsigned: wraps around

    double third = 1.0 / 3.0;
    std::cout << "1/3 default   : " << third << std::endl;
    std::cout.precision(12);
    std::cout << "1/3 precision12: " << third << std::endl;
    return 0;
}
