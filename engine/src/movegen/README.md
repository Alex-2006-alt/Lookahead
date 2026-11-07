# Layer 1: Move Generation (`engine/src/movegen`)

Generates pseudo-legal and legal moves. Can only include headers from `core/`.

## Files
- `attacks.h` / `attacks.cpp`: Precalculated leaping attacks and magic bitboards for sliding pieces.
- `movegen.h` / `movegen.cpp`: Legal move generator (captures, quiet moves, evasions, castling, promotions).
- `perft.h` / `perft.cpp`: Move generation validation and node counting.
