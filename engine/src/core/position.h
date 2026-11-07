/*
 * What: Complete chess position state, FEN parser/serializer, and make/unmake methods.
 * Why:  Serves as the central state machine for board representation and move exploration.
 * Layer: Layer 0 (core) - depends only on core types, bitboard, move, and zobrist.
 */

#ifndef LOOKAHEAD_CORE_POSITION_H
#define LOOKAHEAD_CORE_POSITION_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include "types.h"
#include "bitboard.h"
#include "move.h"
#include "zobrist.h"

namespace lookahead::core {

// Stores state that cannot be deduced from move alone for O(1) unmake.
struct UndoInfo {
    Piece captured_piece = Piece::None;
    uint8_t castling_rights = 0;
    Square en_passant_sq = Square::None;
    uint16_t halfmove_clock = 0;
    Key zobrist_key = 0ULL;
};

class Position {
public:
    Position();

    // Resets board to standard starting chess layout.
    void set_startpos();

    // Sets board from a FEN string with validation. Returns false on invalid FEN.
    bool set_fen(std::string_view fen, std::string* error = nullptr);

    // Exports the current board position as standard FEN.
    [[nodiscard]] std::string to_fen() const;

    // Piece and color occupancy queries
    [[nodiscard]] Piece piece_at(Square sq) const noexcept { return board_[static_cast<uint8_t>(sq)]; }
    [[nodiscard]] Bitboard pieces(Color c) const noexcept { return color_bb_[static_cast<uint8_t>(c)]; }
    [[nodiscard]] Bitboard pieces(PieceType pt) const noexcept;
    [[nodiscard]] Bitboard pieces(Color c, PieceType pt) const noexcept;
    [[nodiscard]] Bitboard occupied() const noexcept { return color_bb_[0] | color_bb_[1]; }
    [[nodiscard]] Bitboard empty_squares() const noexcept { return ~occupied(); }

    [[nodiscard]] Color side_to_move() const noexcept { return side_to_move_; }
    [[nodiscard]] uint8_t castling_rights() const noexcept { return castling_rights_; }
    [[nodiscard]] Square en_passant_sq() const noexcept { return en_passant_sq_; }
    [[nodiscard]] uint16_t halfmove_clock() const noexcept { return halfmove_clock_; }
    [[nodiscard]] uint16_t fullmove_number() const noexcept { return fullmove_number_; }
    [[nodiscard]] Key zobrist_key() const noexcept { return zobrist_key_; }

    // Recomputes Zobrist key from scratch across all pieces and board state.
    [[nodiscard]] Key compute_zobrist() const noexcept;

    // Executes a move, updating bitboards and Zobrist key incrementally.
    void make_move(Move m, UndoInfo& undo) noexcept;

    // Restores previous board state and Zobrist key from UndoInfo.
    void unmake_move(Move m, const UndoInfo& undo) noexcept;

    // Executes a null move (turn pass) for null-move pruning.
    void make_null_move(UndoInfo& undo) noexcept;
    void unmake_null_move(const UndoInfo& undo) noexcept;

    // Starting FEN constant
    static constexpr std::string_view START_FEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

private:
    void clear() noexcept;
    void put_piece(Piece p, Square sq) noexcept;
    void remove_piece(Square sq) noexcept;

    std::array<Piece, 64> board_{};
    std::array<Bitboard, 2> color_bb_{0ULL, 0ULL};
    std::array<Bitboard, 7> piece_type_bb_{0ULL};

    Color side_to_move_ = Color::White;
    uint8_t castling_rights_ = CastlingRights::None;
    Square en_passant_sq_ = Square::None;
    uint16_t halfmove_clock_ = 0;
    uint16_t fullmove_number_ = 1;
    Key zobrist_key_ = 0ULL;
};

}  // namespace lookahead::core

#endif  // LOOKAHEAD_CORE_POSITION_H
