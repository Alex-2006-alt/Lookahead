# Developer Tools & Test Harnesses (`tools/`)

All scripts in this directory are written in Python 3.12 and use `python-chess` exclusively as an external testing oracle.

## Tools
- `check_layers.py`: Static analysis script enforcing one-way layer dependencies, line limits (<=300 lines), and header comment blocks. Runs in CI on every push and PR.
- `check_perft.py`: Validates engine perft output against `python-chess` across `data/perft_positions.epd`.
- `compare_movegen.py`: Differential fuzzer comparing legal moves across 10,000+ random positions.
- `run_matches.py`: Automated tournament runner for playing engine matches.
- `elo.py`: Calculates Elo difference and 95% confidence intervals from match results.
- `run_ablation.py`: Executes automated ablation runs measuring the Elo impact of individual search and evaluation heuristics.
