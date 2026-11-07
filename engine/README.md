# Lookahead Engine Architecture

The Lookahead chess engine is written in ISO C++20 and compiled into both a native UCI executable and a WebAssembly binary.

---

## Start Reading Here: Layer Roadmap

To understand the codebase, explore the layers in order from bottom to top:

1. [`src/core/`](src/core/README.md):
   * `types.h`: Basic enums (Color, Piece, Square).
   * `bitboard.h`: 64-bit bitboard operations and `<bit>` primitives.
   * `move.h`: 16-bit compact move encoding and unpackers.
   * `version.h`: Central version definition (`0.1.0`).
   * `zobrist.h` / `zobrist.cpp`: 64-bit incremental position hashing.
   * `position.h` / `position.cpp`: Full board state, FEN parser/serializer, and make/unmake.

2. [`src/movegen/`](src/movegen/README.md):
   * `attacks.h` / `attacks.cpp`: Precalculated leaping attacks and magic bitboards for sliding pieces.
   * `movegen.h` / `movegen.cpp`: Legal move generator (evasions, pins, castling, promotions).
   * `perft.h` / `perft.cpp`: Move generation validation and node counting.

3. [`src/eval/`](src/eval/README.md):
   * `evaluate.h` / `evaluate.cpp`: Phase calculation and tapered evaluation driver.
   * `material_psqt.h` / `material_psqt.cpp`: Piece values and piece-square tables.
   * `pawn_structure.cpp`: Doubled, isolated, and passed pawn analysis.
   * `king_safety.cpp`: Pawn shield evaluation and king vulnerability penalties.
   * `mobility.cpp`: Piece attack and safe square counts.

4. [`src/search/`](src/search/README.md):
   * `features.h`: Central toggles for all search and evaluation heuristics.
   * `search.h` / `search.cpp`: Negamax alpha-beta search and iterative deepening.
   * `quiescence.cpp`: Tactical quiescence search.
   * `move_ordering.h` / `move_ordering.cpp`: MVV-LVA, killers, and history heuristic.
   * `transposition_table.h` / `transposition_table.cpp`: Fixed-size hash table.
   * `pruning.cpp`: Null-move pruning and late move reductions.
   * `time_manager.h` / `time_manager.cpp`: Dynamic clock allocation.
   * `trace.h` / `trace.cpp`: Search tree instrumentation for browser visualization.

5. [`src/interface/`](src/interface/README.md):
   * `uci.h` / `uci.cpp`: Universal Chess Interface for standard GUIs.
   * `wasm_api.cpp`: Emscripten C-API export for browser Web Workers.

---

## Compilation

```bash
# Build native engine
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release engine
cmake --build build --config Release
```
