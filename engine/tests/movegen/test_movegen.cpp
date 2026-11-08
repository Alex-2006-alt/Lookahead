#include <gtest/gtest.h>
#include "movegen/movegen.h"
#include "movegen/attacks.h"

using namespace lookahead;
using namespace lookahead::core;
using namespace lookahead::movegen;

class MovegenTest : public ::testing::Test {
protected:
    void SetUp() override {
        movegen::attacks::init();
    }
};

TEST_F(MovegenTest, InitialPositionLegalMovesCount) {
    Position pos;
    MoveList list;
    generate_legal_moves(pos, list);
    // Standard chess starting position has exactly 20 legal moves
    EXPECT_EQ(list.size(), 20);
    EXPECT_FALSE(in_check(pos));
    EXPECT_FALSE(is_checkmate(pos));
    EXPECT_FALSE(is_stalemate(pos));
}

TEST_F(MovegenTest, AbsolutePinRestrictsMovement) {
    Position pos;
    // White King on e1, White Knight on e2, Black Rook on e8 -> Knight is absolutely pinned
    pos.set_fen("4r3/8/8/8/8/8/4N3/4K3 w - - 0 1");
    MoveList list;
    generate_legal_moves(pos, list);

    // King can move to d1, f1, d2, f2. Pinned knight on e2 cannot move!
    for (size_t i = 0; i < list.size(); ++i) {
        EXPECT_NE(list[i].from_square(), Square::E2) << "Pinned knight should not have legal moves!";
    }
}

TEST_F(MovegenTest, PinnedRookCanMoveAlongPinRay) {
    Position pos;
    // White King on e1, White Rook on e3, Black Rook on e8 -> White Rook can move along e-file
    pos.set_fen("4r3/8/8/8/8/4R3/8/4K3 w - - 0 1");
    MoveList list;
    generate_legal_moves(pos, list);

    bool rook_moved_along_file = false;
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].from_square() == Square::E3) {
            EXPECT_EQ(file_of(list[i].to_square()), File::FileE);
            rook_moved_along_file = true;
        }
    }
    EXPECT_TRUE(rook_moved_along_file);
}

TEST_F(MovegenTest, DoubleCheckAllowsOnlyKingMoves) {
    Position pos;
    // White King on e1 in double check by Black Rook on e8 and Black Bishop on a5
    pos.set_fen("4r3/8/8/b7/8/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(in_check(pos));

    MoveList list;
    generate_legal_moves(pos, list);

    // In double check, every legal move must be a king move
    EXPECT_GT(list.size(), 0);
    for (size_t i = 0; i < list.size(); ++i) {
        EXPECT_EQ(list[i].from_square(), Square::E1);
    }
}

TEST_F(MovegenTest, CastlingLegalityChecks) {
    Position pos;
    // Standard castling available
    pos.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    MoveList list;
    generate_legal_moves(pos, list);

    bool has_ks_castle = false, has_qs_castle = false;
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].is_castling()) {
            if (list[i].to_square() == Square::G1) has_ks_castle = true;
            if (list[i].to_square() == Square::C1) has_qs_castle = true;
        }
    }
    EXPECT_TRUE(has_ks_castle);
    EXPECT_TRUE(has_qs_castle);

    // Cannot castle through attacked square (e.g. enemy rook attacking f1)
    Position pos_blocked;
    pos_blocked.set_fen("r3k2r/8/8/8/8/5r2/8/R3K2R w KQkq - 0 1");
    MoveList list_blocked;
    generate_legal_moves(pos_blocked, list_blocked);

    for (size_t i = 0; i < list_blocked.size(); ++i) {
        if (list_blocked[i].from_square() == Square::E1) {
            EXPECT_NE(list_blocked[i].to_square(), Square::G1) << "Cannot castle through attacked square f1!";
        }
    }
}

TEST_F(MovegenTest, EnPassantDiscoveredCheckEdgeCase) {
    Position pos;
    // King on e5, White pawn on d5, Black pawn on c5, Black rook on a5.
    // If Black plays c7-c5, White pawn d5xe6 e.p. would remove both pawns from rank 5,
    // exposing White king to horizontal check by Black rook on a5!
    pos.set_fen("8/8/8/r1kP4/8/8/8/4K3 w - - 0 1"); // Normal position
    // Now setup the rank-5 discovered check scenario:
    // White King on h5, White pawn on f5, Black pawn on g5, Black rook on a5
    Position ep_pos;
    ep_pos.set_fen("8/8/8/r4PpK/8/8/8/8 w - g6 0 1");
    MoveList ep_list;
    generate_legal_moves(ep_pos, ep_list);

    // En passant f5xg6 should NOT be legal because rank 5 would be cleared of pawns
    Move ep_move(Square::F5, Square::G6, MoveFlag::EnPassant);
    EXPECT_FALSE(ep_list.contains(ep_move)) << "En passant creating horizontal discovered check must be illegal!";
}

TEST_F(MovegenTest, CheckmateDetection) {
    Position pos;
    // Scholar's Mate / Fool's Mate checkmate
    pos.set_fen("rnb1kbnr/pppp1ppp/4p3/8/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(in_check(pos));
    EXPECT_TRUE(is_checkmate(pos));
    EXPECT_FALSE(is_stalemate(pos));
}

TEST_F(MovegenTest, StalemateDetection) {
    Position pos;
    // Black king on a8, White queen on c7, White king on a6 -> Stalemate
    pos.set_fen("k7/2Q5/K7/8/8/8/8/8 b - - 0 1");
    EXPECT_FALSE(in_check(pos));
    EXPECT_FALSE(is_checkmate(pos));
    EXPECT_TRUE(is_stalemate(pos));
}

TEST_F(MovegenTest, DrawByFiftyMovesAndInsufficientMaterial) {
    Position pos;
    pos.set_fen("8/8/8/8/8/4K3/8/4k3 w - - 100 50");
    EXPECT_TRUE(is_fifty_move_rule(pos));
    EXPECT_TRUE(is_insufficient_material(pos)); // K vs K

    Position kn_k;
    kn_k.set_fen("8/8/8/8/8/4K3/5N2/4k3 w - - 0 1");
    EXPECT_TRUE(is_insufficient_material(kn_k)); // K+N vs K

    Position kb_k;
    kb_k.set_fen("8/8/8/8/8/4K3/5B2/4k3 w - - 0 1");
    EXPECT_TRUE(is_insufficient_material(kb_k)); // K+B vs K
}
