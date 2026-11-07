/*
 * What: Implementation of the chess Position state machine, FEN parsing, and make/unmake.
 * Why:  Maintains bitboards, pieces, castling privileges, and incremental Zobrist hashes.
 * Layer: Layer 0 (core) - depends only on position.h and standard C++ headers.
 */

#include "position.h"
#include <sstream>

namespace lookahead::core {

namespace {

[[nodiscard]] std::optional<Piece> char_to_piece(char c) noexcept {
    switch (c) {
        case 'P': return Piece::WhitePawn;   case 'p': return Piece::BlackPawn;
        case 'N': return Piece::WhiteKnight; case 'n': return Piece::BlackKnight;
        case 'B': return Piece::WhiteBishop; case 'b': return Piece::BlackBishop;
        case 'R': return Piece::WhiteRook;   case 'r': return Piece::BlackRook;
        case 'Q': return Piece::WhiteQueen;  case 'q': return Piece::BlackQueen;
        case 'K': return Piece::WhiteKing;   case 'k': return Piece::BlackKing;
        default:  return std::nullopt;
    }
}

[[nodiscard]] char piece_to_char(Piece p) noexcept {
    switch (p) {
        case Piece::WhitePawn:   return 'P'; case Piece::BlackPawn:   return 'p';
        case Piece::WhiteKnight: return 'N'; case Piece::BlackKnight: return 'n';
        case Piece::WhiteBishop: return 'B'; case Piece::BlackBishop: return 'b';
        case Piece::WhiteRook:   return 'R'; case Piece::BlackRook:   return 'r';
        case Piece::WhiteQueen:  return 'Q'; case Piece::BlackQueen:  return 'q';
        case Piece::WhiteKing:   return 'K'; case Piece::BlackKing:   return 'k';
        default: return ' ';
    }
}

constexpr Square W_KS_R_FROM = Square::H1, W_KS_R_TO = Square::F1;
constexpr Square W_QS_R_FROM = Square::A1, W_QS_R_TO = Square::D1;
constexpr Square B_KS_R_FROM = Square::H8, B_KS_R_TO = Square::F8;
constexpr Square B_QS_R_FROM = Square::A8, B_QS_R_TO = Square::D8;

}  // namespace

Position::Position() { set_startpos(); }

void Position::clear() noexcept {
    board_.fill(Piece::None);
    color_bb_.fill(0ULL);
    piece_type_bb_.fill(0ULL);
    side_to_move_ = Color::White;
    castling_rights_ = CastlingRights::None;
    en_passant_sq_ = Square::None;
    halfmove_clock_ = 0;
    fullmove_number_ = 1;
    zobrist_key_ = 0ULL;
}

void Position::put_piece(Piece p, Square sq) noexcept {
    board_[static_cast<uint8_t>(sq)] = p;
    set_bit(color_bb_[static_cast<uint8_t>(color_of(p))], sq);
    set_bit(piece_type_bb_[static_cast<uint8_t>(type_of(p))], sq);
}

void Position::remove_piece(Square sq) noexcept {
    const Piece p = board_[static_cast<uint8_t>(sq)];
    if (p == Piece::None) return;
    board_[static_cast<uint8_t>(sq)] = Piece::None;
    clear_bit(color_bb_[static_cast<uint8_t>(color_of(p))], sq);
    clear_bit(piece_type_bb_[static_cast<uint8_t>(type_of(p))], sq);
}

Bitboard Position::pieces(PieceType pt) const noexcept {
    return piece_type_bb_[static_cast<uint8_t>(pt)];
}

Bitboard Position::pieces(Color c, PieceType pt) const noexcept {
    return pieces(c) & pieces(pt);
}

Key Position::compute_zobrist() const noexcept {
    Key k = 0ULL;
    for (uint8_t sq = 0; sq < 64; ++sq) {
        if (board_[sq] != Piece::None) k ^= zobrist::piece(board_[sq], static_cast<Square>(sq));
    }
    k ^= zobrist::castling(castling_rights_);
    if (en_passant_sq_ != Square::None) k ^= zobrist::en_passant(file_of(en_passant_sq_));
    if (side_to_move_ == Color::Black) k ^= zobrist::side_to_move();
    return k;
}

void Position::set_startpos() { set_fen(START_FEN); }

bool Position::set_fen(std::string_view fen, std::string* error) {
    std::istringstream iss(std::string{fen});
    std::string placement, active, castling, ep, halfmove = "0", fullmove = "1";
    if (!(iss >> placement >> active >> castling >> ep)) {
        if (error) *error = "Incomplete FEN string";
        return false;
    }
    iss >> halfmove >> fullmove;
    clear();
    int r = 7, f = 0;
    for (char ch : placement) {
        if (ch == '/') {
            if (f != 8 || --r < 0) { if (error) *error = "Invalid rank in FEN"; return false; }
            f = 0;
        } else if (ch >= '1' && ch <= '8') {
            f += ch - '0';
            if (f > 8) { if (error) *error = "File overflow in FEN"; return false; }
        } else {
            auto piece = char_to_piece(ch);
            if (!piece || f >= 8 || r < 0) { if (error) *error = "Invalid piece in FEN"; return false; }
            put_piece(*piece, make_square(static_cast<File>(f++), static_cast<Rank>(r)));
        }
    }
    if (r != 0 || f != 8) { if (error) *error = "Incomplete board in FEN"; return false; }
    if (active == "w") side_to_move_ = Color::White;
    else if (active == "b") side_to_move_ = Color::Black;
    else { if (error) *error = "Invalid active color in FEN"; return false; }

    castling_rights_ = CastlingRights::None;
    if (castling != "-") {
        for (char c : castling) {
            if (c == 'K') castling_rights_ |= CastlingRights::WhiteOO;
            else if (c == 'Q') castling_rights_ |= CastlingRights::WhiteOOO;
            else if (c == 'k') castling_rights_ |= CastlingRights::BlackOO;
            else if (c == 'q') castling_rights_ |= CastlingRights::BlackOOO;
        }
    }
    if (ep == "-") en_passant_sq_ = Square::None;
    else {
        auto sq = string_to_square(ep);
        if (!sq) { if (error) *error = "Invalid en passant square"; return false; }
        en_passant_sq_ = *sq;
    }
    try {
        halfmove_clock_ = static_cast<uint16_t>(std::stoi(halfmove));
        fullmove_number_ = static_cast<uint16_t>(std::max(1, std::stoi(fullmove)));
    } catch (...) { halfmove_clock_ = 0; fullmove_number_ = 1; }
    zobrist_key_ = compute_zobrist();
    return true;
}

std::string Position::to_fen() const {
    std::string fen;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece p = piece_at(make_square(static_cast<File>(f), static_cast<Rank>(r)));
            if (p == Piece::None) ++empty;
            else {
                if (empty > 0) { fen += std::to_string(empty); empty = 0; }
                fen += piece_to_char(p);
            }
        }
        if (empty > 0) fen += std::to_string(empty);
        if (r > 0) fen += '/';
    }
    fen += (side_to_move_ == Color::White) ? " w " : " b ";
    if (castling_rights_ == CastlingRights::None) fen += "-";
    else {
        if (castling_rights_ & CastlingRights::WhiteOO)  fen += 'K';
        if (castling_rights_ & CastlingRights::WhiteOOO) fen += 'Q';
        if (castling_rights_ & CastlingRights::BlackOO)  fen += 'k';
        if (castling_rights_ & CastlingRights::BlackOOO) fen += 'q';
    }
    fen += " " + square_to_string(en_passant_sq_) + " ";
    fen += std::to_string(halfmove_clock_) + " " + std::to_string(fullmove_number_);
    return fen;
}

