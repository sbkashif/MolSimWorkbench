// unique_ptr (one owner) and shared_ptr (counted owners) do the delete for you.
#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Noisy {
public:
    explicit Noisy(const std::string& name) : name(name) {
        std::cout << "  construct " << name << std::endl;
    }
    ~Noisy() { std::cout << "  destroy   " << name << std::endl; }

private:
    std::string name;
};

int main() {
    std::cout << "-- unique_ptr" << std::endl;
    {
        std::unique_ptr<Noisy> u = std::make_unique<Noisy>("u");
        // std::unique_ptr<Noisy> copy = u;          // does not compile: only one owner allowed
        std::unique_ptr<Noisy> moved = std::move(u); // hand ownership over
        std::cout << "  u is " << (u ? "full" : "empty") << " after the move" << std::endl;
    }   // 'moved' goes out of scope: destroy

    std::cout << "-- shared_ptr" << std::endl;
    std::shared_ptr<Noisy> first = std::make_shared<Noisy>("s");
    std::cout << "  use_count = " << first.use_count() << std::endl;
    {
        std::shared_ptr<Noisy> second = first;       // copying a shared_ptr adds an owner
        std::cout << "  use_count = " << first.use_count() << std::endl;
    }   // 'second' gone: count drops, object stays
    std::cout << "  use_count = " << first.use_count() << std::endl;
    first.reset();                                   // last owner lets go: destroy
    std::cout << "end of main" << std::endl;
    return 0;
}
