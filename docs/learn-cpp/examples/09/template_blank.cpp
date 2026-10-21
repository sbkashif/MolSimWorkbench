// A function template: a recipe with a blank (T). The compiler writes one real function per type used.
#include <iostream>
#include <string>
#include <vector>

template <typename T>
T twice(T x) {
    return x + x;
}

int main() {
    std::cout << twice(21) << std::endl;                  // T = int
    std::cout << twice(1.25) << std::endl;                // T = double
    std::cout << twice(std::string("ab")) << std::endl;   // T = std::string
#ifdef SHOW_ERROR
    std::vector<int> v = {1, 2};
    twice(v);                                             // T = std::vector<int>: no '+' for that
#endif
    return 0;
}
