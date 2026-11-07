/*
 * What: Fundamental chess domain types, enums, square coordinates, and piece definitions.
 * Why:  Establishes strongly typed primitive representations used throughout the engine.
 * Layer: Layer 0 (core) - depends only on standard C++ headers.
 */

#ifndef LOOKAHEAD_CORE_TYPES_H
#define LOOKAHEAD_CORE_TYPES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

namespace lookahead::core {

// Two playing sides in chess, with a sentinel None value.
enum class Color : uint8_t {
    White = 0,
    Black = 1,
    None  = 2
};

// Flips the color between White and Black.
[[nodiscard]] constexpr Color operator~(Color c) noexcept {
    return static_cast<Color>(static_cast<uint8_t>(c) ^ 1);
}

// Chess piece categories independent of color.
enum class PieceType : uint8_t {
    None   = 0,
    Pawn   = 1,
    Knight = 2,
    Bishop = 3,
    Rook   = 4,
    Queen  = 5,
    King   = 6
};

// Distinct colored piece representations.
enum class Piece : uint8_t {
    None        = 0,
    WhitePawn   = 1,
    WhiteKnight = 2,
    WhiteBishop = 3,
    WhiteRook   = 4,
    WhiteQueen  = 5,
    WhiteKing   = 6,
    BlackPawn   = 9,
    BlackKnight = 10,
    BlackBishop = 11,
    BlackRook   = 12,
    BlackQueen  = 13,
    BlackKing   = 14
};

// Combines color and piece type into a colored Piece.
[[nodiscard]] constexpr Piece make_piece(Color c, PieceType pt) noexcept {
    if (pt == PieceType::None || c == Color::None) {
        return Piece::None;
    }
    const uint8_t offset = (c == Color::White) ? 0 : 8;
    return static_cast<Piece>(offset + static_cast<uint8_t>(pt));
}

// Extracts the side owning the piece.
[[nodiscard]] constexpr Color color_of(Piece p) noexcept {
    if (p == Piece::None) {
        return Color::None;
    }
    return (static_cast<uint8_t>(p) < 8) ? Color::White : Color::Black;
}

// Extracts the uncolored piece category.
[[nodiscard]] constexpr PieceType type_of(Piece p) noexcept {
    return static_cast<PieceType>(static_cast<uint8_t>(p) & 0x07);
}

// 64 board squares mapped Little-Endian Rank-File (A1 = 0, H8 = 63).
enum class Square : uint8_t {
    A1 = 0,  B1, C1, D1, E1, F1, G1, H1,
    A2 = 8,  B2, C2, D2, E2, F2, G2, H2,
    A3 = 16, B3, C3, D3, E3, F3, G3, H3,
    A4 = 24, B4, C4, D4, E4, F4, G4, H4,
    A5 = 32, B5, C5, D5, E5, F5, G5, H5,
    A6 = 40, B6, C6, D6, E6, F6, G6, H6,
    A7 = 48, B7, C7, D7, E7, F7, G7, H7,
    A8 = 56, B8, C8, D8, E8, F8, G8, H8,
    None = 64
};

enum class File : uint8_t {
    FileA = 0, FileB, FileC, FileD, FileE, FileF, FileG, FileH
};

enum class Rank : uint8_t {
    Rank1 = 0, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8
};

// Constructs a square index from file and rank coordinates.
[[nodiscard]] constexpr Square make_square(File f, Rank r) noexcept {
    return static_cast<Square>((static_cast<uint8_t>(r) * 8) + static_cast<uint8_t>(f));
}

// Extracts the rank component of a square.
[[nodiscard]] constexpr Rank rank_of(Square sq) noexcept {
    return static_cast<Rank>(static_cast<uint8_t>(sq) / 8);
}

// Extracts the file component of a square.
[[nodiscard]] constexpr File file_of(Square sq) noexcept {
    return static_cast<File>(static_cast<uint8_t>(sq) % 8);
}

// Checks if a square is within valid board boundaries [A1, H8].
[[nodiscard]] constexpr bool is_valid_square(Square sq) noexcept {
    return static_cast<uint8_t>(sq) < 64;
}

// Converts a square to standard algebraic notation (e.g. "e4").
[[nodiscard]] inline std::string square_to_string(Square sq) {
    if (!is_valid_square(sq)) {
        return "-";
    }
    std::string s;
    s += static_cast<char>('a' + static_cast<uint8_t>(file_of(sq)));
    s += static_cast<char>('1' + static_cast<uint8_t>(rank_of(sq)));
    return s;
}

// Parses standard algebraic notation into a Square.
[[nodiscard]] inline std::optional<Square> string_to_square(std::string_view sv) noexcept {
    if (sv.size() != 2) {
        return std::nullopt;
    }
    if (sv[0] < 'a' || sv[0] > 'h' || sv[1] < '1' || sv[1] > '8') {
        return std::nullopt;
    }
    const File f = static_cast<File>(sv[0] - 'a');
    const Rank r = static_cast<Rank>(sv[1] - '1');
    return make_square(f, r);
}

// Bitmask representing available castling privileges.
namespace CastlingRights {
    inline constexpr uint8_t None     = 0;
    inline constexpr uint8_t WhiteOO  = 1 << 0;  // Kingside
    inline constexpr uint8_t WhiteOOO = 1 << 1;  // Queenside
    inline constexpr uint8_t BlackOO  = 1 << 2;  // Kingside
    inline constexpr uint8_t BlackOOO = 1 << 3;  // Queenside
    inline constexpr uint8_t WhiteAll = WhiteOO | WhiteOOO;
    inline constexpr uint8_t BlackAll = BlackOO | BlackOOO;
    inline constexpr uint8_t All      = WhiteAll | BlackAll;
}

// Standard evaluation scores in centipawns.
using Value = int32_t;
inline constexpr Value ValueZero     = 0;
inline constexpr Value ValueDraw     = 0;
inline constexpr Value ValueInfinite = 30000;
inline constexpr Value ValueMate     = 29000;
inline constexpr Value ValueMateMax  = ValueMate - 256;

}  // namespace lookahead::core

#endif  // LOOKAHEAD_CORE_TYPES_H
