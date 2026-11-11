"""Settings for the MLIP training agent. Everything the harness enforces lives here."""
import os
import sys
from pathlib import Path

# Where the public data sits. rMD17 from https://doi.org/10.6084/m9.figshare.12672038
RMD17_DIR = Path(os.environ.get("RMD17_DIR", Path(__file__).parent / "data" / "rmd17"))
MOLECULE = "ethanol"
SPLIT = "01"                 # rMD17 ships five official train/test splits

# The goal the agent is given. None of these numbers was tuned: they
# were picked by hand for a first run. Where one has a reason, it is
# given here.
# Force target: about half the hand-trained model's 40.3 meV/Å.
TARGET_FORCE_MAE = 20.0      # meV/Å on the selection frames
# MD check: rMD17's frames come from MD at 500 K; 1 ps runs in under
# a minute; 0.01 eV is about 2% of the kinetic energy; 0.75 Å is well
# under the shortest bond (O-H, 0.97 Å).
MD_TEMPERATURE = 500.0       # K, start temperature
MD_STEPS = 2000              # 0.5 fs each, so 1 ps
MD_DRIFT_LIMIT = 0.01        # eV, total-energy range after t=0
MIN_PAIR_LIMIT = 0.75        # Å, closest two atoms may come

# Limits the harness enforces, whatever the LLM asks for. All picked
# by hand to keep a run to minutes on a laptop, except MAX_FRAMES.
START_FRAMES = 200
MAX_FRAMES = 1000            # rMD17 readme: train on at most 1000
MAX_TRAININGS = 5
MAX_TOOL_CALLS = 20
EPOCH_RANGE = (10, 150)
CHANNEL_CHOICES = (16, 32, 64)
# The rMD17 readme limits training to 1000 frames, not testing. So the
# selection frames, which evaluate() uses and the agent decides on, are
# drawn from frames in neither official split. The official test split
# (1000 frames) is kept whole and scored once, after stop.
SELECTION_FRAMES = 500

# Where mace_run_train lives: next to the Python running the agent.
PYTHON_BIN = Path(os.environ.get("MLIP_PYTHON_BIN", Path(sys.executable).parent))
