// Chapter 2 toy: a variable's type is fixed. Python would happily rebind x to a string.
#include <string>

int main() {
    double x = 1.0;
    x = "hello";            // error: x is a double, forever
    const double cutoff = 2.5;
    cutoff = 3.0;           // error: const cannot be assigned again
    int n{2.9};             // error: brace initialisation refuses to lose the .9
    return 0;
}