void Position::make_move(Move m, UndoInfo& undo) noexcept {
    const Square from = m.from_square(), to = m.to_square();
    const Piece mover = piece_at(from);
    const PieceType pt = type_of(mover);
    const Color us = side_to_move_;

    undo.captured_piece = piece_at(to);
    undo.castling_rights = castling_rights_;
    undo.en_passant_sq = en_passant_sq_;
    undo.halfmove_clock = halfmove_clock_;
    undo.zobrist_key = zobrist_key_;

    if (m.is_en_passant()) {
        const Square cap_sq = make_square(file_of(to), rank_of(from));
        undo.captured_piece = piece_at(cap_sq);
        remove_piece(cap_sq);
        zobrist_key_ ^= zobrist::piece(undo.captured_piece, cap_sq);
    } else if (undo.captured_piece != Piece::None) {
        remove_piece(to);
        zobrist_key_ ^= zobrist::piece(undo.captured_piece, to);
    }

    remove_piece(from);
    zobrist_key_ ^= zobrist::piece(mover, from);
    Piece placed = m.is_promotion() ? make_piece(us, m.promotion_type()) : mover;
    put_piece(placed, to);
    zobrist_key_ ^= zobrist::piece(placed, to);

    if (m.is_castling()) {
        Square r_from = Square::None, r_to = Square::None;
        if (to == Square::G1) { r_from = W_KS_R_FROM; r_to = W_KS_R_TO; }
        else if (to == Square::C1) { r_from = W_QS_R_FROM; r_to = W_QS_R_TO; }
        else if (to == Square::G8) { r_from = B_KS_R_FROM; r_to = B_KS_R_TO; }
        else if (to == Square::C8) { r_from = B_QS_R_FROM; r_to = B_QS_R_TO; }
        const Piece rook = make_piece(us, PieceType::Rook);
        remove_piece(r_from); put_piece(rook, r_to);
        zobrist_key_ ^= zobrist::piece(rook, r_from) ^ zobrist::piece(rook, r_to);
    }

    if (pt == PieceType::King) {
        castling_rights_ &= (us == Color::White) ? ~CastlingRights::WhiteAll : ~CastlingRights::BlackAll;
    }
    if (from == Square::H1 || to == Square::H1) castling_rights_ &= ~CastlingRights::WhiteOO;
    if (from == Square::A1 || to == Square::A1) castling_rights_ &= ~CastlingRights::WhiteOOO;
    if (from == Square::H8 || to == Square::H8) castling_rights_ &= ~CastlingRights::BlackOO;
    if (from == Square::A8 || to == Square::A8) castling_rights_ &= ~CastlingRights::BlackOOO;
    zobrist_key_ ^= zobrist::castling(undo.castling_rights) ^ zobrist::castling(castling_rights_);

    if (en_passant_sq_ != Square::None) {
        zobrist_key_ ^= zobrist::en_passant(file_of(en_passant_sq_));
        en_passant_sq_ = Square::None;
    }
    if (m.is_double_pawn_push()) {
        en_passant_sq_ = make_square(file_of(from), (us == Color::White) ? Rank::Rank3 : Rank::Rank6);
        zobrist_key_ ^= zobrist::en_passant(file_of(en_passant_sq_));
    }

    halfmove_clock_ = (pt == PieceType::Pawn || undo.captured_piece != Piece::None) ? 0 : halfmove_clock_ + 1;
    if (us == Color::Black) ++fullmove_number_;
    side_to_move_ = ~side_to_move_;
    zobrist_key_ ^= zobrist::side_to_move();
}

