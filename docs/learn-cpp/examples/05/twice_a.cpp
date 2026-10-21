#include <iostream>
#include "twice.h"

double useHeaderInB();   // defined in twice_b.cpp

int main() {
    std::cout << twice(1.5) << " " << thrice(1.5) << " " << useHeaderInB() << std::endl;
    return 0;
}
