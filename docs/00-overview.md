# 00 - Architecture & System Design Overview

## Abstract
Lookahead is an original chess engine built in modern C++20, targeted for both native command-line UCI execution and in-browser interactive execution via WebAssembly. It prioritizes clarity, rigorous testing, and algorithmic transparency over black-box complexity.

---

## Architectural Layers

The engine strictly adheres to a unidirectional dependency hierarchy:

```
[interface]
   │
[search]
   │
[eval]
   │
[movegen]
   │
[core]
```

### 1. `core/` (Layer 0)
* Pure representations: `Bitboard`, `Square`, `Piece`, `Color`, `Move`.
* Board state storage: bitboards for each piece and color, castling rights, en passant target, halfmove clock, fullmove number.
* 64-bit Zobrist hashing with incremental make/unmake XOR updates.
* FEN parsing and serialization with strict input validation.

### 2. `movegen/` (Layer 1)
* Attack tables: non-sliding attacks (knights, kings, pawns) computed at startup.
* Sliding attacks: Magic bitboards for rooks and bishops.
* Pseudo-legal and strictly legal move generation.
* Perft (performance test) tree generation and divide debugging.

### 3. `eval/` (Layer 2)
* Static evaluation function returning an evaluation in centipawns from the perspective of the side to move.
* Tapered evaluation linearly interpolating between middlegame (MG) and endgame (EG) phases.
* Individual components: material, piece-square tables (PSQT), pawn structure, king safety, and piece mobility.

### 4. `search/` (Layer 3)
* Alpha-beta minimax search using the negamax framework.
* Iterative deepening with dynamic time management.
* Move ordering: transposition table move, MVV-LVA (Most Valuable Victim - Least Valuable Attacker), killer moves, and history heuristic.
* Quiescence search to mitigate the horizon effect.
* Selectivity techniques: null-move pruning, late move reductions (LMR), and check extensions.
* Search tree instrumentation for browser visualization.

### 5. `interface/` (Layer 4)
* Universal Chess Interface (UCI) protocol compliance for standard chess GUIs.
* WebAssembly C-API bridging the engine to modern browser environments.

---

## Systems Engineering Constraints

* **Strict One-Way Includes**: No file in a lower layer may include a header from a higher layer.
* **Bounded File Length**: No file exceeds 300 lines of code.
* **No Dynamic Allocations in Hot Paths**: Transposition tables and search stacks are allocated once upon engine initialization.
* **Determinism & Reproducibility**: All pseudo-random numbers (e.g. Zobrist seeds) use fixed generators (SplitMix64) to ensure identical behavior across machines.
