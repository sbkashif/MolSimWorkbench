// A class with virtual functions carries one hidden pointer per object.
#include <iostream>

struct Plain {
    double k;
    double energy() const { return k; }
};
struct WithVirtual {
    virtual ~WithVirtual() = default;
    virtual double energy() const { return k; }
    double k;
};

int main() {
    std::cout << "sizeof(double)      = " << sizeof(double) << std::endl;
    std::cout << "sizeof(Plain)       = " << sizeof(Plain) << std::endl;
    std::cout << "sizeof(WithVirtual) = " << sizeof(WithVirtual) << std::endl;
    return 0;
}
