# 07 - Testing Methodology & Strength Measurement

## Rigorous Verification Hierarchy

1. **Perft Oracle Tests**: Every tricky position in `data/perft_positions.epd` is validated against `python-chess`.
2. **Differential Fuzzing (`tools/compare_movegen.py`)**: 10,000+ random board positions generated via random walks are compared move-for-move against `python-chess`'s move generator.
3. **Fuzzing Make/Unmake**: After executing random moves, unmaking them must return the board state, bitboards, and Zobrist hash to the exact initial state.
4. **Sanitizer Enforcement**: All tests pass under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
5. **Layer Verification**: `tools/check_layers.py` ensures 0 architecture layer violations.

---

## Strength & Rating Measurement

Lookahead's playing strength is evaluated using automated match runners against calibrated opponents (such as Stockfish with `UCI_LimitStrength` or previous engine revisions):
* Opening book: balanced openings from `data/openings.epd`.
* Minimum 200 games per matchup with alternated colors to control for first-move advantage.
* Elo calculations reported with a 95% confidence interval ($\pm E$).
