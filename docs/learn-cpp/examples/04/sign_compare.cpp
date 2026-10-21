// Chapter 4 toy: comparing an int index with a container's size() mixes signed and unsigned.
#include <cstddef>
#include <vector>

int main() {
    std::vector<double> v(3, 0.0);
    double sum = 0.0;
    for (int i = 0; i < v.size(); ++i) sum += v[i];           // int vs std::size_t
    for (std::size_t i = 0; i < v.size(); ++i) sum += v[i];   // same type: no warning
    return static_cast<int>(sum);
}
