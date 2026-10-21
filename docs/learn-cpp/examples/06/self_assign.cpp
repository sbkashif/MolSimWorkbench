// A parameter with the same name as a member hides the member.
#include <iostream>

class Potential {
public:
    void setCutoffWrong(double cutoff) { cutoff = cutoff; }        // parameter = parameter
    void setCutoffRight(double cutoff) { this->cutoff = cutoff; }  // member = parameter
    double getCutoff() const { return cutoff; }

private:
    double cutoff = 10.0;
};

int main() {
    Potential a;
    Potential b;
    a.setCutoffWrong(2.5);
    b.setCutoffRight(2.5);
    std::cout << "a: " << a.getCutoff() << "  b: " << b.getCutoff() << std::endl;
    return 0;
}
