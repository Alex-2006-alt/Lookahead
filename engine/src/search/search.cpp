/*
 * What: Implementation of root iterative deepening and negamax alpha-beta search.
 * Why:  Explores game trees efficiently using minimax theorem, alpha-beta cutoffs, and TT.
 * Layer: Layer 3 (search) - integrates transposition_table, quiescence, eval, and movegen.
 */

#include "search.h"
#include <chrono>
#include "features.h"
#include "transposition_table.h"
#include "quiescence.h"
#include "movegen/movegen.h"

namespace lookahead::search {

namespace {

int negamax(core::Position& pos, int depth, int ply, int alpha, int beta,
            uint64_t& nodes, core::Move& pv_move) noexcept {
    ++nodes;

    // Quiescence search at leaf nodes
    if (depth <= 0) {
        return quiescence(pos, alpha, beta, nodes);
    }

    const uint64_t key = pos.zobrist_key();
    core::Move tt_move{};

    // Transposition Table probing
    if (active_features.use_transposition_table) {
        if (const TTEntry* entry = global_tt.probe(key)) {
            tt_move = entry->move;
            if (entry->depth >= depth) {
                const int tt_score = score_from_tt(entry->score, ply);
                if (entry->flag == TTFlag::Exact) {
                    pv_move = tt_move;
                    return tt_score;
                }
                if (entry->flag == TTFlag::LowerBound && tt_score >= beta) {
                    pv_move = tt_move;
                    return tt_score;
                }
                if (entry->flag == TTFlag::UpperBound && tt_score <= alpha) {
                    return tt_score;
                }
            }
        }
    }

    movegen::MoveList moves;
    movegen::generate_legal_moves(pos, moves);

    // Terminal position evaluation
    if (moves.empty()) {
        if (movegen::in_check(pos)) {
            return -SCORE_MATE + ply; // Faster mates are preferred
        }
        return 0; // Stalemate
    }

    // Place TT move first if available for immediate beta cutoffs
    if (!tt_move.is_null()) {
        for (size_t i = 0; i < moves.size(); ++i) {
            if (moves[i] == tt_move) {
                std::swap(moves[0], moves[i]);
                break;
            }
        }
    }

    const int original_alpha = alpha;
    int best_score = -SCORE_INFINITY;
    core::Move best_move{};
    core::UndoInfo undo;

    for (size_t i = 0; i < moves.size(); ++i) {
        const core::Move m = moves[i];
        pos.make_move(m, undo);

        core::Move child_pv{};
        const int score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, nodes, child_pv);
        pos.unmake_move(m, undo);

        if (score > best_score) {
            best_score = score;
            best_move = m;
        }
        if (score > alpha) {
            alpha = score;
            pv_move = m;
        }
        if (alpha >= beta) {
            break; // Beta cutoff
        }
    }

    // Transposition Table recording
    if (active_features.use_transposition_table) {
        TTFlag flag = TTFlag::Exact;
        if (best_score <= original_alpha) {
            flag = TTFlag::UpperBound;
        } else if (best_score >= beta) {
            flag = TTFlag::LowerBound;
        }
        global_tt.store(key, best_move, best_score, depth, flag, ply);
    }

    return best_score;
}

}  // namespace

SearchResult search(core::Position& pos, const SearchLimits& limits) {
    SearchResult result;
    const auto start_time = std::chrono::high_resolution_clock::now();

    const int max_d = (limits.depth > 0) ? limits.depth : 6;
    global_tt.advance_generation();

    // Iterative deepening loop
    for (int current_depth = 1; current_depth <= max_d; ++current_depth) {
        core::Move root_pv{};
        const int score = negamax(pos, current_depth, 0, -SCORE_INFINITY, SCORE_INFINITY,
                                  result.nodes, root_pv);

        if (!root_pv.is_null()) {
            result.best_move = root_pv;
        }
        result.score = score;
        result.depth_reached = current_depth;

        // Break early if a forced checkmate is found
        if (std::abs(score) >= MATE_THRESHOLD) {
            break;
        }
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end_time - start_time;
    result.elapsed_seconds = elapsed.count();
    result.nps = (result.elapsed_seconds > 0.0)
                     ? (static_cast<double>(result.nodes) / result.elapsed_seconds)
                     : 0.0;

    return result;
}

}  // namespace lookahead::search
