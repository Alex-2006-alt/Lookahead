/*
 * What: Move ordering tables (Killers, History) and dynamic move prioritization.
 * Why:  Maximizes alpha-beta pruning efficiency by exploring high-probability cutoffs first.
 * Layer: Layer 3 (search) - relies on core and movegen.
 */

#ifndef LOOKAHEAD_SEARCH_MOVE_ORDERING_H
#define LOOKAHEAD_SEARCH_MOVE_ORDERING_H

#include <array>
#include <cstdint>
#include <cstddef>
#include "core/types.h"
#include "core/move.h"
#include "core/position.h"
#include "movegen/movegen.h"

namespace lookahead::search {

inline constexpr int MAX_SEARCH_PLY = 64;

// Move ordering heuristic score tiers.
inline constexpr int SCORE_TT_MOVE = 2'000'000;
inline constexpr int SCORE_CAPTURE_BASE = 1'000'000;
inline constexpr int SCORE_PROMOTION_BASE = 900'000;
inline constexpr int SCORE_KILLER_1 = 800'000;
inline constexpr int SCORE_KILLER_2 = 700'000;
inline constexpr int SCORE_HISTORY_MAX = 50'000;

// MVV-LVA victim values (None=0, Pawn=100, Knight=300, Bishop=300, Rook=500, Queen=900, King=0).
inline constexpr std::array<int, 7> VICTIM_SCORES = {0, 100, 300, 300, 500, 900, 0};

// Computes Most Valuable Victim - Least Valuable Attacker score for captures.
[[nodiscard]] int mvv_lva_score(const core::Position& pos, core::Move m) noexcept;

// Table storing two non-capture moves per search ply that caused beta cutoffs.
class KillerTable {
public:
    void clear() noexcept {
        for (auto& ply_killers : killers_) {
            ply_killers[0] = core::Move{};
            ply_killers[1] = core::Move{};
        }
    }

    void store(int ply, core::Move m) noexcept {
        if (ply >= 0 && ply < MAX_SEARCH_PLY) {
            if (killers_[ply][0] != m) {
                killers_[ply][1] = killers_[ply][0];
                killers_[ply][0] = m;
            }
        }
    }

    [[nodiscard]] bool is_killer_1(int ply, core::Move m) const noexcept {
        return ply >= 0 && ply < MAX_SEARCH_PLY && killers_[ply][0] == m;
    }

    [[nodiscard]] bool is_killer_2(int ply, core::Move m) const noexcept {
        return ply >= 0 && ply < MAX_SEARCH_PLY && killers_[ply][1] == m;
    }

    [[nodiscard]] core::Move killer_1(int ply) const noexcept {
        return (ply >= 0 && ply < MAX_SEARCH_PLY) ? killers_[ply][0] : core::Move{};
    }

    [[nodiscard]] core::Move killer_2(int ply) const noexcept {
        return (ply >= 0 && ply < MAX_SEARCH_PLY) ? killers_[ply][1] : core::Move{};
    }

private:
    std::array<std::array<core::Move, 2>, MAX_SEARCH_PLY> killers_{};
};

// Butterfly history table tracking successful quiet moves across entire search tree.
class HistoryTable {
public:
    void clear() noexcept {
        for (auto& color_table : table_) {
            for (auto& from_table : color_table) {
                from_table.fill(0);
            }
        }
    }

    void record(core::Color c, core::Square from, core::Square to, int depth) noexcept {
        const size_t c_idx = static_cast<size_t>(c);
        const size_t from_idx = static_cast<size_t>(from);
        const size_t to_idx = static_cast<size_t>(to);
        if (c_idx < 2 && from_idx < 64 && to_idx < 64) {
            const int bonus = depth * depth;
            table_[c_idx][from_idx][to_idx] += bonus;
            if (table_[c_idx][from_idx][to_idx] > SCORE_HISTORY_MAX) {
                age();
            }
        }
    }

    [[nodiscard]] int get(core::Color c, core::Square from, core::Square to) const noexcept {
        const size_t c_idx = static_cast<size_t>(c);
        const size_t from_idx = static_cast<size_t>(from);
        const size_t to_idx = static_cast<size_t>(to);
        if (c_idx < 2 && from_idx < 64 && to_idx < 64) {
            return table_[c_idx][from_idx][to_idx];
        }
        return 0;
    }

private:
    void age() noexcept {
        for (auto& color_table : table_) {
            for (auto& from_table : color_table) {
                for (int& val : from_table) {
                    val /= 2;
                }
            }
        }
    }

    std::array<std::array<std::array<int, 64>, 64>, 2> table_{};
};

// Assigns an ordering score to a single legal move.
[[nodiscard]] int score_move(const core::Position& pos, core::Move m, core::Move tt_move,
                             const KillerTable& killers, const HistoryTable& history,
                             int ply) noexcept;

// Populates an array of heuristic scores for each move in the list.
void score_moves(const core::Position& pos, const movegen::MoveList& moves,
                 core::Move tt_move, const KillerTable& killers,
                 const HistoryTable& history, int ply,
                 std::array<int, movegen::MoveList::MAX_CAPACITY>& scores) noexcept;

// In-place selection swap finding the highest-scored move starting from start_idx.
void pick_next_move(movegen::MoveList& moves,
                    std::array<int, movegen::MoveList::MAX_CAPACITY>& scores,
                    size_t start_idx) noexcept;

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_MOVE_ORDERING_H
