# 01 - Board Representation & Bitboard Math

## Bitboard Fundamentals

A bitboard is a 64-bit unsigned integer (`uint64_t`) where each bit corresponds directly to a single square on the 8x8 chessboard. Lookahead uses Little-Endian Rank-File (LERF) mapping:

```
Rank 8 | 56 57 58 59 60 61 62 63
Rank 7 | 48 49 50 51 52 53 54 55
Rank 6 | 40 41 42 43 44 45 46 47
Rank 5 | 32 33 34 35 36 37 38 39
Rank 4 | 24 25 26 27 28 29 30 31
Rank 3 | 16 17 18 19 20 21 22 23
Rank 2 |  8  9 10 11 12 13 14 15
Rank 1 |  0  1  2  3  4  5  6  7
       -------------------------
         a  b  c  d  e  f  g  h
```

Square index: `sq = rank * 8 + file` (where file: a=0..h=7, rank: 1=0..8=7).

---

## C++20 Hardware Bit Operations

Modern C++20 introduces the `<bit>` header, allowing direct mapping to CPU instructions:

* `std::countr_zero(bb)`: Finds the least significant bit (LSB) in $O(1)$ time (compiles to `tzcnt` or `bsf` on x86, `clz` on ARM).
* `std::popcount(bb)`: Counts the number of active bits (compiles to `popcnt`).
* Bit clearing: `bb &= bb - 1` clears the lowest active bit efficiently.

---

## Move Representation (16-bit)

A move is represented as a compact 16-bit integer (`uint16_t`):

```
Bits 0..5:   From square (0..63)
Bits 6..11:  To square (0..63)
Bits 12..13: Promotion piece type (0: Knight, 1: Bishop, 2: Rook, 3: Queen)
Bits 14..15: Move flags (0: Normal/Quiet, 1: Capture, 2: En Passant, 3: Castle/Promotion)
```

This 16-bit encoding allows 4 moves to fit into a single 64-bit CPU register or cache line, drastically speeding up move picking and transposition table storage.

---

## Zobrist Hashing

A 64-bit Zobrist key uniquely identifies a board position to detect 3-fold repetitions and index transposition table buckets.

Components:
* `piece_square[12][64]`: Random numbers for each piece type on each square.
* `castling[16]`: 16 possible states of castling rights.
* `en_passant[8]`: En passant target file (if valid).
* `side_to_move`: Toggled every turn.

Incremental updating:
When a piece moves from $s_1$ to $s_2$, the key is updated simply with XOR:
$$\text{key} \leftarrow \text{key} \oplus \text{piece\_square}[P][s_1] \oplus \text{piece\_square}[P][s_2]$$
No full recalculation is needed during move execution.
