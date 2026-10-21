// A raw pointer into a vector goes stale when the vector moves to a bigger block of memory.
#include <iostream>
#include <vector>

int main() {
    std::vector<double> sx(4, 1.0);
    const double* x = sx.data();   // non-owning: sx still owns the numbers
    std::cout << "capacity " << sx.capacity() << ", x == sx.data(): " << (x == sx.data()) << std::endl;

    sx.resize(1000);               // too big for the old block: new block, old one freed
    std::cout << "capacity " << sx.capacity() << ", x == sx.data(): " << (x == sx.data()) << std::endl;

    std::cout << "x[0] = " << x[0] << std::endl;   // reads freed memory: undefined behaviour
    return 0;
}
