# Engine Unit Tests (`engine/tests`)

This directory mirrors the layout of `src/`:
- `core/`: FEN roundtrips, Zobrist hash equivalence, make/unmake fuzz tests.
- `movegen/`: Perft suite against `data/perft_positions.epd`.
- `eval/`: Evaluation symmetry tests and piece-square table sanity checks.
- `search/`: Mate solvers (`data/mate_puzzles.epd`), tactical puzzles (`data/tactics.epd`), and repetition handling.
- `interface/`: UCI parser and command execution tests.
