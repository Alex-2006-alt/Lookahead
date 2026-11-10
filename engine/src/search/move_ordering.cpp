/*
 * What: Implementation of move scoring heuristics, MVV-LVA, and selection sorting.
 * Why:  Prioritizes high-potential moves to trigger early alpha-beta branch cutoffs.
 * Layer: Layer 3 (search) - depends on move_ordering.h, core types, and position.
 */

#include "move_ordering.h"
#include <algorithm>

namespace lookahead::search {

int mvv_lva_score(const core::Position& pos, core::Move m) noexcept {
    const core::Piece captured = pos.piece_at(m.to_square());
    const core::PieceType victim_pt = (captured != core::Piece::None)
                                          ? core::type_of(captured)
                                          : (m.is_en_passant() ? core::PieceType::Pawn : core::PieceType::None);

    const core::Piece moving = pos.piece_at(m.from_square());
    const core::PieceType attacker_pt = core::type_of(moving);

    const int victim_val = VICTIM_SCORES[static_cast<size_t>(victim_pt)];
    const int attacker_val = static_cast<int>(attacker_pt);

    int score = victim_val * 10 - attacker_val;
    if (m.is_promotion()) {
        score += 800;
    }
    return score;
}

int score_move(const core::Position& pos, core::Move m, core::Move tt_move,
               const KillerTable& killers, const HistoryTable& history,
               int ply) noexcept {
    // Priority 1: Transposition table best move
    if (!tt_move.is_null() && m == tt_move) {
        return SCORE_TT_MOVE;
    }

    // Priority 2: Captures ordered by MVV-LVA
    if (m.is_capture()) {
        return SCORE_CAPTURE_BASE + mvv_lva_score(pos, m);
    }

    // Priority 3: Pawn promotions
    if (m.is_promotion()) {
        const int promo_val = static_cast<int>(m.promotion_type()) * 100;
        return SCORE_PROMOTION_BASE + promo_val;
    }

    // Priority 4: Primary killer move
    if (killers.is_killer_1(ply, m)) {
        return SCORE_KILLER_1;
    }

    // Priority 5: Secondary killer move
    if (killers.is_killer_2(ply, m)) {
        return SCORE_KILLER_2;
    }

    // Priority 6: Quiet moves ranked by history heuristic
    return history.get(pos.side_to_move(), m.from_square(), m.to_square());
}

void score_moves(const core::Position& pos, const movegen::MoveList& moves,
                 core::Move tt_move, const KillerTable& killers,
                 const HistoryTable& history, int ply,
                 std::array<int, movegen::MoveList::MAX_CAPACITY>& scores) noexcept {
    for (size_t i = 0; i < moves.size(); ++i) {
        scores[i] = score_move(pos, moves[i], tt_move, killers, history, ply);
    }
}

void pick_next_move(movegen::MoveList& moves,
                    std::array<int, movegen::MoveList::MAX_CAPACITY>& scores,
                    size_t start_idx) noexcept {
    size_t best_idx = start_idx;
    int best_score = scores[start_idx];

    for (size_t i = start_idx + 1; i < moves.size(); ++i) {
        if (scores[i] > best_score) {
            best_score = scores[i];
            best_idx = i;
        }
    }

    if (best_idx != start_idx) {
        std::swap(moves[start_idx], moves[best_idx]);
        std::swap(scores[start_idx], scores[best_idx]);
    }
}

}  // namespace lookahead::search
