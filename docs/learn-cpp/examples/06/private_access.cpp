// private members can be used inside the class, not outside it.
#include <iostream>

class Cutoff {
public:
    void set(double value) { cutoff = value; }   // inside the class: allowed
    double get() const { return cutoff; }

private:
    double cutoff = 10.0;
};

int main() {
    Cutoff c;
    c.set(2.5);
    std::cout << c.get() << std::endl;
    c.cutoff = -1.0;                             // outside the class: refused
    return 0;
}
