/*
 * What: Implementation of search pruning heuristics, reductions, and material checks.
 * Why:  Minimizes tree size by selectively reducing and pruning unpromising branches.
 * Layer: Layer 3 (search) - depends on pruning.h and core types.
 */

#include "pruning.h"

namespace lookahead::search {

bool has_non_pawn_material(const core::Position& pos, core::Color c) noexcept {
    const core::Bitboard non_pawn = pos.pieces(c) 
                                  ^ pos.pieces(c, core::PieceType::Pawn) 
                                  ^ pos.pieces(c, core::PieceType::King);
    return non_pawn != 0ULL;
}

int late_move_reduction(int depth, int moves_searched) noexcept {
    if (depth < 3 || moves_searched < 4) {
        return 0;
    }
    const double d_log = std::log(static_cast<double>(depth));
    const double m_log = std::log(static_cast<double>(moves_searched));
    const int r = static_cast<int>(0.75 + (d_log * m_log) / 2.25);
    return std::clamp(r, 1, depth - 2);
}

}  // namespace lookahead::search
