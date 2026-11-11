// Chapter 4 toy: std::vector basics, copying, bounds, and what happens when it grows.
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

using Vec3 = std::array<double, 3>;

int main() {
    std::vector<double> xs;                 // empty, like xs = []
    xs.push_back(1.0);                      // xs.append(1.0)
    xs.push_back(2.0);
    std::cout << "size = " << xs.size() << ", xs[1] = " << xs[1] << std::endl;

    std::vector<double> zeros(4, 0.0);      // round brackets: (how many, of what) -> [0.0] * 4
    std::vector<double> listed = {4, 0.0};  // braces: the elements themselves -> [4.0, 0.0]
    std::cout << "zeros.size() = " << zeros.size() << ", listed.size() = " << listed.size() << std::endl;
    std::cout << "sizeof(xs) = " << sizeof(xs) << ", sizeof(zeros) = " << sizeof(zeros) << std::endl;

    // Assignment copies every element.
    std::vector<double> ys = xs;
    ys[0] = 99.0;
    std::cout << "after ys[0] = 99: xs[0] = " << xs[0] << ", ys[0] = " << ys[0] << std::endl;

    // A vector of Vec3: one block of numbers, x y z x y z ...
    std::vector<Vec3> positions = {{0.0, 0.0, 0.0}, {1.2, 0.0, 0.0}};
    std::cout << "positions[1][0] = " << positions[1][0] << std::endl;

    // [] does not check the index. .at() does.
    try {
        std::cout << xs.at(5) << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "xs.at(5) threw std::out_of_range" << std::endl;
    }

    // Growing moves the storage: an old address is no longer where the data lives.
    std::vector<double> grow;
    grow.push_back(0.0);
    const double* before = grow.data();
    for (int i = 0; i < 100; ++i) grow.push_back(i);
    const double* after = grow.data();
    std::cout << "storage moved while growing: " << (before != after ? "yes" : "no") << std::endl;

    // size() is unsigned: on an empty vector, size() - 1 wraps around.
    std::vector<double> empty;
    std::cout << "empty.size() - 1 = " << empty.size() - 1 << std::endl;
    return 0;
}
