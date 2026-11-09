/*
 * What: Piece mobility evaluation measuring safe destination squares.
 * Why:  Rewards active, unrestricted minor and major pieces while penalizing trapped pieces.
 * Layer: Layer 2 (eval) - depends only on core types, position, movegen attacks, and Score.
 */

#ifndef LOOKAHEAD_EVAL_MOBILITY_H
#define LOOKAHEAD_EVAL_MOBILITY_H

#include "core/types.h"
#include "core/position.h"
#include "material_psqt.h"

namespace lookahead::eval {

// Mobility bonus weights per safe square S(mg, eg) in centipawns.
inline constexpr Score KNIGHT_MOBILITY_WEIGHT = Score(4, 4);
inline constexpr Score BISHOP_MOBILITY_WEIGHT = Score(5, 5);
inline constexpr Score ROOK_MOBILITY_WEIGHT   = Score(2, 4);
inline constexpr Score QUEEN_MOBILITY_WEIGHT  = Score(1, 2);

// Evaluates mobility for both sides and returns the score from White's perspective.
[[nodiscard]] Score evaluate_mobility(const core::Position& pos) noexcept;

}  // namespace lookahead::eval

#endif  // LOOKAHEAD_EVAL_MOBILITY_H
