# 02 - Move Generation & Perft Verification

## Move Generation Architecture

Move generation in Lookahead proceeds in two steps:
1. **Attack Lookups**: Calculating reachable destination squares based on precomputed bitboards and ray attacks.
2. **Move Synthesis**: Generating pseudo-legal moves and filtering out moves that leave the king in check.

---

## Magic Bitboards for Sliders

Sliding pieces (rooks and bishops) have attacks blocked by other pieces. Lookahead uses **Magic Bitboards**:
* For each square, an attack mask defines relevant occupancy squares.
* The occupancy subset is multiplied by a 64-bit "magic number" and shifted right by $(64 - \text{bits})$ to compute a hash table index:
$$\text{index} = ((\text{occupancy} \ \& \ \text{mask}) \times \text{magic}) \gg (64 - \text{bits})$$
* This maps any board occupancy to the exact attack set in a precomputed lookup array in $O(1)$ time.

---

## Legal Move Filtering

A move is legal if and only if the moving side's king is not in check after the move is completed:
1. Identify in-check status and pinned pieces before move generation.
2. Avoid expensive make/unmake operations for moves that can be proven illegal via pin ray analysis.
3. Handle special cases: en passant discovered checks, castling through check, and promotion.

---

## Perft (Performance Test)

Perft recursively counts all leaf nodes at a given depth. Because chess rules create complex combinatorics, perft is the gold standard for verifying engine correctness.

Standard Starting Position Perft:
* Depth 1: 20
* Depth 2: 400
* Depth 3: 8,902
* Depth 4: 197,281
* Depth 5: 4,865,609
* Depth 6: 119,060,324

Every perft test in Lookahead is verified against `python-chess` as an external oracle to ensure zero bugs in move generation.
