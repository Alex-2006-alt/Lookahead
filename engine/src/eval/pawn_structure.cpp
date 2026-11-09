/*
 * What: Implementation of pawn structure analysis (doubled, isolated, passed, connected).
 * Why:  Evaluates positional pawn strengths and weaknesses with exact color symmetry.
 * Layer: Layer 2 (eval) - depends only on pawn_structure.h, bitboard, and types.
 */

#include "pawn_structure.h"
#include "core/bitboard.h"

namespace lookahead::eval {

namespace {

// Returns squares in front of square sq on file and adjacent files.
[[nodiscard]] core::Bitboard passer_mask(core::Color c, core::Square sq) noexcept {
    const int f = static_cast<int>(core::file_of(sq));
    const int r = static_cast<int>(core::rank_of(sq));
    core::Bitboard mask = 0ULL;

    const int file_start = std::max(0, f - 1);
    const int file_end = std::min(7, f + 1);

    if (c == core::Color::White) {
        for (int rank_idx = r + 1; rank_idx < 8; ++rank_idx) {
            for (int file_idx = file_start; file_idx <= file_end; ++file_idx) {
                mask |= core::square_bb(core::make_square(static_cast<core::File>(file_idx),
                                                         static_cast<core::Rank>(rank_idx)));
            }
        }
    } else {
        for (int rank_idx = r - 1; rank_idx >= 0; --rank_idx) {
            for (int file_idx = file_start; file_idx <= file_end; ++file_idx) {
                mask |= core::square_bb(core::make_square(static_cast<core::File>(file_idx),
                                                         static_cast<core::Rank>(rank_idx)));
            }
        }
    }
    return mask;
}

// Evaluates pawn structure features for one side.
[[nodiscard]] Score evaluate_side_pawns(const core::Position& pos, core::Color us) noexcept {
    Score score{0, 0};
    const core::Color them = ~us;
    const core::Bitboard our_pawns = pos.pieces(us, core::PieceType::Pawn);
    const core::Bitboard enemy_pawns = pos.pieces(them, core::PieceType::Pawn);

    // 1. Doubled pawns: penalize files with more than one pawn
    for (uint8_t file_idx = 0; file_idx < 8; ++file_idx) {
        const auto f = static_cast<core::File>(file_idx);
        const int count = core::popcount(our_pawns & core::file_bb(f));
        if (count > 1) {
            score += DOUBLED_PAWN_PENALTY * (count - 1);
        }
    }

    core::Bitboard pawns_copy = our_pawns;
    while (pawns_copy) {
        const core::Square sq = core::pop_lsb(pawns_copy);
        const auto f = core::file_of(sq);
        const auto r = core::rank_of(sq);
        const int rel_rank = (us == core::Color::White) ? static_cast<int>(r) : (7 - static_cast<int>(r));

        // 2. Isolated pawns: no friendly pawns on adjacent files
        core::Bitboard adj_files = 0ULL;
        if (f > core::File::FileA) adj_files |= core::file_bb(static_cast<core::File>(static_cast<uint8_t>(f) - 1));
        if (f < core::File::FileH) adj_files |= core::file_bb(static_cast<core::File>(static_cast<uint8_t>(f) + 1));

        if ((our_pawns & adj_files) == 0ULL) {
            score += ISOLATED_PAWN_PENALTY;
        }

        // 3. Passed pawns: no enemy pawns on file or adjacent files in front
        if ((passer_mask(us, sq) & enemy_pawns) == 0ULL) {
            score += PASSED_PAWN_BONUS[static_cast<size_t>(rel_rank)];
        }

        // 4. Connected pawns: supported diagonally from behind or adjacent phalanx
        bool supported = false;
        if (us == core::Color::White) {
            if (f > core::File::FileA && r > core::Rank::Rank1) {
                const auto sup_sq = core::make_square(static_cast<core::File>(static_cast<uint8_t>(f) - 1),
                                                      static_cast<core::Rank>(static_cast<uint8_t>(r) - 1));
                if (pos.piece_at(sup_sq) == core::Piece::WhitePawn) supported = true;
            }
            if (f < core::File::FileH && r > core::Rank::Rank1) {
                const auto sup_sq = core::make_square(static_cast<core::File>(static_cast<uint8_t>(f) + 1),
                                                      static_cast<core::Rank>(static_cast<uint8_t>(r) - 1));
                if (pos.piece_at(sup_sq) == core::Piece::WhitePawn) supported = true;
            }
        } else {
            if (f > core::File::FileA && r < core::Rank::Rank8) {
                const auto sup_sq = core::make_square(static_cast<core::File>(static_cast<uint8_t>(f) - 1),
                                                      static_cast<core::Rank>(static_cast<uint8_t>(r) + 1));
                if (pos.piece_at(sup_sq) == core::Piece::BlackPawn) supported = true;
            }
            if (f < core::File::FileH && r < core::Rank::Rank8) {
                const auto sup_sq = core::make_square(static_cast<core::File>(static_cast<uint8_t>(f) + 1),
                                                      static_cast<core::Rank>(static_cast<uint8_t>(r) + 1));
                if (pos.piece_at(sup_sq) == core::Piece::BlackPawn) supported = true;
            }
        }
        if (supported) {
            score += PAWN_SUPPORT_BONUS;
        }

        // Phalanx: pawns side by side on same rank (counted once from left to right)
        if (f < core::File::FileH) {
            const auto right_sq = core::make_square(static_cast<core::File>(static_cast<uint8_t>(f) + 1), r);
            const core::Piece expected = (us == core::Color::White) ? core::Piece::WhitePawn : core::Piece::BlackPawn;
            if (pos.piece_at(right_sq) == expected) {
                score += PHALANX_BONUS;
            }
        }
    }

    return score;
}

}  // namespace

Score evaluate_pawn_structure(const core::Position& pos) noexcept {
    const Score white_pawns = evaluate_side_pawns(pos, core::Color::White);
    const Score black_pawns = evaluate_side_pawns(pos, core::Color::Black);
    return white_pawns - black_pawns;
}

}  // namespace lookahead::eval
