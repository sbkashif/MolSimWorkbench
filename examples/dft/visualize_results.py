#!/usr/bin/env python3
"""
Simple Python visualization for DFT results using matplotlib
"""

import os
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def read_xyz_file(filename):
    """Read XYZ structure file"""
    atoms = []
    positions = []
    
    with open(filename, 'r') as f:
        lines = f.readlines()
        
    n_atoms = int(lines[0].strip())
    
    for i in range(2, 2 + n_atoms):
        parts = lines[i].strip().split()
        atom = parts[0]
        x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
        atoms.append(atom)
        positions.append([x, y, z])
    
    return atoms, np.array(positions)

def read_dft_results(filename):
    """Read DFT results data"""
    energy_hartree = None
    energy_ev = None
    forces = []
    
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith("# Energy (Hartree):"):
                energy_hartree = float(line.split(":")[1].strip())
            elif line.startswith("# Energy (eV):"):
                energy_ev = float(line.split(":")[1].strip())
            elif not line.startswith("#") and line.strip():
                parts = line.strip().split()
                if len(parts) >= 5:
                    atom = parts[0]
                    fx, fy, fz, mag = float(parts[1]), float(parts[2]), float(parts[3]), float(parts[4])
                    forces.append([atom, fx, fy, fz, mag])
    
    return energy_hartree, energy_ev, forces

def plot_molecular_structure(atoms, positions, forces, output_dir):
    """Create 3D plot of molecular structure with force vectors"""
    fig = plt.figure(figsize=(12, 10))
    
    # Structure plot
    ax1 = fig.add_subplot(221, projection='3d')
    
    colors = {'O': 'red', 'H': 'white', 'C': 'gray', 'N': 'blue'}
    sizes = {'O': 200, 'H': 100, 'C': 150, 'N': 150}
    
    for i, (atom, pos) in enumerate(zip(atoms, positions)):
        color = colors.get(atom, 'gray')
        size = sizes.get(atom, 100)
        ax1.scatter(pos[0], pos[1], pos[2], c=color, s=size, 
                   edgecolor='black', linewidth=1, alpha=0.8)
        ax1.text(pos[0], pos[1], pos[2] + 0.2, f'{atom}{i+1}', 
                fontsize=10, ha='center')
    
    ax1.set_xlabel('X (Bohr)')
    ax1.set_ylabel('Y (Bohr)')
    ax1.set_zlabel('Z (Bohr)')
    ax1.set_title('Molecular Structure')
    
    # Force vectors
    ax2 = fig.add_subplot(222, projection='3d')
    
    for i, (atom, pos) in enumerate(zip(atoms, positions)):
        color = colors.get(atom, 'gray')
        ax2.scatter(pos[0], pos[1], pos[2], c=color, s=sizes.get(atom, 100),
                   edgecolor='black', alpha=0.8)
        
        # Draw force vector
        force = forces[i]
        fx, fy, fz = force[1], force[2], force[3]
        scale = 0.1  # Scale factor for visualization
        ax2.quiver(pos[0], pos[1], pos[2], 
                  fx*scale, fy*scale, fz*scale,
                  color='blue', arrow_length_ratio=0.1, linewidth=2)
        
        ax2.text(pos[0], pos[1], pos[2] + 0.2, f'{atom}{i+1}', 
                fontsize=10, ha='center')
    
    ax2.set_xlabel('X (Bohr)')
    ax2.set_ylabel('Y (Bohr)')
    ax2.set_zlabel('Z (Bohr)')
    ax2.set_title('Forces on Atoms')
    
    # Force magnitudes bar chart
    ax3 = fig.add_subplot(223)
    atom_labels = [f"{force[0]}{i+1}" for i, force in enumerate(forces)]
    force_mags = [force[4] for force in forces]
    
    bars = ax3.bar(atom_labels, force_mags, color=['red' if 'O' in label else 'lightblue' for label in atom_labels])
    ax3.set_ylabel('Force Magnitude (Hartree/Bohr)')
    ax3.set_title('Force Magnitudes')
    ax3.grid(True, alpha=0.3)
    
    # Add value labels on bars
    for bar, mag in zip(bars, force_mags):
        height = bar.get_height()
        ax3.text(bar.get_x() + bar.get_width()/2., height + 0.1,
                f'{mag:.3f}', ha='center', va='bottom')
    
    # Energy information
    ax4 = fig.add_subplot(224)
    ax4.axis('off')
    
    energy_hartree, energy_ev, _ = read_dft_results(os.path.join(output_dir, 'dft_results.dat'))
    
    energy_text = f"""
DFT Calculation Results

Energy:
  {energy_hartree:.3f} Hartree
  {energy_ev:.2f} eV
  {energy_hartree * 627.509:.1f} kcal/mol

Method: B3LYP/6-31G*
Molecule: Water (H₂O)

Force Analysis:
  Max Force: {max(force_mags):.3f} Hartree/Bohr
  Min Force: {min(force_mags):.3f} Hartree/Bohr
    """
    
    ax4.text(0.1, 0.9, energy_text, transform=ax4.transAxes, 
            fontsize=11, verticalalignment='top', fontfamily='monospace')
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'dft_visualization.png'), dpi=300, bbox_inches='tight')
    print(f"✅ Saved visualization: {output_dir}/dft_visualization.png")
    
    return fig

def main():
    output_dir = 'output'
    
    if not os.path.exists(output_dir):
        print("❌ Output directory not found. Run the DFT example first.")
        return
    
    try:
        # Read structure and results
        atoms, positions = read_xyz_file(os.path.join(output_dir, 'water_structure.xyz'))
        energy_hartree, energy_ev, forces = read_dft_results(os.path.join(output_dir, 'dft_results.dat'))
        
        print("📊 Creating DFT visualization...")
        print(f"Molecule: {len(atoms)} atoms")
        print(f"Energy: {energy_hartree:.3f} Hartree ({energy_ev:.2f} eV)")
        
        # Create visualization
        fig = plot_molecular_structure(atoms, positions, forces, output_dir)
        
        print("🎨 Visualization complete!")
        print("Files generated:")
        print(f"  • {output_dir}/dft_visualization.png")
        
        # Show plot if in interactive mode
        import sys
        if hasattr(sys, 'ps1'):  # Interactive mode
            plt.show()
            
    except Exception as e:
        print(f"❌ Error creating visualization: {e}")
        print("Make sure you have matplotlib installed: pip install matplotlib")

if __name__ == "__main__":
    main()