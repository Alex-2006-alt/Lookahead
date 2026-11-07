/*
 * What: Deterministic 64-bit Zobrist hashing keys and incremental update functions.
 * Why:  Enables O(1) position transposition lookups and 3-fold repetition detection.
 * Layer: Layer 0 (core) - depends only on types.h and standard C++ headers.
 */

#ifndef LOOKAHEAD_CORE_ZOBRIST_H
#define LOOKAHEAD_CORE_ZOBRIST_H

#include <cstdint>
#include <optional>
#include "types.h"

namespace lookahead::core {

using Key = uint64_t;
inline constexpr Key KeyEmpty = 0ULL;

namespace zobrist {

// Initializes deterministic Zobrist hash tables using a fixed SplitMix64 seed.
void init() noexcept;

// Returns the Zobrist key for a colored piece on a given square.
[[nodiscard]] Key piece(Piece p, Square sq) noexcept;

// Returns the Zobrist key for castling privileges (0..15).
[[nodiscard]] Key castling(uint8_t rights) noexcept;

// Returns the Zobrist key for an active en passant target file (File A..H).
[[nodiscard]] Key en_passant(File f) noexcept;

// Returns the single Zobrist key toggled when Black is to move.
[[nodiscard]] Key side_to_move() noexcept;

}  // namespace zobrist

}  // namespace lookahead::core

#endif  // LOOKAHEAD_CORE_ZOBRIST_H
