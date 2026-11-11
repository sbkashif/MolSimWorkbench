// The same mistake, but the method says `override`: the compiler refuses it.
#include <string>

struct Base {
    virtual ~Base() = default;
    virtual std::string getName() const { return "unnamed"; }
};

struct Tether : Base {
    std::string getName() override { return "Tether"; }      // forgot `const`, said `override`
};

int main() {
    Tether t;
    return 0;
}
