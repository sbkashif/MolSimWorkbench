#include "trajectory_writer.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <sstream>

TrajectoryWriter::TrajectoryWriter(const std::string& filename, TrajectoryFormat format, bool dryRun)
    : filename(filename), format(format), writeFrequency(1), frameCount(0), dryRun(dryRun) {
    
    // Don't open a file if in dry run mode
    if (dryRun) {
        return;
    }
    
    // Strip extension if already present to avoid double extensions
    std::string baseFilename = filename;
    size_t dotPos = baseFilename.rfind('.');
    if (dotPos != std::string::npos) {
        std::string ext = baseFilename.substr(dotPos);
        if (ext == ".xyz" || ext == ".dcd" || ext == ".xtc") {
            baseFilename = baseFilename.substr(0, dotPos);
        }
    }
    
    // Open the output file based on format
    switch (format) {
        case TrajectoryFormat::XYZ:
            outputFile.open(baseFilename + ".xyz");
            break;
            
        case TrajectoryFormat::DCD:
            // DCD is a binary format
            outputFile.open(baseFilename + ".dcd", std::ios::binary);
            break;
            
        case TrajectoryFormat::XTC:
            // Currently, we'll just use XYZ as a fallback for XTC format
            // since XTC requires external libraries (libxdrfile)
            std::cout << "Warning: XTC format not fully implemented. Using XYZ instead." << std::endl;
            outputFile.open(baseFilename + ".xyz");
            break;
    }
    
    if (!outputFile.is_open() && !dryRun) {
        std::cerr << "Error: Could not open trajectory file " << filename << " for writing." << std::endl;
    }
}

TrajectoryWriter::~TrajectoryWriter() {
    close();
}

bool TrajectoryWriter::writeFrame(const Geometry& geometry, double time, const std::string& comment) {
    // If in dry run mode, just pretend we wrote the frame
    if (dryRun) {
        frameCount++;
        return true;
    }
    
    if (!outputFile.is_open()) {
        return false;
    }
    
    bool success = false;
    
    // Call the appropriate format-specific writer
    switch (format) {
        case TrajectoryFormat::XYZ:
            success = writeXYZFrame(geometry, time, comment);
            break;
            
        case TrajectoryFormat::DCD:
            // For the first frame, write the header
            if (frameCount == 0) {
                writeDCDHeader(geometry.getNumberOfAtoms(), 0.001); // Default timestep
            }
            success = writeDCDFrame(geometry);
            break;
            
        case TrajectoryFormat::XTC:
            // Currently using XYZ as fallback
            success = writeXYZFrame(geometry, time, comment);
            break;
    }
    
    if (success) {
        frameCount++;
        // Flush the output to ensure data is written to disk
        outputFile.flush();
    }
    
    return success;
}

bool TrajectoryWriter::writeFrameWithVelocities(const Geometry& geometry, 
                                               const std::vector<std::array<double, 3>>& velocities,
                                               double time, 
                                               const std::string& comment) {
    // If in dry run mode, just pretend we wrote the frame
    if (dryRun) {
        frameCount++;
        return true;
    }
    
    // Currently only the XYZ format supports storing velocities
    // In a custom way (we append them after coordinates)
    if (format != TrajectoryFormat::XYZ) {
        return writeFrame(geometry, time, comment);
    }
    
    if (!outputFile.is_open()) {
        return false;
    }
    
    // Get atom data
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    size_t numAtoms = atoms.size();
    
    // Write number of atoms
    outputFile << numAtoms << std::endl;
    
    // Write comment line with time
    outputFile << createComment(time, comment) << std::endl;
    
    // Write atom data: element, x, y, z, vx, vy, vz
    for (size_t i = 0; i < numAtoms; ++i) {
        outputFile << std::setw(5) << atoms[i] 
                  << std::fixed << std::setprecision(6)
                  << std::setw(12) << positions[i][0]
                  << std::setw(12) << positions[i][1]
                  << std::setw(12) << positions[i][2];
        
        // Add velocities if available
        if (i < velocities.size()) {
            outputFile << std::setw(12) << velocities[i][0]
                      << std::setw(12) << velocities[i][1]
                      << std::setw(12) << velocities[i][2];
        }
        
        outputFile << std::endl;
    }
    
    frameCount++;
    
    // Flush the output to ensure data is written to disk
    outputFile.flush();
    
    return true;
}

void TrajectoryWriter::close() {
    if (outputFile.is_open()) {
        // For DCD format, update the number of frames in the header
        if (format == TrajectoryFormat::DCD && frameCount > 0 && !dryRun) {
            try {
                // Save current position
                std::streampos currentPos = outputFile.tellp();
                
                // Go back to header position + 4 (skip past the identifier "CORD")
                outputFile.seekp(4 + sizeof(int32_t), std::ios::beg);
                
                // Write the actual number of frames
                int32_t numFrames = frameCount;
                outputFile.write(reinterpret_cast<char*>(&numFrames), sizeof(int32_t));
                
                // Return to previous position
                outputFile.seekp(currentPos);
                
                std::cout << "Updated DCD header with final frame count: " << frameCount << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error updating DCD header: " << e.what() << std::endl;
            }
        }
        
        outputFile.close();
    }
}

