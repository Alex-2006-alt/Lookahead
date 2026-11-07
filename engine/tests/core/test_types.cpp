#include <gtest/gtest.h>
#include "core/types.h"

using namespace lookahead::core;

TEST(TypesTest, ColorInversion) {
    EXPECT_EQ(~Color::White, Color::Black);
    EXPECT_EQ(~Color::Black, Color::White);
}

TEST(TypesTest, PieceCreationAndDeconstruction) {
    Piece wp = make_piece(Color::White, PieceType::Pawn);
    EXPECT_EQ(wp, Piece::WhitePawn);
    EXPECT_EQ(color_of(wp), Color::White);
    EXPECT_EQ(type_of(wp), PieceType::Pawn);

    Piece bk = make_piece(Color::Black, PieceType::King);
    EXPECT_EQ(bk, Piece::BlackKing);
    EXPECT_EQ(color_of(bk), Color::Black);
    EXPECT_EQ(type_of(bk), PieceType::King);

    Piece bq = make_piece(Color::Black, PieceType::Queen);
    EXPECT_EQ(color_of(bq), Color::Black);
    EXPECT_EQ(type_of(bq), PieceType::Queen);
}

TEST(TypesTest, SquareCoordinates) {
    EXPECT_EQ(make_square(File::FileA, Rank::Rank1), Square::A1);
    EXPECT_EQ(make_square(File::FileE, Rank::Rank4), Square::E4);
    EXPECT_EQ(make_square(File::FileH, Rank::Rank8), Square::H8);

    EXPECT_EQ(file_of(Square::E4), File::FileE);
    EXPECT_EQ(rank_of(Square::E4), Rank::Rank4);

    EXPECT_TRUE(is_valid_square(Square::A1));
    EXPECT_TRUE(is_valid_square(Square::H8));
    EXPECT_FALSE(is_valid_square(Square::None));
}

TEST(TypesTest, SquareStringConversions) {
    EXPECT_EQ(square_to_string(Square::E4), "e4");
    EXPECT_EQ(square_to_string(Square::A1), "a1");
    EXPECT_EQ(square_to_string(Square::H8), "h8");
    EXPECT_EQ(square_to_string(Square::None), "-");

    EXPECT_EQ(string_to_square("e4"), Square::E4);
    EXPECT_EQ(string_to_square("a1"), Square::A1);
    EXPECT_EQ(string_to_square("h8"), Square::H8);
    EXPECT_EQ(string_to_square("z9"), std::nullopt);
    EXPECT_EQ(string_to_square("e"), std::nullopt);
    EXPECT_EQ(string_to_square("e44"), std::nullopt);
}

TEST(TypesTest, CastlingMasks) {
    EXPECT_EQ(CastlingRights::WhiteAll, CastlingRights::WhiteOO | CastlingRights::WhiteOOO);
    EXPECT_EQ(CastlingRights::BlackAll, CastlingRights::BlackOO | CastlingRights::BlackOOO);
    EXPECT_EQ(CastlingRights::All, 15);
}
