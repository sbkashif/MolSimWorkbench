#ifndef LATTICE_H
#define LATTICE_H

#include <cstddef>
#include <vector>
#include "lj_math.h"

// Returns the positions; writes the box length into `box`.
std::vector<Vec3> makeCubicLattice(std::size_t numAtoms, double density, unsigned seed, double& box);

#endif
