// What a lambda captures: a copy taken now ([x]) or the variable itself ([&x]).
#include <iostream>

int main() {
    int t = 0;
    auto byValue = [t]() { return t; };       // copies t now: the copy holds 0
    auto byReference = [&t]() { return t; };  // refers to t itself
    t = 5;
    std::cout << "byValue() = " << byValue() << ", byReference() = " << byReference() << std::endl;

    int calls = 0;
    auto count = [&calls]() { ++calls; };     // by reference, so it can change the caller's variable
    count();
    count();
    std::cout << "calls = " << calls << std::endl;
    return 0;
}
