// A one-argument constructor can act as a silent conversion unless it is `explicit`.
#include <iostream>

struct LooseTether {
    LooseTether(double k) : k(k) {}
    double k;
};
struct StrictTether {
    explicit StrictTether(double k) : k(k) {}
    double k;
};

void report(const LooseTether& t) { std::cout << "LooseTether with k = " << t.k << std::endl; }
void report(const StrictTether& t) { std::cout << "StrictTether with k = " << t.k << std::endl; }

int main() {
    report(0.1);                  // compiles: 0.1 silently becomes a LooseTether
    report(StrictTether(0.1));    // fine: we said which type we meant
#ifdef SHOW_ERROR
    StrictTether t = 0.1;         // refused: no silent conversion
#endif
    return 0;
}
