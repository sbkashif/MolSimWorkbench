// Chapter 2 toy: a variable declared without a value holds whatever was in that memory.
#include <iostream>

int main() {
    double energy;              // no "= 0.0"
    energy += 1.5;              // adds to garbage
    std::cout << "energy = " << energy << std::endl;
    return 0;
}
