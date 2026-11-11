#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// The socket: what every energy function must provide.
class Potential {
public:
    virtual ~Potential() = default;
    virtual double energy(double r) const = 0;
    virtual std::string name() const = 0;
};

// Plug 1: Lennard-Jones.
class LJPotential : public Potential {
public:
    LJPotential(double epsilon, double sigma, double cutoff) {
        epsilon_ = epsilon;
        sigma_ = sigma;
        cutoff_ = cutoff;
    }
    double energy(double r) const override {
        if (r > cutoff_) {
            return 0.0;
        }
        double sr6 = std::pow(sigma_ / r, 6);
        return 4.0 * epsilon_ * (sr6 * sr6 - sr6);
    }
    std::string name() const override { return "Lennard-Jones"; }

private:
    double epsilon_;
    double sigma_;
    double cutoff_;
};

// Plug 2: a soft repulsion, A (1 - r/rc)^2 inside rc, zero outside.
class SoftRepulsion : public Potential {
public:
    SoftRepulsion(double strength, double range) {
        strength_ = strength;
        range_ = range;
    }
    double energy(double r) const override {
        if (r > range_) {
            return 0.0;
        }
        double x = 1.0 - r / range_;
        return strength_ * x * x;
    }
    std::string name() const override { return "soft repulsion"; }

private:
    double strength_;
    double range_;
};

// The force step: holds any number of plugs and adds them up.
class ForceCalculator {
public:
    void addPotential(std::shared_ptr<Potential> potential) {
        potentials_.push_back(potential);
    }
    double energy(double r) const {
        double total = 0.0;
        for (const std::shared_ptr<Potential>& p : potentials_) {
            total += p->energy(r);
        }
        return total;
    }
    void describe() const {
        for (const std::shared_ptr<Potential>& p : potentials_) {
            std::cout << "  plug: " << p->name() << "\n";
        }
    }

private:
    std::vector<std::shared_ptr<Potential>> potentials_;
};

int main() {
    ForceCalculator calculator;
    calculator.addPotential(std::make_shared<LJPotential>(1.0, 1.0, 2.5));
    std::cout << "LJ only:          U(0.95) = " << calculator.energy(0.95)
              << ", U(1.5) = " << calculator.energy(1.5) << "\n";

    calculator.addPotential(std::make_shared<SoftRepulsion>(10.0, 1.0));
    calculator.describe();
    std::cout << "LJ + soft:        U(0.95) = " << calculator.energy(0.95)
              << ", U(1.5) = " << calculator.energy(1.5) << "\n";
    return 0;
}
