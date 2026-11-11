// Chapter 2 toy: auto takes the type from the right-hand side, once, at compile time.
#include <iostream>

int main() {
    auto a = 2;       // int
    auto b = 2.0;     // double
    auto c = a / 4;   // int / int -> int
    auto d = b / 4;   // double / int -> double
    std::cout << "a/4 = " << c << ", b/4 = " << d << std::endl;
    std::cout << "sizeof(a) = " << sizeof(a) << ", sizeof(b) = " << sizeof(b) << std::endl;
    return 0;
}
