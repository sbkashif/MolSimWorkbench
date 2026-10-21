// A const object only allows calls to const methods.
#include <cstddef>
#include <vector>

class Atoms {
public:
    void add(double x) { xs.push_back(x); }
    std::size_t count() { return xs.size(); }    // forgot `const`

private:
    std::vector<double> xs;
};

std::size_t countAtoms(const Atoms& atoms) {
    return atoms.count();
}

int main() {
    Atoms atoms;
    atoms.add(1.0);
    return static_cast<int>(countAtoms(atoms));
}
