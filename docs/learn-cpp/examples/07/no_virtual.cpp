// Without `virtual`, a call through a base pointer runs the BASE version.
#include <iostream>
#include <string>

struct PlainBase {
    std::string name() const { return "PlainBase"; }
};
struct PlainDerived : PlainBase {
    std::string name() const { return "PlainDerived"; }
};

struct VirtualBase {
    virtual ~VirtualBase() = default;
    virtual std::string name() const { return "VirtualBase"; }
};
struct VirtualDerived : VirtualBase {
    std::string name() const override { return "VirtualDerived"; }
};

int main() {
    PlainDerived pd;
    VirtualDerived vd;
    PlainBase* p = &pd;
    VirtualBase* v = &vd;
    std::cout << "plain:   pd.name() = " << pd.name() << ",  p->name() = " << p->name() << std::endl;
    std::cout << "virtual: vd.name() = " << vd.name() << ", v->name() = " << v->name() << std::endl;
    return 0;
}
