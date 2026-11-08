#ifndef MD_STATE_H
#define MD_STATE_H

#include <array>
#include <vector>
#include "../builder/geometry.h"
#include "force_calculator.h"
#include "../utils/trajectory_writer.h" // SimulationBox

// What an integrator or a thermostat is allowed to touch during one step.
//
// MolecularDynamics owns the positions, velocities and forces; it hands this
// view to its Integrator and Thermostat plugs, the same way ForceCalculator
// hands a Geometry to each Potential. Mass is 1.0 for every atom (reduced
// units), so force and acceleration are the same number.
struct MDState {
    Geometry& geometry;
    std::vector<std::array<double, 3>>& velocities;
    std::vector<std::array<double, 3>>& forces;
    double& potentialEnergy;             // matches `forces` (same positions)
    ForceCalculator& forceCalculator;
    bool usePBC = false;
    SimulationBox box;

    // Forces and potential energy at the current positions.
    void computeForces();

    // Wraps every atom back into the (orthorhombic) box. No-op without PBC.
    void wrapPositions();

    // Kinetic energy, 1/2 sum v^2 (mass 1).
    double kineticEnergy() const;

    // Instantaneous temperature, 2 KE / (3 N) with kB = 1.
    double temperature() const;
};

#endif // MD_STATE_H
