#include <gtest/gtest.h>
#include "movegen/attacks.h"

using namespace lookahead;
using namespace lookahead::core;

TEST(AttacksTest, PawnAttacks) {
    movegen::attacks::init();

    Bitboard w_e4 = movegen::attacks::pawn(Color::White, Square::E4);
    EXPECT_EQ(w_e4, square_bb(Square::D5) | square_bb(Square::F5));

    Bitboard b_e5 = movegen::attacks::pawn(Color::Black, Square::E5);
    EXPECT_EQ(b_e5, square_bb(Square::D4) | square_bb(Square::F4));

    // Edge pawns must not wrap around board
    Bitboard w_a2 = movegen::attacks::pawn(Color::White, Square::A2);
    EXPECT_EQ(w_a2, square_bb(Square::B3));

    Bitboard w_h2 = movegen::attacks::pawn(Color::White, Square::H2);
    EXPECT_EQ(w_h2, square_bb(Square::G3));
}

TEST(AttacksTest, KnightAttacks) {
    movegen::attacks::init();

    // Center knight has 8 targets
    Bitboard e4 = movegen::attacks::knight(Square::E4);
    EXPECT_EQ(popcount(e4), 8);
    EXPECT_TRUE(test_bit(e4, Square::D6));
    EXPECT_TRUE(test_bit(e4, Square::F6));
    EXPECT_TRUE(test_bit(e4, Square::C5));
    EXPECT_TRUE(test_bit(e4, Square::G5));
    EXPECT_TRUE(test_bit(e4, Square::C3));
    EXPECT_TRUE(test_bit(e4, Square::G3));
    EXPECT_TRUE(test_bit(e4, Square::D2));
    EXPECT_TRUE(test_bit(e4, Square::F2));

    // Corner knight has exactly 2 targets
    Bitboard a1 = movegen::attacks::knight(Square::A1);
    EXPECT_EQ(popcount(a1), 2);
    EXPECT_TRUE(test_bit(a1, Square::B3));
    EXPECT_TRUE(test_bit(a1, Square::C2));
}

TEST(AttacksTest, KingAttacks) {
    movegen::attacks::init();

    Bitboard e4 = movegen::attacks::king(Square::E4);
    EXPECT_EQ(popcount(e4), 8);

    Bitboard a1 = movegen::attacks::king(Square::A1);
    EXPECT_EQ(popcount(a1), 3);
    EXPECT_TRUE(test_bit(a1, Square::A2));
    EXPECT_TRUE(test_bit(a1, Square::B1));
    EXPECT_TRUE(test_bit(a1, Square::B2));
}

TEST(AttacksTest, RookMagicAttacks) {
    movegen::attacks::init();

    // Empty board: rook on e4 attacks 14 squares (7 on rank 4, 7 on file E)
    Bitboard empty_e4 = movegen::attacks::rook(Square::E4, BitboardEmpty);
    EXPECT_EQ(popcount(empty_e4), 14);

    // Rook with blockers
    Bitboard occ = square_bb(Square::E6) | square_bb(Square::E2) | square_bb(Square::C4);
    Bitboard blocked_e4 = movegen::attacks::rook(Square::E4, occ);

    // North ray hits e6 and stops (e7/e8 blocked)
    EXPECT_TRUE(test_bit(blocked_e4, Square::E5));
    EXPECT_TRUE(test_bit(blocked_e4, Square::E6));
    EXPECT_FALSE(test_bit(blocked_e4, Square::E7));
    EXPECT_FALSE(test_bit(blocked_e4, Square::E8));

    // West ray hits c4 and stops (b4/a4 blocked)
    EXPECT_TRUE(test_bit(blocked_e4, Square::D4));
    EXPECT_TRUE(test_bit(blocked_e4, Square::C4));
    EXPECT_FALSE(test_bit(blocked_e4, Square::B4));
    EXPECT_FALSE(test_bit(blocked_e4, Square::A4));
}

TEST(AttacksTest, BishopMagicAttacks) {
    movegen::attacks::init();

    // Empty board: bishop on e4 attacks 13 squares
    Bitboard empty_e4 = movegen::attacks::bishop(Square::E4, BitboardEmpty);
    EXPECT_EQ(popcount(empty_e4), 13);

    // Bishop with blockers
    Bitboard occ = square_bb(Square::G6);
    Bitboard blocked_e4 = movegen::attacks::bishop(Square::E4, occ);
    EXPECT_TRUE(test_bit(blocked_e4, Square::F5));
    EXPECT_TRUE(test_bit(blocked_e4, Square::G6));
    EXPECT_FALSE(test_bit(blocked_e4, Square::H7));  // behind blocker
}

TEST(AttacksTest, QueenAttacks) {
    movegen::attacks::init();

    Bitboard occ = square_bb(Square::E6);
    Bitboard q = movegen::attacks::queen(Square::E4, occ);
    Bitboard expected = movegen::attacks::bishop(Square::E4, occ) | movegen::attacks::rook(Square::E4, occ);
    EXPECT_EQ(q, expected);
}

TEST(AttacksTest, RaysBetweenAndLine) {
    movegen::attacks::init();

    // Between a1 and a8: squares a2..a7 (6 squares)
    Bitboard b = movegen::attacks::between(Square::A1, Square::A8);
    EXPECT_EQ(popcount(b), 6);
    EXPECT_TRUE(test_bit(b, Square::A2));
    EXPECT_TRUE(test_bit(b, Square::A7));
    EXPECT_FALSE(test_bit(b, Square::A1));
    EXPECT_FALSE(test_bit(b, Square::A8));

    // Non-collinear squares
    EXPECT_EQ(movegen::attacks::between(Square::A1, Square::B3), BitboardEmpty);

    // Line ray
    Bitboard l = movegen::attacks::line(Square::A1, Square::H8);
    EXPECT_EQ(popcount(l), 8);
    EXPECT_TRUE(test_bit(l, Square::D4));
}
