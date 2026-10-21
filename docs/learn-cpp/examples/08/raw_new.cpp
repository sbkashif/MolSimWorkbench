// new / delete by hand. Forget the delete and the destructor never runs.
#include <iostream>
#include <string>

class Noisy {
public:
    explicit Noisy(const std::string& name) : name(name) {
        std::cout << "  construct " << name << std::endl;
    }
    ~Noisy() { std::cout << "  destroy   " << name << std::endl; }

private:
    std::string name;
};

Noisy* makeOnHeap(const std::string& name) {
    return new Noisy(name);    // storage outlives this function: fine
}

int main() {
    Noisy* kept = makeOnHeap("kept");
    Noisy* forgotten = makeOnHeap("forgotten");
    delete kept;               // destructor runs, storage returned
    (void)forgotten;           // no delete: a leak. Its destructor never runs.
    std::cout << "end of main" << std::endl;
    return 0;
}
