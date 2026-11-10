/*
 * What: Root search interface, alpha-beta negamax engine, and search limits container.
 * Why:  Orchestrates minimax tree exploration, transposition table caching, and quiescence.
 * Layer: Layer 3 (search) - coordinates all search heuristics above eval and movegen.
 */

#ifndef LOOKAHEAD_SEARCH_SEARCH_H
#define LOOKAHEAD_SEARCH_SEARCH_H

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include "core/types.h"
#include "core/move.h"
#include "core/position.h"
#include "move_ordering.h"

namespace lookahead::search {

// Triangular array for exact and deterministic principal variation tracking.
struct PVTable {
    std::array<std::array<core::Move, MAX_SEARCH_PLY>, MAX_SEARCH_PLY> table{};
    std::array<int, MAX_SEARCH_PLY> length{};

    void init(int ply) noexcept {
        if (ply < MAX_SEARCH_PLY) length[ply] = ply;
    }

    void update(int ply, core::Move m) noexcept {
        if (ply < MAX_SEARCH_PLY) {
            table[ply][ply] = m;
            const int next_len = (ply + 1 < MAX_SEARCH_PLY) ? length[ply + 1] : ply + 1;
            for (int j = ply + 1; j < next_len && j < MAX_SEARCH_PLY; ++j) {
                table[ply][j] = table[ply + 1][j];
            }
            length[ply] = next_len;
        }
    }

    [[nodiscard]] std::vector<core::Move> get_pv(int ply) const {
        std::vector<core::Move> pv;
        if (ply < MAX_SEARCH_PLY) {
            for (int i = ply; i < length[ply] && i < MAX_SEARCH_PLY; ++i) {
                if (!table[ply][i].is_null()) pv.push_back(table[ply][i]);
            }
        }
        return pv;
    }
};

// Constraints governing the scope of the search execution.
struct SearchLimits {
    int depth = 0;          // Fixed depth limit (0 = time-directed search)
    uint64_t max_nodes = 0; // Maximum nodes before search termination
    int move_time_ms = 0;   // Exact time in milliseconds allocated for move
    int wtime_ms = 0;       // White remaining time in milliseconds
    int btime_ms = 0;       // Black remaining time in milliseconds
    int winc_ms = 0;        // White increment per move in milliseconds
    int binc_ms = 0;        // Black increment per move in milliseconds
    int moves_to_go = 0;    // Moves until next time control (0 = sudden death)
};

// Real-time iteration telemetry structure.
struct SearchInfo {
    int depth = 0;
    int score = 0;
    uint64_t nodes = 0;
    double elapsed_seconds = 0.0;
    double nps = 0.0;
    std::vector<core::Move> pv{};
    std::string pv_string{};
};

// Callback signature for streaming search updates.
using SearchCallback = std::function<void(const SearchInfo&)>;

// Principal search output and performance diagnostic metrics.
struct SearchResult {
    core::Move best_move{};
    int score = 0;
    int depth_reached = 0;
    uint64_t nodes = 0;
    double elapsed_seconds = 0.0;
    double nps = 0.0;
    std::vector<core::Move> pv{};
    std::string pv_string{};
};

// Executes alpha-beta negamax search from root position according to limits.
[[nodiscard]] SearchResult search(core::Position& pos, const SearchLimits& limits,
                                  SearchCallback callback = nullptr);

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_SEARCH_H
