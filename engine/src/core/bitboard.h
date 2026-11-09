/*
 * What: 64-bit integer bitboard operations, board masks, and bitwise hardware primitives.
 * Why:  Enables parallel board computation and constant-time piece scanning.
 * Layer: Layer 0 (core) - depends only on types.h and standard C++ headers.
 */

#ifndef LOOKAHEAD_CORE_BITBOARD_H
#define LOOKAHEAD_CORE_BITBOARD_H

#include <bit>
#include <cstdint>
#include <string>
#include "types.h"

namespace lookahead::core {

using Bitboard = uint64_t;

inline constexpr Bitboard BitboardEmpty = 0ULL;
inline constexpr Bitboard BitboardAll   = ~0ULL;

// File bitboards
inline constexpr Bitboard FileABB = 0x0101010101010101ULL;
inline constexpr Bitboard FileBBB = FileABB << 1;
inline constexpr Bitboard FileCBB = FileABB << 2;
inline constexpr Bitboard FileDBB = FileABB << 3;
inline constexpr Bitboard FileEBB = FileABB << 4;
inline constexpr Bitboard FileFBB = FileABB << 5;
inline constexpr Bitboard FileGBB = FileABB << 6;
inline constexpr Bitboard FileHBB = FileABB << 7;

// Rank bitboards
inline constexpr Bitboard Rank1BB = 0x00000000000000FFULL;
inline constexpr Bitboard Rank2BB = Rank1BB << 8;
inline constexpr Bitboard Rank3BB = Rank1BB << 16;
inline constexpr Bitboard Rank4BB = Rank1BB << 24;
inline constexpr Bitboard Rank5BB = Rank1BB << 32;
inline constexpr Bitboard Rank6BB = Rank1BB << 40;
inline constexpr Bitboard Rank7BB = Rank1BB << 48;
inline constexpr Bitboard Rank8BB = Rank1BB << 56;

// Converts a file to an entire column bitboard.
[[nodiscard]] constexpr Bitboard file_bb(File f) noexcept {
    return FileABB << static_cast<uint8_t>(f);
}

// Converts a rank to an entire row bitboard.
[[nodiscard]] constexpr Bitboard rank_bb(Rank r) noexcept {
    return Rank1BB << (8 * static_cast<uint8_t>(r));
}

// Converts a square to a single-bit bitboard.
[[nodiscard]] constexpr Bitboard square_bb(Square sq) noexcept {
    return 1ULL << static_cast<uint8_t>(sq);
}

// Tests whether a bit is set at a given square.
[[nodiscard]] constexpr bool test_bit(Bitboard bb, Square sq) noexcept {
    return (bb & square_bb(sq)) != 0ULL;
}

// Sets the bit corresponding to a square.
constexpr void set_bit(Bitboard& bb, Square sq) noexcept {
    bb |= square_bb(sq);
}

// Clears the bit corresponding to a square.
constexpr void clear_bit(Bitboard& bb, Square sq) noexcept {
    bb &= ~square_bb(sq);
}

// Toggles the bit corresponding to a square.
constexpr void toggle_bit(Bitboard& bb, Square sq) noexcept {
    bb ^= square_bb(sq);
}

// Counts the number of active bits (set bits) using hardware popcount.
[[nodiscard]] constexpr int popcount(Bitboard bb) noexcept {
    return std::popcount(bb);
}

// Identifies the least significant active bit (LSB) in O(1).
[[nodiscard]] constexpr Square lsb(Bitboard bb) noexcept {
    return static_cast<Square>(std::countr_zero(bb));
}

// Clears the lowest active bit and returns its square index.
constexpr Square pop_lsb(Bitboard& bb) noexcept {
    const Square s = lsb(bb);
    bb &= bb - 1ULL;
    return s;
}

// Ray directional shift operations clipped to board edges
[[nodiscard]] constexpr Bitboard shift_north(Bitboard bb) noexcept {
    return bb << 8;
}

[[nodiscard]] constexpr Bitboard shift_south(Bitboard bb) noexcept {
    return bb >> 8;
}

[[nodiscard]] constexpr Bitboard shift_east(Bitboard bb) noexcept {
    return (bb & ~FileHBB) << 1;
}

[[nodiscard]] constexpr Bitboard shift_west(Bitboard bb) noexcept {
    return (bb & ~FileABB) >> 1;
}

[[nodiscard]] constexpr Bitboard shift_north_east(Bitboard bb) noexcept {
    return (bb & ~FileHBB) << 9;
}

[[nodiscard]] constexpr Bitboard shift_north_west(Bitboard bb) noexcept {
    return (bb & ~FileABB) << 7;
}

[[nodiscard]] constexpr Bitboard shift_south_east(Bitboard bb) noexcept {
    return (bb & ~FileHBB) >> 7;
}

[[nodiscard]] constexpr Bitboard shift_south_west(Bitboard bb) noexcept {
    return (bb & ~FileABB) >> 9;
}

// Formats a bitboard as an 8x8 ASCII diagram for debugging.
[[nodiscard]] inline std::string bitboard_to_string(Bitboard bb) {
    std::string result;
    for (int r = 7; r >= 0; --r) {
        result += static_cast<char>('1' + r);
        result += " ";
        for (int f = 0; f < 8; ++f) {
            const Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            result += test_bit(bb, sq) ? "1 " : ". ";
        }
        result += "\n";
    }
    result += "  a b c d e f g h\n";
    return result;
}

}  // namespace lookahead::core

#endif  // LOOKAHEAD_CORE_BITBOARD_H
