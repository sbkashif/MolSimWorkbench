// Copying a derived object into a base VALUE keeps only the base part.
#include <iostream>
#include <string>
#include <vector>

struct Base {
    virtual ~Base() = default;
    virtual std::string name() const { return "Base"; }
};
struct Derived : Base {
    std::string name() const override { return "Derived"; }
    double extra = 42.0;
};

int main() {
    Derived d;
    Base copy = d;            // a new Base object, copied from d's Base part
    Base& ref = d;            // a second name for d itself
    std::cout << "copy.name() = " << copy.name() << std::endl;
    std::cout << "ref.name()  = " << ref.name() << std::endl;

    std::vector<Base> values = {d};      // each element is a Base: sliced
    std::vector<Base*> pointers = {&d};  // each element points at the real object
    std::cout << "values[0].name()    = " << values[0].name() << std::endl;
    std::cout << "pointers[0]->name() = " << pointers[0]->name() << std::endl;
    return 0;
}
