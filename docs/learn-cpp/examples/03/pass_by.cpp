// Chapter 3 toy: passing by value hands the function a copy; by reference, the caller's own variable.
#include <iostream>

void doubleByValue(double x) { x *= 2.0; }
void doubleByReference(double& x) { x *= 2.0; }

int main() {
    double v = 1.5;
    doubleByValue(v);
    std::cout << "after doubleByValue:     v = " << v << std::endl;
    doubleByReference(v);
    std::cout << "after doubleByReference: v = " << v << std::endl;
    return 0;
}
