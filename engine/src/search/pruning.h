/*
 * What: Heuristic branch pruning and reduction algorithms (RFP, NMP, LMR).
 * Why:  Drastically cuts search trees while preserving tactical and strategic accuracy.
 * Layer: Layer 3 (search) - depends on core and movegen.
 */

#ifndef LOOKAHEAD_SEARCH_PRUNING_H
#define LOOKAHEAD_SEARCH_PRUNING_H

#include <cstdint>
#include <cmath>
#include <algorithm>
#include "core/types.h"
#include "core/position.h"
#include "core/move.h"

namespace lookahead::search {

// Checks whether side to move has non-pawn, non-king pieces (guards against zugzwang).
[[nodiscard]] bool has_non_pawn_material(const core::Position& pos, core::Color c) noexcept;

// Reverse Futility Pruning (Static Null Move) margin.
[[nodiscard]] constexpr int rfp_margin(int depth) noexcept {
    return 120 * depth;
}

// Determines if static evaluation allows an early beta cutoff.
[[nodiscard]] inline bool can_reverse_futility_prune(int depth, int static_eval, int beta) noexcept {
    return (depth <= 3) && (static_eval - rfp_margin(depth) >= beta);
}

// Computes null-move search depth reduction factor.
[[nodiscard]] constexpr int null_move_reduction(int depth) noexcept {
    return 2 + (depth / 6);
}

// Computes Late Move Reduction (LMR) depth reduction for quiet moves.
[[nodiscard]] int late_move_reduction(int depth, int moves_searched) noexcept;

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_PRUNING_H
