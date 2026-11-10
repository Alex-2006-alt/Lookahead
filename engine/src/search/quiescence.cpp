/*
 * What: Implementation of quiescence capture search with stand-pat and MVV-LVA ordering.
 * Why:  Prevents blundering pieces in tactical sequences right at the search horizon.
 * Layer: Layer 3 (search) - depends on quiescence.h, eval, movegen, and features.
 */

#include "quiescence.h"
#include <algorithm>
#include "features.h"
#include "move_ordering.h"
#include "eval/evaluate.h"
#include "movegen/movegen.h"

namespace lookahead::search {

int quiescence(core::Position& pos, int alpha, int beta, uint64_t& nodes) noexcept {
    ++nodes;

    const int stand_pat = eval::evaluate(pos);

    // If quiescence is toggled off via feature flag, return static evaluation directly
    if (!active_features.use_quiescence_search) {
        return stand_pat;
    }

    // Stand-pat beta cutoff: position is already too favorable for opponent to allow
    if (stand_pat >= beta) {
        return beta;
    }

    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    // Generate all legal moves and filter captures and promotions
    movegen::MoveList all_moves;
    movegen::generate_legal_moves(pos, all_moves);

    struct ScoredMove {
        core::Move move;
        int score = 0;
    };
    std::array<ScoredMove, 256> captures{};
    size_t num_captures = 0;

    for (size_t i = 0; i < all_moves.size(); ++i) {
        const core::Move m = all_moves[i];
        if (m.is_capture() || m.is_promotion()) {
            captures[num_captures++] = {m, mvv_lva_score(pos, m)};
        }
    }

    // Sort captures by MVV-LVA descending order
    std::sort(captures.begin(), captures.begin() + num_captures,
              [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    core::UndoInfo undo;
    for (size_t i = 0; i < num_captures; ++i) {
        const core::Move m = captures[i].move;

        // Delta pruning: skip hopeless captures that cannot raise alpha even with safety margin
        const core::Piece captured = pos.piece_at(m.to_square());
        const int captured_val = VICTIM_SCORES[static_cast<size_t>(core::type_of(captured))];
        if (!m.is_promotion() && (stand_pat + captured_val + 200 < alpha)) {
            continue;
        }

        pos.make_move(m, undo);
        const int score = -quiescence(pos, -beta, -alpha, nodes);
        pos.unmake_move(m, undo);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

}  // namespace lookahead::search
