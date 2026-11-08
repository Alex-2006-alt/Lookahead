# 02 - Move Generation & Perft Verification

Lookahead employs a strictly legal move generator optimized for high throughput, zero dynamic allocations, and verifiable correctness.

---

## 1. Architecture & Layer Boundaries

The move generation layer (`engine/src/movegen/`) sits directly above `core/` and has zero dependencies on evaluation, search, or interfaces:

* [`attacks.h`](file:///engine/src/movegen/attacks.h) / [`attacks.cpp`](file:///engine/src/movegen/attacks.cpp): Precomputed leaping tables (Pawns, Knights, Kings) and 64 Bishop / 64 Rook Collision-Free Magic Bitboards.
* [`movegen.h`](file:///engine/src/movegen/movegen.h) / [`movegen.cpp`](file:///engine/src/movegen/movegen.cpp): Strictly legal move generator with pin ray masking, check evasions, and game-end rule evaluators.
* [`perft.h`](file:///engine/src/movegen/perft.h) / [`perft.cpp`](file:///engine/src/movegen/perft.cpp): Recursive perft counter, divide analyzer, and throughput benchmark harness.

---

## 2. Magic Bitboards for Sliding Pieces

Sliding pieces (bishops, rooks, queens) generate ray attacks blocked by arbitrary board occupancies. Lookahead resolves these in $O(1)$ time without runtime branches via Fancy Magic Bitboards:

$$\text{index} = ((\text{occupancy} \ \& \ \text{mask}) \times \text{magic}) \gg (64 - \text{bits})$$

### Memory Footprint & Determinism
* **Bishops**: 64 squares $\times$ variable table sizes (5 to 9 bits = 32 to 512 entries) $\approx$ 5,248 `Bitboard` entries ($\approx$ 41 KB).
* **Rooks**: 64 squares $\times$ variable table sizes (10 to 12 bits = 1024 to 4096 entries) $\approx$ 102,400 `Bitboard` entries ($\approx$ 800 KB).
* **Total Attack Table Footprint**: $\approx$ 841 KB.
* **Deterministic Initialization**: Magic numbers are embedded directly as fixed 64-bit constants verified collision-free, ensuring instantaneous startup and bit-identical behavior across native and WebAssembly builds.

---

## 3. Strictly Legal Move Generation

Unlike engines that generate pseudo-legal moves and test legality by making and unmaking each move, Lookahead generates **strictly legal moves directly**:

1. **Pre-calculation of Checkers & Pins**:
   - `checkers = attackers_to(king_sq, ~us)`
   - Pinned pieces along ray lines to our King are identified via `attacks::ray_line` and ray intersections.
2. **Double Check Handling**:
   - When `popcount(checkers) >= 2`, non-king moves are completely skipped; only legal King evasions are generated.
3. **Single Check Evasions**:
   - The target mask is restricted to `checkers | ray_between(king_sq, checker_sq)` (blocking or capturing the checker).
4. **Pin Ray Constraints**:
   - Pinned pieces can only move along the line connecting their square and our King (`attacks::line(from, king_sq)`).
5. **Special Edge Cases**:
   - **En Passant Discovered Check**: If King and capturing pawn share a rank with an enemy slider, removing both pawns could expose a check along the rank. Lookahead explicitly verifies king safety for en passant captures.
   - **Castling Transit**: Both transit squares and destination squares must be unattacked and empty.

### Zero-Allocation MoveList
Moves are written directly into a stack-allocated buffer:
```cpp
class MoveList {
    std::array<Move, 256> moves_{};
    size_t size_ = 0;
    ...
};
```
No dynamic memory allocations (`new` or `std::vector::push_back`) occur in the search or move generation path.

---

## 4. Perft & Divide Verification

Perft (Performance Test) counts all leaf nodes at depth $D$:
$$\text{perft}(D) = \sum_{m \in \text{legal\_moves}} \text{perft}(D - 1)$$

When a mismatch occurs, `perft_divide(pos, D)` breaks down node counts by each legal root move:
```
e2e4: 20
d2d4: 20
g1f3: 20
...
Total nodes: 400
```

---

## 5. Differential Testing with python-chess

Lookahead's move generator is verified using two automated tools:

1. **`tools/check_perft.py`**:
   - Executes standard perft positions from [`data/perft_positions.epd`](file:///data/perft_positions.epd) (including Kiwipete depth 4 = 4,085,603 nodes).
   - Validates node count equality and measures engine NPS.
2. **`tools/compare_movegen.py`**:
   - Plays random games and samples 10,000 distinct positions.
   - Asserts move-for-move identity between Lookahead and `python-chess` legal move sets.
