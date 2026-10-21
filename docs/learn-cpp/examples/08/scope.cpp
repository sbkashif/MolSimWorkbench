// Stack lifetime: an object is destroyed at the closing brace of the block it was created in.
#include <iostream>
#include <string>

class Noisy {
public:
    explicit Noisy(const std::string& name) : name(name) {
        std::cout << "  construct " << name << std::endl;
    }
    ~Noisy() { std::cout << "  destroy   " << name << std::endl; }   // the destructor

private:
    std::string name;
};

int main() {
    std::cout << "enter main" << std::endl;
    Noisy a("a");
    {
        std::cout << "enter inner block" << std::endl;
        Noisy b("b");
        Noisy c("c");
        std::cout << "leave inner block" << std::endl;
    }   // c, then b, are destroyed here: reverse order of creation
    std::cout << "leave main" << std::endl;
    return 0;
}   // a is destroyed here
