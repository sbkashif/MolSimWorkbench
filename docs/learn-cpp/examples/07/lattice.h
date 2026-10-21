#ifndef LATTICE_H
#define LATTICE_H

#include <cstddef>
#include "geometry.h"

Geometry makeCubicLattice(std::size_t numAtoms, double density, unsigned seed);

#endif
