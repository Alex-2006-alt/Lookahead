/*
 * What: Pawn structure evaluation for doubled, isolated, passed, and connected pawns.
 * Why:  Rewards sound pawn skeletons and penalized structural weaknesses across phases.
 * Layer: Layer 2 (eval) - depends only on core types, bitboard, position, and Score.
 */

#ifndef LOOKAHEAD_EVAL_PAWN_STRUCTURE_H
#define LOOKAHEAD_EVAL_PAWN_STRUCTURE_H

#include "core/types.h"
#include "core/position.h"
#include "material_psqt.h"

namespace lookahead::eval {

// Penalties and bonuses for pawn structure features (S(mg, eg) in centipawns).
inline constexpr Score DOUBLED_PAWN_PENALTY   = Score(-12, -20);
inline constexpr Score ISOLATED_PAWN_PENALTY  = Score(-14, -18);
inline constexpr Score PAWN_SUPPORT_BONUS    = Score(10, 8);
inline constexpr Score PHALANX_BONUS         = Score(8, 6);

// Rank-based passed pawn bonuses (index 0..7 = rank 1..8).
inline constexpr std::array<Score, 8> PASSED_PAWN_BONUS = {
    Score(0, 0),     // Rank 1 (unused)
    Score(0, 0),     // Rank 2
    Score(8, 12),    // Rank 3
    Score(18, 25),   // Rank 4
    Score(36, 50),   // Rank 5
    Score(70, 95),   // Rank 6
    Score(125, 160), // Rank 7
    Score(0, 0)      // Rank 8 (promoted)
};

// Evaluates pawn structure for both sides and returns the score from White's perspective.
[[nodiscard]] Score evaluate_pawn_structure(const core::Position& pos) noexcept;

}  // namespace lookahead::eval

#endif  // LOOKAHEAD_EVAL_PAWN_STRUCTURE_H
