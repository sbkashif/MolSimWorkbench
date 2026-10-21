#ifndef TWICE_H
#define TWICE_H

// Two function BODIES in a header. Only one is marked `inline`.
double twice(double x) { return 2.0 * x; }
inline double thrice(double x) { return 3.0 * x; }

#endif
