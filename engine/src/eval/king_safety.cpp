/*
 * What: Implementation of king shelter evaluation, file exposure, and attack units.
 * Why:  Quantifies tactical king vulnerability to enemy piece threats and mating nets.
 * Layer: Layer 2 (eval) - depends only on king_safety.h, movegen attacks, and bitboard.
 */

#include "king_safety.h"
#include "movegen/attacks.h"
#include "core/bitboard.h"

namespace lookahead::eval {

namespace {

// Evaluates king shelter, pawn shield, and file exposure for one side.
[[nodiscard]] Score evaluate_shelter(const core::Position& pos, core::Color us) noexcept {
    const core::Bitboard king_bb = pos.pieces(us, core::PieceType::King);
    if (king_bb == 0ULL) return Score{0, 0};
    Score score{0, 0};
    const core::Square king_sq = core::lsb(king_bb);
    const int k_file = static_cast<int>(core::file_of(king_sq));
    const int k_rank = static_cast<int>(core::rank_of(king_sq));

    const core::Bitboard our_pawns = pos.pieces(us, core::PieceType::Pawn);
    const core::Bitboard all_pawns = pos.pieces(core::PieceType::Pawn);

    const bool in_home_ranks = (us == core::Color::White) ? (k_rank <= 2) : (k_rank >= 5);
    if (!in_home_ranks) {
        // King is uncastled or active in center/middle board
        return Score(-30, 10); // Penalty in mg, slight activity bonus in eg
    }

    const int file_start = std::max(0, k_file - 1);
    const int file_end = std::min(7, k_file + 1);

    for (int f_idx = file_start; f_idx <= file_end; ++f_idx) {
        const auto f = static_cast<core::File>(f_idx);
        const core::Bitboard file_mask = core::file_bb(f);

        // Open file check (no pawns at all on file near king)
        if ((all_pawns & file_mask) == 0ULL) {
            score += Score(-20, 0);
        } else if ((our_pawns & file_mask) == 0ULL) {
            // Semi-open file (no friendly shield pawn)
            score += Score(-15, 0);
        } else {
            // Friendly pawn present on shelter file: reward proximity to king
            if (us == core::Color::White) {
                if (our_pawns & core::square_bb(core::make_square(f, core::Rank::Rank2))) {
                    score += Score(15, 0);
                } else if (our_pawns & core::square_bb(core::make_square(f, core::Rank::Rank3))) {
                    score += Score(8, 0);
                } else {
                    score += Score(-10, 0); // Advanced shield
                }
            } else {
                if (our_pawns & core::square_bb(core::make_square(f, core::Rank::Rank7))) {
                    score += Score(15, 0);
                } else if (our_pawns & core::square_bb(core::make_square(f, core::Rank::Rank6))) {
                    score += Score(8, 0);
                } else {
                    score += Score(-10, 0); // Advanced shield
                }
            }
        }
    }
    return score;
}

// Evaluates virtual enemy attack pressure focused on the king ring.
[[nodiscard]] Score evaluate_attack_units(const core::Position& pos, core::Color us) noexcept {
    const core::Bitboard king_bb = pos.pieces(us, core::PieceType::King);
    if (king_bb == 0ULL) return Score{0, 0};
    const core::Color them = ~us;
    const core::Square king_sq = core::lsb(king_bb);
    const core::Bitboard king_ring = movegen::attacks::king(king_sq) | core::square_bb(king_sq);
    const core::Bitboard occ = pos.occupied();

    int attack_units = 0;

    // Enemy Knights
    core::Bitboard knights = pos.pieces(them, core::PieceType::Knight);
    while (knights) {
        const core::Square sq = core::pop_lsb(knights);
        attack_units += core::popcount(movegen::attacks::knight(sq) & king_ring) * 2;
    }

    // Enemy Bishops
    core::Bitboard bishops = pos.pieces(them, core::PieceType::Bishop);
    while (bishops) {
        const core::Square sq = core::pop_lsb(bishops);
        attack_units += core::popcount(movegen::attacks::bishop(sq, occ) & king_ring) * 2;
    }

    // Enemy Rooks
    core::Bitboard rooks = pos.pieces(them, core::PieceType::Rook);
    while (rooks) {
        const core::Square sq = core::pop_lsb(rooks);
        attack_units += core::popcount(movegen::attacks::rook(sq, occ) & king_ring) * 3;
    }

    // Enemy Queens
    core::Bitboard queens = pos.pieces(them, core::PieceType::Queen);
    while (queens) {
        const core::Square sq = core::pop_lsb(queens);
        attack_units += core::popcount(movegen::attacks::queen(sq, occ) & king_ring) * 5;
    }

    // Progressive quadratic penalty in middlegame, zero in pure endgame
    const int penalty_mg = (attack_units * attack_units) / 4;
    return Score(-penalty_mg, 0);
}

}  // namespace

Score evaluate_king_safety(const core::Position& pos) noexcept {
    const Score white_shelter = evaluate_shelter(pos, core::Color::White);
    const Score white_attacks = evaluate_attack_units(pos, core::Color::White);
    const Score white_total = white_shelter + white_attacks;

    const Score black_shelter = evaluate_shelter(pos, core::Color::Black);
    const Score black_attacks = evaluate_attack_units(pos, core::Color::Black);
    const Score black_total = black_shelter + black_attacks;

    return white_total - black_total;
}

}  // namespace lookahead::eval
