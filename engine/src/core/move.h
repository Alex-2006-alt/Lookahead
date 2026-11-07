/*
 * What: 16-bit compact chess move encoding and decoding class.
 * Why:  Maximizes CPU cache density and eliminates heap overhead in move lists.
 * Layer: Layer 0 (core) - depends only on types.h and standard C++ headers.
 */

#ifndef LOOKAHEAD_CORE_MOVE_H
#define LOOKAHEAD_CORE_MOVE_H

#include <cstdint>
#include <string>
#include "types.h"

namespace lookahead::core {

// 4-bit move classification flags (stored in bits 12..15)
enum class MoveFlag : uint16_t {
    Quiet                = 0,
    DoublePawnPush       = 1,
    KingCastle           = 2,
    QueenCastle          = 3,
    Capture              = 4,
    EnPassant            = 5,
    KnightPromotion      = 8,
    BishopPromotion      = 9,
    RookPromotion        = 10,
    QueenPromotion       = 11,
    KnightPromoCapture   = 12,
    BishopPromoCapture   = 13,
    RookPromoCapture     = 14,
    QueenPromoCapture    = 15
};

class Move {
public:
    constexpr Move() noexcept : data_(0) {}
    constexpr explicit Move(uint16_t raw) noexcept : data_(raw) {}

    // Constructs a move from origin, destination, and behavioral flags.
    constexpr Move(Square from, Square to, MoveFlag flag = MoveFlag::Quiet) noexcept
        : data_(static_cast<uint16_t>(from) |
               (static_cast<uint16_t>(to) << 6) |
               (static_cast<uint16_t>(flag) << 12)) {}

    [[nodiscard]] constexpr uint16_t raw() const noexcept { return data_; }
    [[nodiscard]] constexpr bool is_null() const noexcept { return data_ == 0; }

    [[nodiscard]] constexpr Square from_square() const noexcept {
        return static_cast<Square>(data_ & 0x3F);
    }

    [[nodiscard]] constexpr Square to_square() const noexcept {
        return static_cast<Square>((data_ >> 6) & 0x3F);
    }

    [[nodiscard]] constexpr MoveFlag flag() const noexcept {
        return static_cast<MoveFlag>((data_ >> 12) & 0x0F);
    }

    // Indicates whether the move captures an opposing piece.
    [[nodiscard]] constexpr bool is_capture() const noexcept {
        return (static_cast<uint16_t>(flag()) & 0x04) != 0;
    }

    // Indicates whether the move promotes a pawn to another piece.
    [[nodiscard]] constexpr bool is_promotion() const noexcept {
        return (static_cast<uint16_t>(flag()) & 0x08) != 0;
    }

    // Indicates whether the move is kingside or queenside castling.
    [[nodiscard]] constexpr bool is_castling() const noexcept {
        const MoveFlag f = flag();
        return f == MoveFlag::KingCastle || f == MoveFlag::QueenCastle;
    }

    // Indicates an en passant pawn capture.
    [[nodiscard]] constexpr bool is_en_passant() const noexcept {
        return flag() == MoveFlag::EnPassant;
    }

    // Indicates a two-square initial pawn advance.
    [[nodiscard]] constexpr bool is_double_pawn_push() const noexcept {
        return flag() == MoveFlag::DoublePawnPush;
    }

    // Extracts the promoted piece type if this is a promotion move.
    [[nodiscard]] constexpr PieceType promotion_type() const noexcept {
        if (!is_promotion()) {
            return PieceType::None;
        }
        // Offset: Knight=2, Bishop=3, Rook=4, Queen=5
        const uint8_t promo_index = static_cast<uint16_t>(flag()) & 0x03;
        return static_cast<PieceType>(static_cast<uint8_t>(PieceType::Knight) + promo_index);
    }

    // Formats the move as standard UCI notation (e.g. "e2e4", "e7e8q").
    [[nodiscard]] inline std::string to_uci() const {
        if (is_null()) {
            return "0000";
        }
        std::string uci = square_to_string(from_square()) + square_to_string(to_square());
        if (is_promotion()) {
            switch (promotion_type()) {
                case PieceType::Knight: uci += 'n'; break;
                case PieceType::Bishop: uci += 'b'; break;
                case PieceType::Rook:   uci += 'r'; break;
                case PieceType::Queen:  uci += 'q'; break;
                default: break;
            }
        }
        return uci;
    }

    constexpr bool operator==(const Move& other) const noexcept { return data_ == other.data_; }
    constexpr bool operator!=(const Move& other) const noexcept { return data_ != other.data_; }

    static constexpr Move null() noexcept { return Move(0); }

private:
    uint16_t data_;
};

}  // namespace lookahead::core

#endif  // LOOKAHEAD_CORE_MOVE_H
