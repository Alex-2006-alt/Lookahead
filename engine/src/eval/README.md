# Layer 2: Evaluation (`engine/src/eval`)

Evaluates board positions statically. Can only include headers from `core/` and `movegen/`.

## Files
- `evaluate.h` / `evaluate.cpp`: Game phase calculation and tapered evaluation driver.
- `material_psqt.h` / `material_psqt.cpp`: Piece values and piece-square tables.
- `pawn_structure.cpp`: Doubled, isolated, and passed pawn analysis.
- `king_safety.cpp`: Pawn shield evaluation and king vulnerability.
- `mobility.cpp`: Safe square counts for minor and major pieces.
