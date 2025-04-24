#ifndef MD_PARAMETERS_H
#define MD_PARAMETERS_H

// Structure to hold molecular dynamics simulation parameters
struct MDParameters {
    double timeStep = 0.001;           // Time step in ps
    int maxSteps = 1000;               // Maximum number of simulation steps
    double temperature = 300.0;        // Target temperature in K
    bool useConstantTemperature = false; // Whether to use temperature coupling
    double couplingConstant = 0.1;     // Temperature coupling constant
    int trajectoryFrequency = 10;      // How often to save trajectory frames
    int energyFrequency = 10;          // How often to save energy data
    std::string trajectoryFile = "trajectory.xyz"; // Output trajectory file
    std::string energyFile = "energy.dat";         // Output energy file
    bool writeTrajectory = true;       // Whether to write trajectory
    bool writeEnergy = true;           // Whether to write energy file
};

#endif // MD_PARAMETERS_H