bool TrajectoryWriter::isOpen() const {
    return outputFile.is_open();
}

std::string TrajectoryWriter::createComment(double time, const std::string& comment) const {
    std::stringstream ss;
    
    // Add the original comment if provided
    if (!comment.empty()) {
        ss << comment;
    } else {
        ss << "Frame " << frameCount << ", Time: " << time << " ps";
    }
    
    return ss.str();
}

bool TrajectoryWriter::writeXYZFrame(const Geometry& geometry, double time, const std::string& comment) {
    // Get atom data
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    size_t numAtoms = atoms.size();
    
    // Write number of atoms
    outputFile << numAtoms << std::endl;
    
    // Write comment line with time
    outputFile << createComment(time, comment) << std::endl;
    
    // Write atom data: element, x, y, z
    for (size_t i = 0; i < numAtoms; ++i) {
        outputFile << std::setw(5) << atoms[i] 
                  << std::fixed << std::setprecision(6)
                  << std::setw(12) << positions[i][0]
                  << std::setw(12) << positions[i][1]
                  << std::setw(12) << positions[i][2]
                  << std::endl;
    }
    
    return true;
}

bool TrajectoryWriter::writeDCDFrame(const Geometry& geometry) {
    // DCD format is binary and complex with specific block structure
    const auto& positions = geometry.getPositions();
    size_t numAtoms = positions.size();
    
    try {
        // Write box information (unit cell) - 6 doubles stored in a specific format for CHARMM
        int32_t boxBlockSize = 48;  // Always 48 bytes for box info (6 doubles)
        outputFile.write(reinterpret_cast<char*>(&boxBlockSize), sizeof(int32_t));
        
        // VMD expects these in the order: A, gamma, B, beta, alpha, C
        // But we need to convert angles to cosines
        double cosAlpha = cos(simulationBox.alpha * M_PI / 180.0);
        double cosBeta = cos(simulationBox.beta * M_PI / 180.0);
        double cosGamma = cos(simulationBox.gamma * M_PI / 180.0);
        
        double unitCell[6] = {
            simulationBox.a,     // A (box length along x)
            cosGamma,            // cos(gamma) (cosine of angle between a and b)
            simulationBox.b,     // B (box length along y)
            cosBeta,             // cos(beta) (cosine of angle between a and c)
            cosAlpha,            // cos(alpha) (cosine of angle between b and c)
            simulationBox.c      // C (box length along z)
        };
        
        // Write the 6 box dimensions as double precision (6 * 8 = 48 bytes)
        outputFile.write(reinterpret_cast<char*>(unitCell), boxBlockSize);
        
        // Write ending box block size marker
        outputFile.write(reinterpret_cast<char*>(&boxBlockSize), sizeof(int32_t));
        
        // Calculate coordinate block size - must be numAtoms * sizeof(float)
        int32_t coordBlockSize = static_cast<int32_t>(numAtoms * sizeof(float));
        
        // Write X coordinates
        outputFile.write(reinterpret_cast<char*>(&coordBlockSize), sizeof(int32_t));
        
        std::vector<float> xcoords(numAtoms);
        for (size_t i = 0; i < numAtoms; ++i) {
            xcoords[i] = static_cast<float>(positions[i][0]);
        }
        outputFile.write(reinterpret_cast<char*>(xcoords.data()), coordBlockSize);
        
        outputFile.write(reinterpret_cast<char*>(&coordBlockSize), sizeof(int32_t));
        
        // Write Y coordinates
        outputFile.write(reinterpret_cast<char*>(&coordBlockSize), sizeof(int32_t));
        
        std::vector<float> ycoords(numAtoms);
        for (size_t i = 0; i < numAtoms; ++i) {
            ycoords[i] = static_cast<float>(positions[i][1]);
        }
        outputFile.write(reinterpret_cast<char*>(ycoords.data()), coordBlockSize);
        
        outputFile.write(reinterpret_cast<char*>(&coordBlockSize), sizeof(int32_t));
        
        // Write Z coordinates
        outputFile.write(reinterpret_cast<char*>(&coordBlockSize), sizeof(int32_t));
        
        std::vector<float> zcoords(numAtoms);
        for (size_t i = 0; i < numAtoms; ++i) {
            zcoords[i] = static_cast<float>(positions[i][2]);
        }
        outputFile.write(reinterpret_cast<char*>(zcoords.data()), coordBlockSize);
        
        outputFile.write(reinterpret_cast<char*>(&coordBlockSize), sizeof(int32_t));
        
        return outputFile.good();
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing DCD frame: " << e.what() << std::endl;
        return false;
    }
}

