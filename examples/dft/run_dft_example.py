#!/usr/bin/env python3
"""MolSimWorkbench DFT example using direct Python bindings."""

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from molsimworkbench import _molsim


def build_water_geometry():
    geometry = _molsim.Geometry()
    geometry.addAtom("O", [0.0, 0.0, 0.0])
    geometry.addAtom("H", [0.757, 0.586, 0.0])
    geometry.addAtom("H", [-0.757, 0.586, 0.0])
    return geometry


def main():
    dft_config = PROJECT_ROOT / "data" / "potentials" / "dft.yaml"

    print("=== MolSimWorkbench DFT Example (Binding-Backed) ===")
    print(f"Using DFT config: {dft_config}")

    if not dft_config.exists():
        print(f"❌ DFT config not found: {dft_config}")
        return 1

    geometry = build_water_geometry()
    dft = _molsim.DFTPotential(str(dft_config))

    energy_hartree = dft.calculatePotentialEnergy(geometry)
    energy_ev = energy_hartree * 27.2114
    energy_kcal = energy_hartree * 627.509
    forces = dft.calculateFlatForces(geometry)

    print("\n✅ DFT calculation completed")
    print(f"Total energy: {energy_hartree:.6f} Hartree")
    print(f"Total energy: {energy_ev:.6f} eV")
    print(f"Total energy: {energy_kcal:.6f} kcal/mol")
    print("Force vector length:", len(forces))

    return 0


if __name__ == "__main__":
    sys.exit(main())
