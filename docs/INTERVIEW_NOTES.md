# Technical Interview Notes: Chess Engine Engineering

Concise, rigorous answers to 15 key technical interview questions based on Lookahead's architecture and implementation.

---

### 1. Why use Bitboards instead of an 8x8 array or mailbox representation?
**Answer**: Bitboards represent piece placement as 64-bit integers (`uint64_t`). This maps the 64 squares of a chessboard directly to CPU registers. Complex board queries (e.g. "which black pieces attack white's king", "find all pinned pieces") become single bitwise operations (`AND`, `OR`, `XOR`, shifts). Furthermore, C++20 `<bit>` instructions like `std::countr_zero` (trailing zero count) allow scanning active pieces in $O(1)$ without looping over 64 squares.

---

### 2. What are Magic Bitboards and why are they necessary for sliding pieces?
**Answer**: Sliding pieces (rooks and bishops) have dynamic ray attacks that are blocked by intervening pieces. To compute attacks in $O(1)$, Magic Bitboards isolate the blocker configuration via an occupancy mask, multiply it by a precomputed 64-bit constant ("magic number"), and shift the result right to generate a dense, collision-free index into an attack lookup table.

---

### 3. Why choose Magic Bitboards over BMI2 PEXT instructions for Lookahead?
**Answer**: While `_pext_u64` is faster on modern Intel x86 CPUs, it has significant performance penalties on AMD Zen 1/2, does not exist on ARM CPUs (like Apple Silicon), and is completely unavailable in WebAssembly. Magic Bitboards use standard 64-bit multiplication and bit shifts, guaranteeing identical performance and total cross-platform determinism between the native build and the browser WASM engine.

---

### 4. How does Lookahead represent a chess move in just 16 bits?
**Answer**: Lookahead uses a `uint16_t`:
* Bits 0..5: From square (0..63, 6 bits)
* Bits 6..11: To square (0..63, 6 bits)
* Bits 12..13: Promotion piece type (Knight, Bishop, Rook, Queen, 2 bits)
* Bits 14..15: Move type flags (Quiet, Capture, En Passant, Castle/Promo, 2 bits)
Fitting moves into 16 bits minimizes cache footprint, speeds up copying in move buffers, and allows 4 moves per 64-bit word.

---

### 5. Why is incremental Zobrist hashing preferred over hash recomputation?
**Answer**: Recomputing a 64-bit hash from scratch requires iterating over all 64 squares, taking dozens of CPU cycles per node. Zobrist hashing uses the XOR involution property ($X \oplus Y \oplus Y = X$). Making or unmaking a move requires only 2 to 4 XOR operations with precomputed 64-bit random tables, keeping hashing overhead near zero during tree search.

---

### 6. What is the Horizon Effect and how does Quiescence Search resolve it?
**Answer**: The horizon effect occurs when a fixed-depth search stops right in the middle of a tactical exchange (e.g., evaluating a position immediately after a Queen captures a defended Pawn, concluding that the capturing side is +9, unaware that the Queen is captured on the very next ply). Quiescence search continues searching captures and promotions until the board is "quiet", ensuring static evaluation is only evaluated on stable positions.

---

### 7. How does Alpha-Beta pruning reduce search complexity from Minimax?
**Answer**: Minimax visits $O(b^d)$ nodes (where $b$ is branching factor $\approx 35$, and $d$ is depth). Alpha-beta maintains an interval $[\alpha, \beta]$ representing the window of plausible scores. If a child move produces a score $\ge \beta$, the opposing player will never permit this branch; hence, the remaining moves are skipped (beta cutoff). With perfect move ordering, alpha-beta reduces complexity to $O(b^{d/2}) = O(\sqrt{b^d})$, effectively doubling search depth for the same computational budget.

---

### 8. Why is Move Ordering considered the most important heuristic in game tree search?
**Answer**: Alpha-beta pruning achieves its optimal $O(b^{d/2})$ efficiency only if the best move is explored first. If bad moves are examined first, no pruning occurs and the search degrades toward $O(b^d)$. Lookahead prioritizes: (1) Transposition Table move, (2) MVV-LVA winning captures, (3) Killer moves from sibling nodes, and (4) History heuristic scores.

---

### 9. What is Null-Move Pruning and what is its primary pitfall?
**Answer**: Null-Move Pruning tests if the current position is so advantageous that even if the side to move "passes" (makes no move) and gives the opponent two turns in a row, the engine still exceeds $\beta$ in a reduced-depth search. If so, deep search is skipped. The pitfall is **Zugzwang** (common in endgames), where having to move is a disadvantage; passing would mask the fact that any legal move destroys the position. Lookahead disables NMP when material is low (e.g. only pawns and kings remain).

---

### 10. How are Checkmate scores handled in the Transposition Table?
**Answer**: Mate scores cannot be stored as raw values because a mate-in-2 at ply 4 is different from a mate-in-2 at ply 10. Lookahead stores mate scores relative to the current position:
* Storing: $\text{score} + \text{ply}$
* Retrieving: $\text{score} - \text{ply}$
This guarantees that checkmate path lengths remain consistent when retrieved across different depths and plies.

---

### 11. What is Late Move Reduction (LMR) and how does it work?
**Answer**: Because move ordering places high-probability moves first, moves examined late in the legal move list (e.g. after move 4 or 6) are statistically unlikely to be optimal. LMR searches these moves with reduced depth ($d - 1$ or $d - 2$). If a reduced move unexpectedly beats $\alpha$, the engine re-searches it at full depth to verify the score.

---

### 12. How does Lookahead guarantee zero dynamic memory allocation in the search hot path?
**Answer**: Dynamic heap allocation (`new`, `malloc`, `std::vector::push_back` reallocation) induces cache misses, heap lock contention, and unpredictable latency. Lookahead uses fixed-capacity stack arrays for move generation buffers (`MoveList` of size 256), a preallocated static transposition table, and passes lightweight `UndoInfo` structs down the call stack.

---

### 13. How is the engine ported to WebAssembly without freezing the browser UI?
**Answer**: Heavy C++ computations on the browser's main JavaScript thread freeze the DOM and cause frame drops. Lookahead compiles the C++ code to WebAssembly using Emscripten and runs the WASM instance inside a dedicated **Web Worker**. The React UI communicates with the worker via non-blocking asynchronous `postMessage` channels.

---

### 14. What are Tapered Evaluations and why are they essential?
**Answer**: Piece values and positional priorities shift dynamically as pieces are traded off. For instance, king safety matters immensely with queens on the board, but king centralization is paramount in endgames. Lookahead calculates both a Middlegame score ($S_{MG}$) and an Endgame score ($S_{EG}$), then smoothly interpolates between them based on total remaining non-pawn material (Game Phase).

---

### 15. How does Lookahead ensure architectural boundaries are not violated during development?
**Answer**: Lookahead enforces a strict one-way dependency model (`core <- movegen <- eval <- search <- interface`). The repository includes `tools/check_layers.py`, an automated static analysis script that runs in CI on every push and pull request. It parses `#include` statements and fails if any lower layer references a higher layer, while also enforcing a 300-line limit per file.
