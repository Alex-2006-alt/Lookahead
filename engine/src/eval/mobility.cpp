/*
 * What: Implementation of piece mobility evaluation using attacks and safe target masks.
 * Why:  Rewards activity and piece scope while avoiding squares controlled by enemy pawns.
 * Layer: Layer 2 (eval) - depends only on mobility.h, movegen attacks, and bitboard.
 */

#include "mobility.h"
#include "movegen/attacks.h"
#include "core/bitboard.h"

namespace lookahead::eval {

namespace {

// Evaluates mobility score for one side.
[[nodiscard]] Score evaluate_side_mobility(const core::Position& pos, core::Color us) noexcept {
    Score score{0, 0};
    const core::Color them = ~us;
    const core::Bitboard occ = pos.occupied();

    // Squares attacked by enemy pawns are unsafe for friendly pieces
    core::Bitboard enemy_pawn_attacks = 0ULL;
    core::Bitboard enemy_pawns = pos.pieces(them, core::PieceType::Pawn);
    while (enemy_pawns) {
        const core::Square sq = core::pop_lsb(enemy_pawns);
        enemy_pawn_attacks |= movegen::attacks::pawn(them, sq);
    }

    const core::Bitboard safe_mask = ~pos.pieces(us) & ~enemy_pawn_attacks;

    // Knights
    core::Bitboard knights = pos.pieces(us, core::PieceType::Knight);
    while (knights) {
        const core::Square sq = core::pop_lsb(knights);
        const core::Bitboard reachable = movegen::attacks::knight(sq) & safe_mask;
        score += KNIGHT_MOBILITY_WEIGHT * core::popcount(reachable);
    }

    // Bishops
    core::Bitboard bishops = pos.pieces(us, core::PieceType::Bishop);
    while (bishops) {
        const core::Square sq = core::pop_lsb(bishops);
        const core::Bitboard reachable = movegen::attacks::bishop(sq, occ) & safe_mask;
        score += BISHOP_MOBILITY_WEIGHT * core::popcount(reachable);
    }

    // Rooks
    core::Bitboard rooks = pos.pieces(us, core::PieceType::Rook);
    while (rooks) {
        const core::Square sq = core::pop_lsb(rooks);
        const core::Bitboard reachable = movegen::attacks::rook(sq, occ) & safe_mask;
        score += ROOK_MOBILITY_WEIGHT * core::popcount(reachable);
    }

    // Queens
    core::Bitboard queens = pos.pieces(us, core::PieceType::Queen);
    while (queens) {
        const core::Square sq = core::pop_lsb(queens);
        const core::Bitboard reachable = movegen::attacks::queen(sq, occ) & safe_mask;
        score += QUEEN_MOBILITY_WEIGHT * core::popcount(reachable);
    }

    return score;
}

}  // namespace

Score evaluate_mobility(const core::Position& pos) noexcept {
    const Score white_mobility = evaluate_side_mobility(pos, core::Color::White);
    const Score black_mobility = evaluate_side_mobility(pos, core::Color::Black);
    return white_mobility - black_mobility;
}

}  // namespace lookahead::eval
