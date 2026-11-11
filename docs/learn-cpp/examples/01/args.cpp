#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: args <name>" << std::endl;
        return 1;
    }
    const std::string name = argv[1];
    std::cout << "argc = " << argc << std::endl;
    std::cout << "Hello, " << name << std::endl;
    return 0;
}
