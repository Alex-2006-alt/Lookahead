/*
 * What: Material values, game phase weights, and PeSTO piece-square tables (PSQT).
 * Why:  Provides foundational midgame/endgame evaluation terms with smooth phase tapering.
 * Layer: Layer 2 (eval) - depends only on core types and position.
 */

#ifndef LOOKAHEAD_EVAL_MATERIAL_PSQT_H
#define LOOKAHEAD_EVAL_MATERIAL_PSQT_H

#include <cstdint>
#include <array>
#include "core/types.h"
#include "core/position.h"

namespace lookahead::eval {

// S(mg, eg) score container representing paired midgame and endgame evaluation values.
struct Score {
    int32_t mg = 0;
    int32_t eg = 0;

    constexpr Score() noexcept = default;
    constexpr Score(int32_t m, int32_t e) noexcept : mg(m), eg(e) {}

    constexpr Score operator+(const Score& o) const noexcept { return Score(mg + o.mg, eg + o.eg); }
    constexpr Score operator-(const Score& o) const noexcept { return Score(mg - o.mg, eg - o.eg); }
    constexpr Score operator*(int32_t factor) const noexcept { return Score(mg * factor, eg * factor); }
    constexpr Score& operator+=(const Score& o) noexcept { mg += o.mg; eg += o.eg; return *this; }
    constexpr Score& operator-=(const Score& o) noexcept { mg -= o.mg; eg -= o.eg; return *this; }
    constexpr Score operator-() const noexcept { return Score(-mg, -eg); }
    constexpr bool operator==(const Score& o) const noexcept { return mg == o.mg && eg == o.eg; }
};

// Maximum game phase total (4 knights*1 + 4 bishops*1 + 4 rooks*2 + 2 queens*4 = 24).
inline constexpr int TOTAL_PHASE = 24;

// Phase weights per piece type (indexed by core::PieceType 0..6: None, Pawn, Knight, Bishop, Rook, Queen, King).
inline constexpr std::array<int, 7> PIECE_PHASE = {0, 0, 1, 1, 2, 4, 0};

// PeSTO base material values: S(mg, eg) in centipawns (indexed by core::PieceType 0..6).
inline constexpr std::array<Score, 7> PIECE_VALUES = {
    Score(0, 0),       // None
    Score(82, 94),     // Pawn
    Score(337, 281),   // Knight
    Score(365, 297),   // Bishop
    Score(477, 512),   // Rook
    Score(1025, 936),  // Queen
    Score(0, 0)        // King
};

// Returns White piece-square table bonus S(mg, eg) for a given piece type and square.
[[nodiscard]] Score psqt_value(core::PieceType pt, core::Square sq) noexcept;

// Computes combined material and PSQT score for the position from White's perspective.
[[nodiscard]] std::pair<Score, Score> evaluate_material_and_psqt(const core::Position& pos) noexcept;

// Calculates current non-pawn material phase (0 = pure endgame, 24 = full opening).
[[nodiscard]] int calculate_game_phase(const core::Position& pos) noexcept;

}  // namespace lookahead::eval

#endif  // LOOKAHEAD_EVAL_MATERIAL_PSQT_H