void Position::unmake_move(Move m, const UndoInfo& undo) noexcept {
    const Square from = m.from_square(), to = m.to_square();
    side_to_move_ = ~side_to_move_;
    const Color us = side_to_move_;

    const Piece mover = piece_at(to);
    remove_piece(to);
    put_piece(m.is_promotion() ? make_piece(us, PieceType::Pawn) : mover, from);

    if (m.is_en_passant()) {
        put_piece(undo.captured_piece, make_square(file_of(to), rank_of(from)));
    } else if (undo.captured_piece != Piece::None) {
        put_piece(undo.captured_piece, to);
    }

    if (m.is_castling()) {
        Square r_from = Square::None, r_to = Square::None;
        if (to == Square::G1) { r_from = W_KS_R_FROM; r_to = W_KS_R_TO; }
        else if (to == Square::C1) { r_from = W_QS_R_FROM; r_to = W_QS_R_TO; }
        else if (to == Square::G8) { r_from = B_KS_R_FROM; r_to = B_KS_R_TO; }
        else if (to == Square::C8) { r_from = B_QS_R_FROM; r_to = B_QS_R_TO; }
        const Piece rook = make_piece(us, PieceType::Rook);
        remove_piece(r_to); put_piece(rook, r_from);
    }

    if (us == Color::Black) --fullmove_number_;
    castling_rights_ = undo.castling_rights;
    en_passant_sq_ = undo.en_passant_sq;
    halfmove_clock_ = undo.halfmove_clock;
    zobrist_key_ = undo.zobrist_key;
}

void Position::make_null_move(UndoInfo& undo) noexcept {
    undo.castling_rights = castling_rights_;
    undo.en_passant_sq = en_passant_sq_;
    undo.halfmove_clock = halfmove_clock_;
    undo.zobrist_key = zobrist_key_;
    undo.captured_piece = Piece::None;

    if (en_passant_sq_ != Square::None) {
        zobrist_key_ ^= zobrist::en_passant(file_of(en_passant_sq_));
        en_passant_sq_ = Square::None;
    }
    ++halfmove_clock_;
    if (side_to_move_ == Color::Black) ++fullmove_number_;
    side_to_move_ = ~side_to_move_;
    zobrist_key_ ^= zobrist::side_to_move();
}

void Position::unmake_null_move(const UndoInfo& undo) noexcept {
    side_to_move_ = ~side_to_move_;
    if (side_to_move_ == Color::Black) --fullmove_number_;
    castling_rights_ = undo.castling_rights;
    en_passant_sq_ = undo.en_passant_sq;
    halfmove_clock_ = undo.halfmove_clock;
    zobrist_key_ = undo.zobrist_key;
}

}  // namespace lookahead::core
