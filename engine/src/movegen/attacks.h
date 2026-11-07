/*
 * What: Precomputed leaping attacks, Magic Bitboards for sliders, and ray masks.
 * Why:  Provides O(1) piece reachability lookups, check verification, and pin ray masks.
 * Layer: Layer 1 (movegen) - depends only on core types and bitboard.
 */

#ifndef LOOKAHEAD_MOVEGEN_ATTACKS_H
#define LOOKAHEAD_MOVEGEN_ATTACKS_H

#include <cstdint>
#include "core/types.h"
#include "core/bitboard.h"

namespace lookahead::movegen::attacks {

// Initializes precalculated attack tables and magic bitboards for sliding pieces.
void init() noexcept;

// Returns destination squares attacked by a pawn of the given color.
[[nodiscard]] core::Bitboard pawn(core::Color c, core::Square sq) noexcept;

// Returns destination squares reachable by a knight.
[[nodiscard]] core::Bitboard knight(core::Square sq) noexcept;

// Returns destination squares reachable by a king.
[[nodiscard]] core::Bitboard king(core::Square sq) noexcept;

// Returns bishop ray attacks given an occupancy bitboard.
[[nodiscard]] core::Bitboard bishop(core::Square sq, core::Bitboard occupied) noexcept;

// Returns rook ray attacks given an occupancy bitboard.
[[nodiscard]] core::Bitboard rook(core::Square sq, core::Bitboard occupied) noexcept;

// Returns queen ray attacks (combination of bishop and rook rays).
[[nodiscard]] inline core::Bitboard queen(core::Square sq, core::Bitboard occupied) noexcept {
    return bishop(sq, occupied) | rook(sq, occupied);
}

// Returns a bitboard of squares strictly between sq1 and sq2 along a ray (or 0 if not collinear).
[[nodiscard]] core::Bitboard between(core::Square sq1, core::Square sq2) noexcept;

// Returns a bitboard of the entire line/ray passing through collinear squares sq1 and sq2.
[[nodiscard]] core::Bitboard line(core::Square sq1, core::Square sq2) noexcept;

}  // namespace lookahead::movegen::attacks

#endif  // LOOKAHEAD_MOVEGEN_ATTACKS_H
