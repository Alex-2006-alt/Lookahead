/*
 * What: Implementation of legal move generation, pin detection, check evasions, and game ends.
 * Why:  Ensures move legality strictly before make_move to avoid speculative rollbacks.
 * Layer: Layer 1 (movegen) - depends only on movegen.h and attacks.h.
 */

#include "movegen.h"
#include "attacks.h"

namespace lookahead::movegen {

using namespace lookahead::core;

namespace {

void add_pawn_moves(core::Square from, core::Square to, core::MoveFlag flag, MoveList& list) noexcept {
    const core::Rank r = core::rank_of(to);
    if (r == core::Rank::Rank8 || r == core::Rank::Rank1) {
        const bool cap = (static_cast<uint16_t>(flag) & 0x04) != 0;
        list.push_back(core::Move(from, to, cap ? core::MoveFlag::QueenPromoCapture : core::MoveFlag::QueenPromotion));
        list.push_back(core::Move(from, to, cap ? core::MoveFlag::RookPromoCapture : core::MoveFlag::RookPromotion));
        list.push_back(core::Move(from, to, cap ? core::MoveFlag::BishopPromoCapture : core::MoveFlag::BishopPromotion));
        list.push_back(core::Move(from, to, cap ? core::MoveFlag::KnightPromoCapture : core::MoveFlag::KnightPromotion));
    } else {
        list.push_back(core::Move(from, to, flag));
    }
}

}  // namespace

core::Bitboard attackers_to(const core::Position& pos, core::Square target, core::Color by_color) noexcept {
    const core::Bitboard occ = pos.occupied();
    core::Bitboard att = 0ULL;
    att |= attacks::pawn(~by_color, target) & pos.pieces(by_color, core::PieceType::Pawn);
    att |= attacks::knight(target) & pos.pieces(by_color, core::PieceType::Knight);
    att |= attacks::king(target) & pos.pieces(by_color, core::PieceType::King);
    att |= attacks::bishop(target, occ) & (pos.pieces(by_color, core::PieceType::Bishop) | pos.pieces(by_color, core::PieceType::Queen));
    att |= attacks::rook(target, occ) & (pos.pieces(by_color, core::PieceType::Rook) | pos.pieces(by_color, core::PieceType::Queen));
    return att;
}

bool is_square_attacked(const core::Position& pos, core::Square sq, core::Color by_color) noexcept {
    return attackers_to(pos, sq, by_color) != 0ULL;
}

bool in_check(const core::Position& pos) noexcept {
    const core::Bitboard k_bb = pos.pieces(pos.side_to_move(), core::PieceType::King);
    if (k_bb == 0ULL) return false;
    return is_square_attacked(pos, core::lsb(k_bb), ~pos.side_to_move());
}

void generate_legal_moves(const core::Position& pos, MoveList& list) noexcept {
    const core::Color us = pos.side_to_move();
    const core::Color them = ~us;
    const core::Square k_sq = core::lsb(pos.pieces(us, core::PieceType::King));
    const core::Bitboard checkers = attackers_to(pos, k_sq, them);
    const int num_checkers = core::popcount(checkers);

    // 1. King moves
    core::Bitboard king_targets = attacks::king(k_sq) & ~pos.pieces(us);
    const core::Bitboard occ_without_king = pos.occupied() ^ core::square_bb(k_sq);
    while (king_targets != 0ULL) {
        const core::Square to = core::pop_lsb(king_targets);
        core::Bitboard attackers = (attacks::pawn(us, to) & pos.pieces(them, core::PieceType::Pawn))
            | (attacks::knight(to) & pos.pieces(them, core::PieceType::Knight))
            | (attacks::king(to) & pos.pieces(them, core::PieceType::King))
            | (attacks::bishop(to, occ_without_king) & (pos.pieces(them, core::PieceType::Bishop) | pos.pieces(them, core::PieceType::Queen)))
            | (attacks::rook(to, occ_without_king) & (pos.pieces(them, core::PieceType::Rook) | pos.pieces(them, core::PieceType::Queen)));
        if (attackers == 0ULL) {
            const MoveFlag flag = (pos.piece_at(to) != core::Piece::None) ? MoveFlag::Capture : MoveFlag::Quiet;
            list.push_back(core::Move(k_sq, to, flag));
        }
    }

    // In double check, only king moves can escape check!
    if (num_checkers >= 2) return;

    // 2. Identify pin rays
    core::Bitboard pinned_bb = 0ULL;
    std::array<core::Bitboard, 64> pin_rays{};
    core::Bitboard pinners = (attacks::bishop(k_sq, 0ULL) & (pos.pieces(them, core::PieceType::Bishop) | pos.pieces(them, core::PieceType::Queen)))
        | (attacks::rook(k_sq, 0ULL) & (pos.pieces(them, core::PieceType::Rook) | pos.pieces(them, core::PieceType::Queen)));
    while (pinners != 0ULL) {
        const core::Square p_sq = core::pop_lsb(pinners);
        const core::Bitboard between_ray = attacks::between(k_sq, p_sq);
        const core::Bitboard blockers = between_ray & pos.occupied();
        if (core::popcount(blockers) == 1 && (blockers & pos.pieces(us))) {
            const core::Square pinned_sq = core::lsb(blockers);
            core::set_bit(pinned_bb, pinned_sq);
            pin_rays[static_cast<uint8_t>(pinned_sq)] = attacks::line(k_sq, p_sq);
        }
    }

    // 3. Target squares for non-king moves (intercept ray or capture checker)
    const core::Bitboard check_mask = (num_checkers == 1)
        ? (checkers | attacks::between(k_sq, core::lsb(checkers)))
        : core::BitboardAll;

    // Pawns
    const auto pawn_dir = (us == core::Color::White) ? 8 : -8;
    const auto double_push_rank = (us == core::Color::White) ? core::Rank::Rank3 : core::Rank::Rank6;
    core::Bitboard pawns = pos.pieces(us, core::PieceType::Pawn);
    while (pawns != 0ULL) {
        const core::Square from = core::pop_lsb(pawns);
        const core::Bitboard pin_line = core::test_bit(pinned_bb, from) ? pin_rays[static_cast<uint8_t>(from)] : core::BitboardAll;

        // Single push
        const auto to_sq_idx = static_cast<int>(from) + pawn_dir;
        if (to_sq_idx >= 0 && to_sq_idx < 64) {
            const auto to = static_cast<core::Square>(to_sq_idx);
            if (pos.piece_at(to) == core::Piece::None) {
                if (core::test_bit(check_mask & pin_line, to)) {
                    add_pawn_moves(from, to, MoveFlag::Quiet, list);
                }
                // Double push
                if (core::rank_of(to) == double_push_rank) {
                    const auto d_to = static_cast<core::Square>(to_sq_idx + pawn_dir);
                    if (pos.piece_at(d_to) == core::Piece::None && core::test_bit(check_mask & pin_line, d_to)) {
                        list.push_back(core::Move(from, d_to, MoveFlag::DoublePawnPush));
                    }
                }
            }
        }

        // Diagonal pawn captures
        core::Bitboard cap_targets = attacks::pawn(us, from) & pos.pieces(them) & check_mask & pin_line;
        while (cap_targets != 0ULL) {
            const core::Square to = core::pop_lsb(cap_targets);
            add_pawn_moves(from, to, MoveFlag::Capture, list);
        }

        // En passant
        if (pos.en_passant_sq() != core::Square::None) {
            const core::Square ep_to = pos.en_passant_sq();
            if (core::test_bit(attacks::pawn(us, from), ep_to)) {
                const core::Square cap_pawn_sq = core::make_square(core::file_of(ep_to), core::rank_of(from));
                if (core::test_bit(check_mask, ep_to) || core::test_bit(checkers, cap_pawn_sq)) {
                    // Check horizontal discovered check edge case
                    const core::Bitboard occ_after = (pos.occupied() ^ core::square_bb(from) ^ core::square_bb(cap_pawn_sq)) | core::square_bb(ep_to);
                    const core::Bitboard slider_att = attacks::rook(k_sq, occ_after) & (pos.pieces(them, core::PieceType::Rook) | pos.pieces(them, core::PieceType::Queen));
                    const core::Bitboard diag_att = attacks::bishop(k_sq, occ_after) & (pos.pieces(them, core::PieceType::Bishop) | pos.pieces(them, core::PieceType::Queen));
                    if (slider_att == 0ULL && diag_att == 0ULL && (!core::test_bit(pinned_bb, from) || core::test_bit(pin_line, ep_to))) {
                        list.push_back(core::Move(from, ep_to, MoveFlag::EnPassant));
                    }
                }
            }
        }
    }

    // Knights (pinned knights can never move)
    core::Bitboard knights = pos.pieces(us, core::PieceType::Knight) & ~pinned_bb;
    while (knights != 0ULL) {
        const core::Square from = core::pop_lsb(knights);
        core::Bitboard targets = attacks::knight(from) & ~pos.pieces(us) & check_mask;
        while (targets != 0ULL) {
            const core::Square to = core::pop_lsb(targets);
            const MoveFlag flag = (pos.piece_at(to) != core::Piece::None) ? MoveFlag::Capture : MoveFlag::Quiet;
            list.push_back(core::Move(from, to, flag));
        }
    }

    // Sliders: Bishops, Rooks, Queens
    auto gen_sliders = [&](core::PieceType pt, bool is_diag, bool is_orth) {
        core::Bitboard pcs = pos.pieces(us, pt);
        while (pcs != 0ULL) {
            const core::Square from = core::pop_lsb(pcs);
            const core::Bitboard pin_line = core::test_bit(pinned_bb, from) ? pin_rays[static_cast<uint8_t>(from)] : core::BitboardAll;
            core::Bitboard targets = 0ULL;
            if (is_diag) targets |= attacks::bishop(from, pos.occupied());
            if (is_orth) targets |= attacks::rook(from, pos.occupied());
            targets &= ~pos.pieces(us) & check_mask & pin_line;
            while (targets != 0ULL) {
                const core::Square to = core::pop_lsb(targets);
                const MoveFlag flag = (pos.piece_at(to) != core::Piece::None) ? MoveFlag::Capture : MoveFlag::Quiet;
                list.push_back(core::Move(from, to, flag));
            }
        }
    };
    gen_sliders(core::PieceType::Bishop, true, false);
    gen_sliders(core::PieceType::Rook, false, true);
    gen_sliders(core::PieceType::Queen, true, true);

    // Castling (only if not in check)
    if (num_checkers == 0) {
        if (us == core::Color::White) {
            if ((pos.castling_rights() & core::CastlingRights::WhiteOO)
                && pos.piece_at(core::Square::F1) == core::Piece::None && pos.piece_at(core::Square::G1) == core::Piece::None
                && !is_square_attacked(pos, core::Square::F1, them) && !is_square_attacked(pos, core::Square::G1, them)) {
                list.push_back(core::Move(core::Square::E1, core::Square::G1, MoveFlag::KingCastle));
            }
            if ((pos.castling_rights() & core::CastlingRights::WhiteOOO)
                && pos.piece_at(core::Square::D1) == core::Piece::None && pos.piece_at(core::Square::C1) == core::Piece::None && pos.piece_at(core::Square::B1) == core::Piece::None
                && !is_square_attacked(pos, core::Square::D1, them) && !is_square_attacked(pos, core::Square::C1, them)) {
                list.push_back(core::Move(core::Square::E1, core::Square::C1, MoveFlag::QueenCastle));
            }
        } else {
            if ((pos.castling_rights() & core::CastlingRights::BlackOO)
                && pos.piece_at(core::Square::F8) == core::Piece::None && pos.piece_at(core::Square::G8) == core::Piece::None
                && !is_square_attacked(pos, core::Square::F8, them) && !is_square_attacked(pos, core::Square::G8, them)) {
                list.push_back(core::Move(core::Square::E8, core::Square::G8, MoveFlag::KingCastle));
            }
            if ((pos.castling_rights() & core::CastlingRights::BlackOOO)
                && pos.piece_at(core::Square::D8) == core::Piece::None && pos.piece_at(core::Square::C8) == core::Piece::None && pos.piece_at(core::Square::B8) == core::Piece::None
                && !is_square_attacked(pos, core::Square::D8, them) && !is_square_attacked(pos, core::Square::C8, them)) {
                list.push_back(core::Move(core::Square::E8, core::Square::C8, MoveFlag::QueenCastle));
            }
        }
    }
}

bool is_checkmate(const core::Position& pos) noexcept {
    if (!in_check(pos)) return false;
    MoveList list;
    generate_legal_moves(pos, list);
    return list.empty();
}

bool is_stalemate(const core::Position& pos) noexcept {
    if (in_check(pos)) return false;
    MoveList list;
    generate_legal_moves(pos, list);
    return list.empty();
}

bool is_fifty_move_rule(const core::Position& pos) noexcept {
    return pos.halfmove_clock() >= 100;
}

bool is_insufficient_material(const core::Position& pos) noexcept {
    if (pos.pieces(core::PieceType::Pawn) || pos.pieces(core::PieceType::Rook) || pos.pieces(core::PieceType::Queen)) {
        return false;
    }
    const int knights = core::popcount(pos.pieces(core::PieceType::Knight));
    const int bishops = core::popcount(pos.pieces(core::PieceType::Bishop));
    if (knights + bishops <= 1) return true;
    if (knights == 0 && bishops == 2) {
        // Bishop pair on same color squares (one per side)
        const core::Bitboard w_b = pos.pieces(core::Color::White, core::PieceType::Bishop);
        const core::Bitboard b_b = pos.pieces(core::Color::Black, core::PieceType::Bishop);
        if (core::popcount(w_b) == 1 && core::popcount(b_b) == 1) {
            const core::Bitboard dark_sqs = 0xAA55AA55AA55AA55ULL;
            const bool w_dark = (w_b & dark_sqs) != 0ULL;
            const bool b_dark = (b_b & dark_sqs) != 0ULL;
            return w_dark == b_dark;
        }
    }
    return false;
}

}  // namespace lookahead::movegen
