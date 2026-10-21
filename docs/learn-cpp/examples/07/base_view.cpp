// Through a Potential* you can only call what Potential promises.
#include "lj_potential.h"

int main() {
    LJPotential lj;
    lj.setCutoff(2.5);          // fine: lj is an LJPotential
    Potential* p = &lj;
    p->setCutoff(3.0);          // refused: Potential has no setCutoff
    return 0;
}
