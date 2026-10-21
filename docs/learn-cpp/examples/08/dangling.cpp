// Returning the address of a local variable. The compiler warns; the pointer is useless.
#include <iostream>

double* makeCutoff() {
    double cutoff = 2.5;   // lives in makeCutoff's stack frame
    return &cutoff;        // the address of something destroyed at the next '}'
}

void otherWork() {
    volatile double scratch[4] = {111.0, 222.0, 333.0, 444.0};   // reuses the same stack space
    (void)scratch;
}

int main() {
    double* p = makeCutoff();
    otherWork();
    std::cout << "*p = " << *p << std::endl;   // undefined behaviour: anything may happen
    return 0;
}
