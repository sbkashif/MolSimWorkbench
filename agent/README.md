# MLIP training agent

An LLM chooses the next step of an MLIP training loop. Tools do the work, and the
harness enforces the limits and writes every step to a log. The website section
"Agentic AI in a molecular simulation workflow" explains it.

| File | What it holds |
| --- | --- |
| `config.py` | the goal's thresholds and every limit the harness enforces |
| `data.py` | turns rMD17 ethanol into the files MACE reads |
| `tools.py` | `train`, `evaluate`, `run_md`, `add_data`, and the tool list the LLM sees |
| `harness.py` | the loop: send the history, check the call, run the tool, log it |

## Run

Needs the `mlip-agent` conda env (`mamba env create -f agent/environment.yml`: MACE, OpenMM, OpenMM-ML, Ollama) and the rMD17
data (`RMD17_DIR`, default `agent/data/rmd17`, a folder holding `npz_data/` and
`splits/`). Start Ollama, then from the repo root:

```bash
python -m agent.harness --model qwen3:8b --run-dir agent/runs/mine
```

A model name starting with `claude-` uses Anthropic's API instead, and needs
`ANTHROPIC_API_KEY` set. Tested on macOS (Apple M4, CPU only); Linux and WSL
are not tested yet.

Each run writes `log.jsonl` (one line per LLM reply and tool call, with the
LLM's reasoning) and the trained models into its run folder. Run folders are not
committed.
