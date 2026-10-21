// A derived method that does not match the base EXACTLY is a new, unrelated method.
#include <iostream>
#include <string>

struct Base {
    virtual ~Base() = default;
    virtual std::string getName() const { return "unnamed"; }
};

struct Tether : Base {
    std::string getName() { return "Tether"; }               // forgot `const`
};

int main() {
    Tether t;
    Base* b = &t;
    std::cout << "t.getName()  = " << t.getName() << std::endl;
    std::cout << "b->getName() = " << b->getName() << std::endl;
    return 0;
}
