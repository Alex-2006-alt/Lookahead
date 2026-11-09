/*
 * What: Root search interface, alpha-beta negamax engine, and search limits container.
 * Why:  Orchestrates minimax tree exploration, transposition table caching, and quiescence.
 * Layer: Layer 3 (search) - coordinates all search heuristics above eval and movegen.
 */

#ifndef LOOKAHEAD_SEARCH_SEARCH_H
#define LOOKAHEAD_SEARCH_SEARCH_H

#include <cstdint>
#include "core/types.h"
#include "core/move.h"
#include "core/position.h"

namespace lookahead::search {

// Constraints governing the scope of the search execution.
struct SearchLimits {
    int depth = 0;          // Fixed depth limit (0 = time-directed search)
    uint64_t max_nodes = 0; // Maximum nodes before search termination
    int move_time_ms = 0;   // Exact time in milliseconds allocated for move
};

// Principal search output and performance diagnostic metrics.
struct SearchResult {
    core::Move best_move{};
    int score = 0;
    int depth_reached = 0;
    uint64_t nodes = 0;
    double elapsed_seconds = 0.0;
    double nps = 0.0;
};

// Executes alpha-beta negamax search from root position according to limits.
[[nodiscard]] SearchResult search(core::Position& pos, const SearchLimits& limits);

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_SEARCH_H
