#include <gtest/gtest.h>
#include "core/bitboard.h"

using namespace lookahead::core;

TEST(BitboardTest, BitManipulation) {
    Bitboard bb = BitboardEmpty;
    EXPECT_EQ(bb, 0ULL);
    EXPECT_EQ(popcount(bb), 0);

    set_bit(bb, Square::E4);
    EXPECT_TRUE(test_bit(bb, Square::E4));
    EXPECT_FALSE(test_bit(bb, Square::D4));
    EXPECT_EQ(popcount(bb), 1);
    EXPECT_EQ(lsb(bb), Square::E4);

    toggle_bit(bb, Square::E4);
    EXPECT_FALSE(test_bit(bb, Square::E4));
    EXPECT_EQ(popcount(bb), 0);

    set_bit(bb, Square::A1);
    set_bit(bb, Square::H8);
    EXPECT_EQ(popcount(bb), 2);
    clear_bit(bb, Square::A1);
    EXPECT_FALSE(test_bit(bb, Square::A1));
    EXPECT_TRUE(test_bit(bb, Square::H8));
}

TEST(BitboardTest, PopLsbIteration) {
    Bitboard bb = square_bb(Square::B2) | square_bb(Square::C3) | square_bb(Square::F6);
    EXPECT_EQ(popcount(bb), 3);

    Square s1 = pop_lsb(bb);
    EXPECT_EQ(s1, Square::B2);
    EXPECT_EQ(popcount(bb), 2);

    Square s2 = pop_lsb(bb);
    EXPECT_EQ(s2, Square::C3);
    EXPECT_EQ(popcount(bb), 1);

    Square s3 = pop_lsb(bb);
    EXPECT_EQ(s3, Square::F6);
    EXPECT_EQ(popcount(bb), 0);
    EXPECT_EQ(bb, BitboardEmpty);
}

TEST(BitboardTest, RayShifts) {
    Bitboard e4 = square_bb(Square::E4);

    EXPECT_EQ(shift_north(e4), square_bb(Square::E5));
    EXPECT_EQ(shift_south(e4), square_bb(Square::E3));
    EXPECT_EQ(shift_east(e4), square_bb(Square::F4));
    EXPECT_EQ(shift_west(e4), square_bb(Square::D4));

    EXPECT_EQ(shift_north_east(e4), square_bb(Square::F5));
    EXPECT_EQ(shift_north_west(e4), square_bb(Square::D5));
    EXPECT_EQ(shift_south_east(e4), square_bb(Square::F3));
    EXPECT_EQ(shift_south_west(e4), square_bb(Square::D3));
}

TEST(BitboardTest, ShiftEdgeClipping) {
    Bitboard h4 = square_bb(Square::H4);
    // Shifting east from H file must not wrap into A file
    EXPECT_EQ(shift_east(h4), BitboardEmpty);
    EXPECT_EQ(shift_north_east(h4), BitboardEmpty);
    EXPECT_EQ(shift_south_east(h4), BitboardEmpty);

    Bitboard a4 = square_bb(Square::A4);
    // Shifting west from A file must not wrap into H file
    EXPECT_EQ(shift_west(a4), BitboardEmpty);
    EXPECT_EQ(shift_north_west(a4), BitboardEmpty);
    EXPECT_EQ(shift_south_west(a4), BitboardEmpty);

    Bitboard e8 = square_bb(Square::E8);
    EXPECT_EQ(shift_north(e8), BitboardEmpty);

    Bitboard e1 = square_bb(Square::E1);
    EXPECT_EQ(shift_south(e1), BitboardEmpty);
}
