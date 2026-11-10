/*
 * What: Implementation of root iterative deepening and negamax alpha-beta search.
 * Why:  Explores game trees efficiently using minimax theorem, alpha-beta cutoffs, and TT.
 * Layer: Layer 3 (search) - integrates TT, quiescence, move ordering, pruning, and time management.
 */

#include "search.h"
#include <algorithm>
#include "features.h"
#include "transposition_table.h"
#include "quiescence.h"
#include "move_ordering.h"
#include "pruning.h"
#include "time_manager.h"
#include "eval/evaluate.h"
#include "movegen/movegen.h"

namespace lookahead::search {

namespace {

int negamax(core::Position& pos, int depth, int ply, int alpha, int beta,
            uint64_t& nodes, KillerTable& killers, HistoryTable& history,
            PVTable& pv, TimeManager& time_mgr, bool can_null) noexcept {
    if (time_mgr.should_stop(nodes)) {
        return 0;
    }
    ++nodes;
    pv.init(ply);

    // Quiescence search at leaf nodes
    if (depth <= 0) {
        return quiescence(pos, alpha, beta, nodes);
    }

    const bool in_check = movegen::in_check(pos);

    // Check extension: explore checks deeper to avoid horizon horizon blunders
    if (in_check) {
        ++depth;
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
                    return tt_score;
                }
                if (entry->flag == TTFlag::LowerBound && tt_score >= beta) {
                    return tt_score;
                }
                if (entry->flag == TTFlag::UpperBound && tt_score <= alpha) {
                    return tt_score;
                }
            }
        }
    }

    // Reverse Futility Pruning (Static Null Move)
    if (active_features.use_reverse_futility && !in_check && depth <= 3 && ply > 0 &&
        std::abs(beta) < MATE_THRESHOLD) {
        const int static_eval = eval::evaluate(pos);
        if (can_reverse_futility_prune(depth, static_eval, beta)) {
            return beta;
        }
    }

    // Null-Move Pruning (NMP)
    if (active_features.use_null_move_pruning && can_null && !in_check && depth >= 3 && ply > 0 &&
        std::abs(beta) < MATE_THRESHOLD && has_non_pawn_material(pos, pos.side_to_move())) {
        const int R = null_move_reduction(depth);
        core::UndoInfo null_undo;
        pos.make_null_move(null_undo);

        const int null_score = -negamax(pos, depth - 1 - R, ply + 1, -beta, -beta + 1,
                                        nodes, killers, history, pv, time_mgr, false);
        pos.unmake_null_move(null_undo);

        if (time_mgr.is_stopped()) return 0;
        if (null_score >= beta) {
            return beta;
        }
    }

    movegen::MoveList moves;
    movegen::generate_legal_moves(pos, moves);

    // Terminal position evaluation
    if (moves.empty()) {
        if (in_check) {
            return -SCORE_MATE + ply;
        }
        return 0; // Stalemate
    }

    std::array<int, movegen::MoveList::MAX_CAPACITY> scores{};
    if (active_features.use_move_ordering) {
        score_moves(pos, moves, tt_move, killers, history, ply, scores);
    } else if (!tt_move.is_null()) {
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
    int moves_searched = 0;

    for (size_t i = 0; i < moves.size(); ++i) {
        if (active_features.use_move_ordering) {
            pick_next_move(moves, scores, i);
        }

        const core::Move m = moves[i];
        pos.make_move(m, undo);
        ++moves_searched;

        const bool gives_check = movegen::in_check(pos);
        int score = 0;
        const bool is_quiet = !m.is_capture() && !m.is_promotion();

        // Late Move Reductions (LMR): quiet moves that do not give check
        if (active_features.use_late_move_reductions && moves_searched >= 4 && depth >= 3 &&
            is_quiet && !in_check && !gives_check) {
            const int R = late_move_reduction(depth, moves_searched);
            score = -negamax(pos, depth - 1 - R, ply + 1, -alpha - 1, -alpha,
                             nodes, killers, history, pv, time_mgr, true);

            if (score > alpha) {
                score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha,
                                 nodes, killers, history, pv, time_mgr, true);
            }
        } else {
            score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha,
                             nodes, killers, history, pv, time_mgr, true);
        }

        pos.unmake_move(m, undo);

        if (time_mgr.is_stopped()) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = m;
        }
        if (score > alpha) {
            alpha = score;
            pv.update(ply, m);
        }
        if (alpha >= beta) {
            if (active_features.use_move_ordering && is_quiet) {
                killers.store(ply, m);
                history.record(pos.side_to_move(), m.from_square(), m.to_square(), depth);
            }
            break;
        }
    }

    // Transposition Table recording
    if (active_features.use_transposition_table && !time_mgr.is_stopped()) {
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

SearchResult search(core::Position& pos, const SearchLimits& limits, SearchCallback callback) {
    SearchResult result;
    TimeManager time_mgr;
    time_mgr.init(limits, pos.side_to_move());

    const int max_d = (limits.depth > 0) ? limits.depth : 64;
    global_tt.advance_generation();

    KillerTable killers;
    HistoryTable history;
    killers.clear();
    history.clear();

    PVTable pv;
    int last_score = 0;

    // Iterative deepening loop
    for (int current_depth = 1; current_depth <= max_d; ++current_depth) {
        if (!time_mgr.can_start_next_depth() && current_depth > 1) {
            break;
        }

        int alpha = -SCORE_INFINITY;
        int beta = SCORE_INFINITY;
        const int delta = 50;

        if (active_features.use_aspiration_windows && current_depth >= 4 &&
            std::abs(last_score) < MATE_THRESHOLD) {
            alpha = std::max(-SCORE_INFINITY, last_score - delta);
            beta = std::min(SCORE_INFINITY, last_score + delta);
        }

        int score = 0;
        while (true) {
            score = negamax(pos, current_depth, 0, alpha, beta,
                            result.nodes, killers, history, pv, time_mgr, true);

            if (time_mgr.is_stopped()) {
                break;
            }

            if (active_features.use_aspiration_windows && current_depth >= 4) {
                if (score <= alpha) {
                    alpha = -SCORE_INFINITY;
                    continue;
                }
                if (score >= beta) {
                    beta = SCORE_INFINITY;
                    continue;
                }
            }
            break;
        }

        if (time_mgr.is_stopped() && current_depth > 1) {
            break;
        }

        last_score = score;
        result.score = score;
        result.depth_reached = current_depth;
        result.pv = pv.get_pv(0);
        if (!result.pv.empty()) {
            result.best_move = result.pv[0];
            std::string pv_str;
            for (size_t k = 0; k < result.pv.size(); ++k) {
                if (k > 0) pv_str += " ";
                pv_str += result.pv[k].to_uci();
            }
            result.pv_string = pv_str;
        }

        result.elapsed_seconds = time_mgr.elapsed_seconds();
        result.nps = (result.elapsed_seconds > 0.0)
                         ? (static_cast<double>(result.nodes) / result.elapsed_seconds)
                         : 0.0;

        if (callback) {
            SearchInfo info;
            info.depth = current_depth;
            info.score = score;
            info.nodes = result.nodes;
            info.elapsed_seconds = result.elapsed_seconds;
            info.nps = result.nps;
            info.pv = result.pv;
            info.pv_string = result.pv_string;
            callback(info);
        }

        if (std::abs(score) >= MATE_THRESHOLD) {
            break;
        }
    }

    result.elapsed_seconds = time_mgr.elapsed_seconds();
    result.nps = (result.elapsed_seconds > 0.0)
                     ? (static_cast<double>(result.nodes) / result.elapsed_seconds)
                     : 0.0;

    return result;
}

}  // namespace lookahead::search
