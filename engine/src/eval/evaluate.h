/*
 * What: Primary evaluation interface, tapered scoring aggregator, and UI breakdown.
 * Why:  Orchestrates evaluation terms with smooth game phase tapering from side-to-move view.
 * Layer: Layer 2 (eval) - depends only on core, movegen, and eval components.
 */

#ifndef LOOKAHEAD_EVAL_EVALUATE_H
#define LOOKAHEAD_EVAL_EVALUATE_H

#include <cstdint>
#include "core/types.h"
#include "core/position.h"
#include "material_psqt.h"

namespace lookahead::eval {

// Granular evaluation term breakdown surfaced to the web UI and diagnostic tools.
struct EvalBreakdown {
    Score material;
    Score psqt;
    Score pawns;
    Score king_safety;
    Score mobility;
    int game_phase = 0;
    int tapered_total = 0; // Centipawn evaluation from perspective of side to move
};

// Computes the static evaluation of the position in centipawns from side-to-move perspective.
[[nodiscard]] int evaluate(const core::Position& pos) noexcept;

// Computes evaluation and fills detailed term breakdown for visualization.
[[nodiscard]] int evaluate_detailed(const core::Position& pos, EvalBreakdown& breakdown) noexcept;

// Tapers paired MG and EG scores smoothly across the 24-point non-pawn material phase.
[[nodiscard]] constexpr int taper(Score score, int phase) noexcept {
    return (score.mg * phase + score.eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
}

}  // namespace lookahead::eval

#endif  // LOOKAHEAD_EVAL_EVALUATE_H
