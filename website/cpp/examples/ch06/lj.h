#ifndef LJ_H
#define LJ_H

// An LJ energy function: declarations only. The code is in lj.cpp.
class LJPotential {
public:
    LJPotential(double epsilon, double sigma, double cutoff);
    double energy(double r) const;

private:
    double epsilon_;
    double sigma_;
    double cutoff_;
};

#endif
