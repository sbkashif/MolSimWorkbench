// A default argument fills in a parameter the caller left out.
#include <iostream>

double scaledCutoff(double sigma, double factor = 2.5) {
    return factor * sigma;
}

int main() {
    std::cout << scaledCutoff(1.0) << std::endl;        // factor = 2.5
    std::cout << scaledCutoff(1.0, 3.0) << std::endl;   // factor = 3.0
    return 0;
}
