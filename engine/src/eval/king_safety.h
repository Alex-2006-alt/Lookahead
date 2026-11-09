/*
 * What: King safety evaluation covering pawn shelter, open files, and enemy attack pressure.
 * Why:  Penalizes exposed kings and rewards solid castled pawn structures in the middlegame.
 * Layer: Layer 2 (eval) - depends only on core types, position, movegen attacks, and Score.
 */

#ifndef LOOKAHEAD_EVAL_KING_SAFETY_H
#define LOOKAHEAD_EVAL_KING_SAFETY_H

#include "core/types.h"
#include "core/position.h"
#include "material_psqt.h"

namespace lookahead::eval {

// Evaluates king safety terms for both sides and returns the score from White's perspective.
[[nodiscard]] Score evaluate_king_safety(const core::Position& pos) noexcept;

}  // namespace lookahead::eval

#endif  // LOOKAHEAD_EVAL_KING_SAFETY_H
