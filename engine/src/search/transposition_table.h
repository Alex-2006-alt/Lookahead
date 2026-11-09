/*
 * What: Fixed-capacity Transposition Table (TT) storing positions, scores, and best moves.
 * Why:  Avoids redundant branch re-evaluations and preserves principal variation moves.
 * Layer: Layer 3 (search) - depends only on core types and move.
 */

#ifndef LOOKAHEAD_SEARCH_TRANSPOSITION_TABLE_H
#define LOOKAHEAD_SEARCH_TRANSPOSITION_TABLE_H

#include <cstdint>
#include <vector>
#include <optional>
#include "core/types.h"
#include "core/move.h"

namespace lookahead::search {

inline constexpr int SCORE_MATE = 29000;
inline constexpr int SCORE_INFINITY = 30000;
inline constexpr int MATE_THRESHOLD = 28000;

// Classification of evaluation bounds stored in the table.
enum class TTFlag : uint8_t {
    None       = 0,
    Exact      = 1, // PV-node: exact score between alpha and beta
    LowerBound = 2, // Cut-node: beta cutoff (true score >= stored score)
    UpperBound = 3  // All-node: failed low (true score <= stored score)
};

// Compact entry representing a cached subtree evaluation.
struct TTEntry {
    uint64_t key = 0ULL;
    core::Move move{};
    int32_t score = 0;
    int16_t depth = 0;
    TTFlag flag = TTFlag::None;
    uint8_t age = 0;
};

// Normalizes mate score to root distance when saving to table.
[[nodiscard]] constexpr int score_to_tt(int score, int ply) noexcept {
    if (score >= MATE_THRESHOLD) return score + ply;
    if (score <= -MATE_THRESHOLD) return score - ply;
    return score;
}

// Restores mate score relative to current search ply when retrieved from table.
[[nodiscard]] constexpr int score_from_tt(int score, int ply) noexcept {
    if (score >= MATE_THRESHOLD) return score - ply;
    if (score <= -MATE_THRESHOLD) return score + ply;
    return score;
}

class TranspositionTable {
public:
    TranspositionTable() = default;
    explicit TranspositionTable(size_t size_mb);

    // Resizes transposition table to target megabytes, rounding to power of two.
    void resize(size_t size_mb);

    // Clears all cached table entries.
    void clear() noexcept;

    // Increments generation age to prefer fresher search results over obsolete iterations.
    void advance_generation() noexcept { ++current_age_; }

    // Stores or updates a position entry with replacement heuristics.
    void store(uint64_t key, core::Move move, int score, int depth, TTFlag flag, int ply) noexcept;

    // Probes table for key. Returns pointer to entry if matching key exists.
    [[nodiscard]] const TTEntry* probe(uint64_t key) const noexcept;

    // Probes table for best move without full evaluation validity check.
    [[nodiscard]] core::Move probe_move(uint64_t key) const noexcept;

    // Returns total number of allocated entries.
    [[nodiscard]] size_t capacity() const noexcept { return entries_.size(); }

private:
    std::vector<TTEntry> entries_;
    size_t size_mask_ = 0;
    uint8_t current_age_ = 0;
};

// Global engine transposition table instance.
inline TranspositionTable global_tt;

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_TRANSPOSITION_TABLE_H
