/*
 * What: Implementation of fixed-capacity Transposition Table methods.
 * Why:  Manages hash entry replacement, memory allocation, and key indexing.
 * Layer: Layer 3 (search) - depends only on transposition_table.h and types.
 */

#include "transposition_table.h"
#include <cstring>

namespace lookahead::search {

TranspositionTable::TranspositionTable(size_t size_mb) {
    resize(size_mb);
}

void TranspositionTable::resize(size_t size_mb) {
    if (size_mb == 0) size_mb = 1;
    const size_t bytes = size_mb * 1024ULL * 1024ULL;
    const size_t count = bytes / sizeof(TTEntry);

    size_t pow2 = 1;
    while ((pow2 << 1) <= count) {
        pow2 <<= 1;
    }
    entries_.resize(pow2);
    size_mask_ = pow2 - 1;
    clear();
}

void TranspositionTable::clear() noexcept {
    for (auto& entry : entries_) {
        entry = TTEntry{};
    }
    current_age_ = 0;
}

void TranspositionTable::store(uint64_t key, core::Move move, int score,
                               int depth, TTFlag flag, int ply) noexcept {
    if (entries_.empty()) return;

    const size_t index = key & size_mask_;
    auto& entry = entries_[index];

    // Preserve existing TT move if new move is empty
    if (entry.key == key && move.is_null()) {
        move = entry.move;
    }

    // Replacement condition: empty slot, same position, older generation, or deeper depth
    const bool should_replace = (entry.key == 0ULL) ||
                                (entry.key == key) ||
                                (entry.age != current_age_) ||
                                (depth >= entry.depth) ||
                                (flag == TTFlag::Exact);

    if (should_replace) {
        entry.key = key;
        entry.move = move;
        entry.score = score_to_tt(score, ply);
        entry.depth = static_cast<int16_t>(depth);
        entry.flag = flag;
        entry.age = current_age_;
    }
}

const TTEntry* TranspositionTable::probe(uint64_t key) const noexcept {
    if (entries_.empty()) return nullptr;
    const size_t index = key & size_mask_;
    const auto& entry = entries_[index];
    if (entry.key == key) {
        return &entry;
    }
    return nullptr;
}

core::Move TranspositionTable::probe_move(uint64_t key) const noexcept {
    if (entries_.empty()) return core::Move{};
    const size_t index = key & size_mask_;
    const auto& entry = entries_[index];
    if (entry.key == key) {
        return entry.move;
    }
    return core::Move{};
}

}  // namespace lookahead::search
