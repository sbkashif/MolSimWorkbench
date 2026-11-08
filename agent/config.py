"""Settings for the MLIP training agent. Everything the harness enforces lives here."""
import os
import sys
from pathlib import Path

# Where the public data sits. rMD17 from https://doi.org/10.6084/m9.figshare.12672038
RMD17_DIR = Path(os.environ.get("RMD17_DIR", Path(__file__).parent / "data" / "rmd17"))
MOLECULE = "ethanol"
SPLIT = "01"                 # rMD17 ships five official train/test splits

# The goal the agent is given. Provisional: chosen for this first run, not tuned.
TARGET_FORCE_MAE = 20.0      # meV/Å on the selection frames
MD_TEMPERATURE = 500.0       # K, start temperature of the MD check
MD_STEPS = 2000              # 0.5 fs each, so 1 ps
MD_DRIFT_LIMIT = 0.01        # eV, total-energy range after t=0
MIN_PAIR_LIMIT = 0.75        # Å, closest two atoms may come

# Limits the harness enforces, whatever the LLM asks for.
START_FRAMES = 200
MAX_FRAMES = 1000            # the rMD17 readme says never train on more than 1000
MAX_TRAININGS = 5
MAX_TOOL_CALLS = 20
EPOCH_RANGE = (10, 150)
CHANNEL_CHOICES = (16, 32, 64)
# The official test split (1000 frames) is cut in two. The agent decides on the
# first half; the harness scores the final model once on the second half, which
# the agent never sees.
SELECTION_FRAMES = 500       # official test frames 0-499: evaluate() uses these
FINAL_TEST_FRAMES = 500      # official test frames 500-999: scored once, after stop

# Where mace_run_train lives: next to the Python running the agent.
PYTHON_BIN = Path(os.environ.get("MLIP_PYTHON_BIN", Path(sys.executable).parent))
