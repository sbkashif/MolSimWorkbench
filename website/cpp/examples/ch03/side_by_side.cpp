#include <iostream>
#include <vector>

int main() {
    std::vector<double> distances = {1.0, 1.122462, 1.5, 2.5};
    for (std::size_t i = 0; i < distances.size(); i++) {
        std::cout << "distances[" << i << "] = " << distances[i]
                  << " at " << &distances[i] << "\n";
    }
    return 0;
}
