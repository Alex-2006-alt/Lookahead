#include <gtest/gtest.h>
#include "core/move.h"

using namespace lookahead::core;

TEST(MoveTest, QuietMoveCreation) {
    Move m(Square::E2, Square::E4, MoveFlag::DoublePawnPush);
    EXPECT_EQ(m.from_square(), Square::E2);
    EXPECT_EQ(m.to_square(), Square::E4);
    EXPECT_TRUE(m.is_double_pawn_push());
    EXPECT_FALSE(m.is_capture());
    EXPECT_FALSE(m.is_promotion());
    EXPECT_FALSE(m.is_castling());
    EXPECT_EQ(m.to_uci(), "e2e4");
}

TEST(MoveTest, NormalCapture) {
    Move m(Square::D4, Square::E5, MoveFlag::Capture);
    EXPECT_EQ(m.from_square(), Square::D4);
    EXPECT_EQ(m.to_square(), Square::E5);
    EXPECT_TRUE(m.is_capture());
    EXPECT_FALSE(m.is_en_passant());
    EXPECT_FALSE(m.is_promotion());
    EXPECT_EQ(m.to_uci(), "d4e5");
}

TEST(MoveTest, EnPassantCapture) {
    Move m(Square::E5, Square::D6, MoveFlag::EnPassant);
    EXPECT_TRUE(m.is_en_passant());
    EXPECT_TRUE(m.is_capture());
    EXPECT_FALSE(m.is_promotion());
    EXPECT_EQ(m.to_uci(), "e5d6");
}

TEST(MoveTest, CastlingMoves) {
    Move kCastle(Square::E1, Square::G1, MoveFlag::KingCastle);
    EXPECT_TRUE(kCastle.is_castling());
    EXPECT_FALSE(kCastle.is_capture());
    EXPECT_EQ(kCastle.to_uci(), "e1g1");

    Move qCastle(Square::E1, Square::C1, MoveFlag::QueenCastle);
    EXPECT_TRUE(qCastle.is_castling());
    EXPECT_EQ(qCastle.to_uci(), "e1c1");
}

TEST(MoveTest, PromotionMoves) {
    Move qPromo(Square::E7, Square::E8, MoveFlag::QueenPromotion);
    EXPECT_TRUE(qPromo.is_promotion());
    EXPECT_FALSE(qPromo.is_capture());
    EXPECT_EQ(qPromo.promotion_type(), PieceType::Queen);
    EXPECT_EQ(qPromo.to_uci(), "e7e8q");

    Move nPromoCapture(Square::E7, Square::F8, MoveFlag::KnightPromoCapture);
    EXPECT_TRUE(nPromoCapture.is_promotion());
    EXPECT_TRUE(nPromoCapture.is_capture());
    EXPECT_EQ(nPromoCapture.promotion_type(), PieceType::Knight);
    EXPECT_EQ(nPromoCapture.to_uci(), "e7f8n");

    Move rPromo(Square::A7, Square::A8, MoveFlag::RookPromotion);
    EXPECT_EQ(rPromo.promotion_type(), PieceType::Rook);
    EXPECT_EQ(rPromo.to_uci(), "a7a8r");

    Move bPromo(Square::B7, Square::B8, MoveFlag::BishopPromotion);
    EXPECT_EQ(bPromo.promotion_type(), PieceType::Bishop);
    EXPECT_EQ(bPromo.to_uci(), "b7b8b");
}

TEST(MoveTest, NullMove) {
    Move nullMove = Move::null();
    EXPECT_TRUE(nullMove.is_null());
    EXPECT_EQ(nullMove.to_uci(), "0000");
}