void TrajectoryWriter::writeDCDHeader(int numAtoms, double timeStep) {
    try {
        // 1. File header (84 bytes + 8 bytes for block size markers)
        int32_t headerBlockSize = 84;
        
        // Write block size marker
        outputFile.write(reinterpret_cast<char*>(&headerBlockSize), sizeof(int32_t));
        
        // "CORD" signature (coordinates file)
        char signature[4] = {'C', 'O', 'R', 'D'};
        outputFile.write(signature, 4);
        
        // Various header fields (standard DCD format)
        int32_t nset = 0;             // Number of frames - will be updated on close
        int32_t istart = 1;           // Starting timestep (1-based)
        int32_t nsavc = writeFrequency; // Steps between frames
        int32_t nstep = 1000;         // Total number of steps in simulation
        
        // Write the key header fields
        outputFile.write(reinterpret_cast<char*>(&nset), sizeof(int32_t));
        outputFile.write(reinterpret_cast<char*>(&istart), sizeof(int32_t));
        outputFile.write(reinterpret_cast<char*>(&nsavc), sizeof(int32_t));
        outputFile.write(reinterpret_cast<char*>(&nstep), sizeof(int32_t));
        
        // Write 5 zeros (for unit cell information)
        int32_t zero = 0;
        for (int i = 0; i < 5; i++) {
            outputFile.write(reinterpret_cast<char*>(&zero), sizeof(int32_t));
        }
        
        // IMPORTANT: CHARMM version flags - we need to set these to indicate CHARMM format
        // This is crucial for VMD to recognize the format correctly
        int32_t charmm_format = 1;  // Using CHARMM format (not X-PLOR)
        outputFile.write(reinterpret_cast<char*>(&charmm_format), sizeof(int32_t));
        
        // Write 4 more zeros
        for (int i = 0; i < 4; i++) {
            outputFile.write(reinterpret_cast<char*>(&zero), sizeof(int32_t));
        }
        
        // Write timestep as double precision (8 bytes)
        // Convert from our time units (ps) to DCD time units (AKMA)
        double deltaT = timeStep * 4.888821; // Convert ps to AKMA time units
        outputFile.write(reinterpret_cast<char*>(&deltaT), sizeof(double));
        
        // Set has_unit_cell flag to 1 for PBC
        int32_t has_unit_cell = 1;
        outputFile.write(reinterpret_cast<char*>(&has_unit_cell), sizeof(int32_t));
        
        // Write 4 more zeros
        for (int i = 0; i < 4; i++) {
            outputFile.write(reinterpret_cast<char*>(&zero), sizeof(int32_t));
        }
        
        // Write the compatibility format version - 24 for X-PLOR/NAMD, but we want CHARMM (>24)
        int32_t version = 24;
        outputFile.write(reinterpret_cast<char*>(&version), sizeof(int32_t));
        
        // Write ending block size marker
        outputFile.write(reinterpret_cast<char*>(&headerBlockSize), sizeof(int32_t));
        
        // 2. Title block (two 80-character titles preceded by block size and count)
        int32_t titleBlockSize = 80 * 2; // 2 title strings of 80 chars each
        outputFile.write(reinterpret_cast<char*>(&titleBlockSize), sizeof(int32_t));
        
        // Number of 80-character strings in the title
        int32_t numTitles = 2;
        outputFile.write(reinterpret_cast<char*>(&numTitles), sizeof(int32_t));
        
        // Title strings (must be exactly 80 characters each)
        char title1[80] = {0};
        char title2[80] = {0};
        
        // Clear the title buffers with spaces
        std::memset(title1, ' ', 80);
        std::memset(title2, ' ', 80);
        
        // Copy titles to buffers
        std::string title1Str = "Created by MolSimWorkbench - CHARMM format DCD";
        std::string title2Str = "Trajectory file with PBC";
        
        std::memcpy(title1, title1Str.c_str(), std::min(title1Str.length(), size_t(79)));
        std::memcpy(title2, title2Str.c_str(), std::min(title2Str.length(), size_t(79)));
        
        // Write titles
        outputFile.write(title1, 80);
        outputFile.write(title2, 80);
        
        // Write ending block size marker
        outputFile.write(reinterpret_cast<char*>(&titleBlockSize), sizeof(int32_t));
        
        // 3. Number of atoms block
        int32_t atomBlockSize = 4; // Just a single 32-bit integer
        outputFile.write(reinterpret_cast<char*>(&atomBlockSize), sizeof(int32_t));
        
        // Number of atoms
        int32_t atomCount = numAtoms;
        outputFile.write(reinterpret_cast<char*>(&atomCount), sizeof(int32_t));
        
        // Write ending block size marker
        outputFile.write(reinterpret_cast<char*>(&atomBlockSize), sizeof(int32_t));
        
        // Store file position for header update on close
        headerPos = outputFile.tellp();
        
        std::cout << "DCD header written successfully for " << numAtoms << " atoms." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing DCD header: " << e.what() << std::endl;
    }
}

bool TrajectoryWriter::writeXTCFrame(const Geometry& geometry, double time) {
    // XTC is a compressed binary format that requires the Gromacs XDR library
    // For now, we'll just use XYZ as a fallback
    return writeXYZFrame(geometry, time, "");
}

void TrajectoryWriter::initializeXTC() {
    // Placeholder for XTC initialization
}

void TrajectoryWriter::finalizeXTC() {
    // Placeholder for XTC finalization
}