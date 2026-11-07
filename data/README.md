# Lookahead Test Datasets (`data/`)

All testing positions, tactical benchmarks, and opening suites reside in this directory in Extended Position Description (EPD) format.

## Files
- `perft_positions.epd`: Rigorously validated perft positions (Initial position, Kiwipete, Position 3, 4, 5, etc.) with exact node counts verified via `python-chess`.
- `mate_puzzles.epd`: Mate-in-1, mate-in-2, and mate-in-3 tactical puzzles.
- `tactics.epd`: Tactical benchmark positions testing pins, skewers, forks, and sacrifices.
- `openings.epd`: Balanced opening suite for automated strength evaluation matches.
