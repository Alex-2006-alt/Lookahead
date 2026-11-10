/*
 * What: Strictly legal move generation, attack detection, and game-end rule checks.
 * Why:  Produces all legal moves in any position with zero heap allocations on the search stack.
 * Layer: Layer 1 (movegen) - depends only on core types, bitboard, move, position, and attacks.
 */

#ifndef LOOKAHEAD_MOVEGEN_MOVEGEN_H
#define LOOKAHEAD_MOVEGEN_MOVEGEN_H

#include <array>
#include <cstdint>
#include "core/types.h"
#include "core/bitboard.h"
#include "core/move.h"
#include "core/position.h"

namespace lookahead::movegen {

// Stack-allocated container holding up to 256 moves with zero dynamic allocations.
class MoveList {
public:
    static constexpr size_t MAX_CAPACITY = 256;

    void push_back(core::Move m) noexcept { moves_[size_++] = m; }
    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] core::Move operator[](size_t idx) const noexcept { return moves_[idx]; }
    [[nodiscard]] core::Move& operator[](size_t idx) noexcept { return moves_[idx]; }
    [[nodiscard]] core::Move* begin() noexcept { return moves_.data(); }
    [[nodiscard]] core::Move* end() noexcept { return moves_.data() + size_; }
    [[nodiscard]] const core::Move* begin() const noexcept { return moves_.data(); }
    [[nodiscard]] const core::Move* end() const noexcept { return moves_.data() + size_; }

    [[nodiscard]] bool contains(core::Move m) const noexcept {
        for (size_t i = 0; i < size_; ++i) {
            if (moves_[i] == m) return true;
        }
        return false;
    }

private:
    std::array<core::Move, 256> moves_{};
    size_t size_ = 0;
};

// Generates all strictly legal moves for the current side to move.
void generate_legal_moves(const core::Position& pos, MoveList& list) noexcept;

// Returns a bitboard of all enemy pieces attacking the specified target square.
[[nodiscard]] core::Bitboard attackers_to(const core::Position& pos, core::Square target, core::Color by_color) noexcept;

// Returns true if the square is under attack by the given side.
[[nodiscard]] bool is_square_attacked(const core::Position& pos, core::Square sq, core::Color by_color) noexcept;

// Returns true if the king of the side to move is currently in check.
[[nodiscard]] bool in_check(const core::Position& pos) noexcept;

// Game-end state evaluation
[[nodiscard]] bool is_checkmate(const core::Position& pos) noexcept;
[[nodiscard]] bool is_stalemate(const core::Position& pos) noexcept;
[[nodiscard]] bool is_insufficient_material(const core::Position& pos) noexcept;
[[nodiscard]] bool is_fifty_move_rule(const core::Position& pos) noexcept;

}  // namespace lookahead::movegen

#endif  // LOOKAHEAD_MOVEGEN_MOVEGEN_H
