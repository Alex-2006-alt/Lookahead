# Layer 0: Core (`engine/src/core`)

The foundation of the engine. Layer 0 has zero dependencies on any higher layer.

## Files
- `types.h`: Color, Piece, Square, and Value definitions.
- `bitboard.h`: 64-bit integer bitboard operations using C++20 `<bit>`.
- `move.h`: Compact 16-bit move encoding.
- `version.h`: Central semantic engine version.
- `zobrist.h` / `zobrist.cpp`: Incremental 64-bit position hashing with SplitMix64 PRNG.
- `position.h` / `position.cpp`: Full board state, FEN parser/serializer, and make/unmake.
