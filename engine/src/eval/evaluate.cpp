/*
 * What: Implementation of root evaluation aggregation and phase tapering.
 * Why:  Combines material, piece-square tables, and positional terms into a single score.
 * Layer: Layer 2 (eval) - depends only on evaluate.h and eval components.
 */

#include "evaluate.h"
#include "pawn_structure.h"
#include "king_safety.h"
#include "mobility.h"

namespace lookahead::eval {

int evaluate_detailed(const core::Position& pos, EvalBreakdown& breakdown) noexcept {
    const int phase = calculate_game_phase(pos);
    breakdown.game_phase = phase;

    const auto [mat, psqt] = evaluate_material_and_psqt(pos);
    breakdown.material = mat;
    breakdown.psqt = psqt;

    // Positional terms
    breakdown.pawns = evaluate_pawn_structure(pos);
    breakdown.king_safety = evaluate_king_safety(pos);
    breakdown.mobility = evaluate_mobility(pos);

    // Total score from White's perspective
    const Score white_total = breakdown.material + breakdown.psqt +
                              breakdown.pawns + breakdown.king_safety +
                              breakdown.mobility;

    // Taper between middlegame and endgame based on non-pawn material phase
    const int white_tapered = taper(white_total, phase);

    // Return perspective of side to move: positive means good for side to move
    const int stm_score = (pos.side_to_move() == core::Color::White) ? white_tapered : -white_tapered;
    breakdown.tapered_total = stm_score;
    return stm_score;
}

int evaluate(const core::Position& pos) noexcept {
    EvalBreakdown breakdown;
    return evaluate_detailed(pos, breakdown);
}

}  // namespace lookahead::eval
