// Chapter 3 toy: a pointer holds an address. &x takes one, *p follows one.
#include <array>
#include <iostream>

int main() {
    double a = 1.0;
    double b = 2.0;

    double* p = &a;                 // p holds the address of a
    std::cout << "p points at a, *p = " << *p << std::endl;
    *p = 10.0;                      // write through the pointer
    std::cout << "after *p = 10:   a = " << a << std::endl;

    p = &b;                         // unlike a reference, a pointer can be re-pointed
    std::cout << "p re-pointed,  *p = " << *p << std::endl;

    p = nullptr;                    // points at nothing
    if (p == nullptr) std::cout << "p is nullptr now" << std::endl;

    std::array<double, 3> v = {4.0, 5.0, 6.0};
    const double* first = v.data(); // address of v[0]
    std::cout << "first[0] = " << first[0] << ", first[2] = " << first[2] << std::endl;

    std::cout << "sizeof(double*) = " << sizeof(p) << ", sizeof(v) = " << sizeof(v) << std::endl;
    std::cout << "&v[0] = " << &v[0] << ", &v[1] = " << &v[1] << std::endl;
    return 0;
}
