#ifndef TRAJECTORY_WRITER_H
#define TRAJECTORY_WRITER_H

#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <memory>
#include "../builder/geometry.h"

// Structure to represent a simulation box (for PBC)
struct SimulationBox {
    double a, b, c;          // Box lengths
    double alpha, beta, gamma; // Box angles (in degrees)
    
    // Default constructor
    SimulationBox() : a(0.0), b(0.0), c(0.0), alpha(90.0), beta(90.0), gamma(90.0) {}
    
    // Constructor with box parameters
    SimulationBox(double a, double b, double c, double alpha = 90.0, double beta = 90.0, double gamma = 90.0)
        : a(a), b(b), c(c), alpha(alpha), beta(beta), gamma(gamma) {}
};

// Enum for supported trajectory file formats
enum class TrajectoryFormat {
    XYZ,        // Simple XYZ format (text-based, widely supported)
    DCD,        // Binary DCD format (CHARMM/NAMD, supported by VMD)
    XTC         // Compressed binary XTC format (GROMACS, very space-efficient)
};

class TrajectoryWriter {
public:
    // Constructor that opens a trajectory file for writing
    TrajectoryWriter(const std::string& filename, TrajectoryFormat format = TrajectoryFormat::XYZ, bool dryRun = false);
    
    // Destructor to close the file
    ~TrajectoryWriter();
    
    // Write a single frame to the trajectory file
    bool writeFrame(const Geometry& geometry, double time = 0.0, const std::string& comment = "");
    
    // Write a frame with additional velocity data (if supported by format)
    bool writeFrameWithVelocities(const Geometry& geometry, 
                                 const std::vector<std::array<double, 3>>& velocities,
                                 double time = 0.0, 
                                 const std::string& comment = "");
    
    // Close the trajectory file
    void close();
    
    // Check if the trajectory file is open and ready for writing
    bool isOpen() const;
    
    // Get the current file format
    TrajectoryFormat getFormat() const { return format; }
    
    // Get/set the trajectory writing frequency (write every N steps)
    void setWriteFrequency(int frequency) { writeFrequency = frequency; }
    int getWriteFrequency() const { return writeFrequency; }
    
    // Set the simulation box parameters (for formats that support it)
    void setBox(const SimulationBox& box) { simulationBox = box; }
    
    // Determine if a frame should be written based on step number
    bool shouldWriteFrame(int step) const { return (step % writeFrequency) == 0; }
    
    // Check if writer is in dry run mode (no file writing)
    bool isDryRun() const { return dryRun; }

private:
    std::string filename;
    TrajectoryFormat format;
    std::ofstream outputFile;
    int writeFrequency;  // Write every Nth frame
    int frameCount;      // Current frame count
    bool dryRun;         // If true, don't actually write to files
    SimulationBox simulationBox; // Simulation box parameters for PBC
    std::streampos headerPos;   // Position of header in DCD file for updating frame count
    
    // Format-specific helper methods
    bool writeXYZFrame(const Geometry& geometry, double time, const std::string& comment);
    bool writeDCDFrame(const Geometry& geometry);
    bool writeXTCFrame(const Geometry& geometry, double time);
    
    // Helper method for creating comment line
    std::string createComment(double time, const std::string& comment) const;
    
    // Write DCD header (called once when first frame is written)
    void writeDCDHeader(int numAtoms, double timeStep);
    
    // Helper methods for XTC compressed format
    void initializeXTC();
    void finalizeXTC();
};

#endif // TRAJECTORY_WRITER_H