#include <iostream>
#include <vector>

int main() {
    std::vector<double> distances = {1.0, 1.122462, 1.5, 2.5};
    std::cout << "distances[10]    = " << distances[10] << "\n";
    std::cout << "distances.at(10) = " << distances.at(10) << "\n";
    return 0;
}
