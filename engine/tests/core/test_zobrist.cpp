#include <gtest/gtest.h>
#include <unordered_set>
#include "core/zobrist.h"

using namespace lookahead::core;

TEST(ZobristTest, InitializationDeterminism) {
    zobrist::init();
    Key k1 = zobrist::piece(Piece::WhiteKnight, Square::G1);
    Key k2 = zobrist::piece(Piece::WhiteKnight, Square::G1);
    EXPECT_NE(k1, 0ULL);
    EXPECT_EQ(k1, k2);

    Key side1 = zobrist::side_to_move();
    Key side2 = zobrist::side_to_move();
    EXPECT_NE(side1, 0ULL);
    EXPECT_EQ(side1, side2);
}

TEST(ZobristTest, KeyUniqueness) {
    zobrist::init();
    std::unordered_set<Key> keys;

    // Piece keys
    for (uint8_t p = 1; p <= 14; ++p) {
        if (p == 7 || p == 8) continue;  // unused piece enum indices
        for (uint8_t sq = 0; sq < 64; ++sq) {
            Key k = zobrist::piece(static_cast<Piece>(p), static_cast<Square>(sq));
            EXPECT_NE(k, 0ULL);
            EXPECT_TRUE(keys.insert(k).second) << "Duplicate key found for piece " << int(p) << " on square " << int(sq);
        }
    }

    // Castling keys
    for (uint8_t c = 0; c < 16; ++c) {
        Key k = zobrist::castling(c);
        EXPECT_NE(k, 0ULL);
        EXPECT_TRUE(keys.insert(k).second);
    }

    // En passant keys
    for (uint8_t f = 0; f < 8; ++f) {
        Key k = zobrist::en_passant(static_cast<File>(f));
        EXPECT_NE(k, 0ULL);
        EXPECT_TRUE(keys.insert(k).second);
    }

    // Side to move key
    EXPECT_TRUE(keys.insert(zobrist::side_to_move()).second);
}

TEST(ZobristTest, XorInvolutionProperty) {
    zobrist::init();
    Key base = 0xAAAAAAAAAAAAAAAAULL;

    Key piece_k = zobrist::piece(Piece::WhitePawn, Square::E2);
    Key modified = base ^ piece_k;
    EXPECT_NE(base, modified);

    Key restored = modified ^ piece_k;
    EXPECT_EQ(base, restored);
}

TEST(ZobristTest, IncrementalMoveSimulation) {
    zobrist::init();
    Key hash = 0ULL;

    // Place White Pawn on e2
    hash ^= zobrist::piece(Piece::WhitePawn, Square::E2);
    // Pawn moves e2 to e4
    hash ^= zobrist::piece(Piece::WhitePawn, Square::E2);  // remove from e2
    hash ^= zobrist::piece(Piece::WhitePawn, Square::E4);  // add to e4
    // Toggle side
    hash ^= zobrist::side_to_move();

    // Now unmake in reverse:
    hash ^= zobrist::side_to_move();
    hash ^= zobrist::piece(Piece::WhitePawn, Square::E4);
    hash ^= zobrist::piece(Piece::WhitePawn, Square::E2);

    // Hash must return exactly to the state before the move
    Key initial_hash = zobrist::piece(Piece::WhitePawn, Square::E2);
    EXPECT_EQ(hash, initial_hash);
}
