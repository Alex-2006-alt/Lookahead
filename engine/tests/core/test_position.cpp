#include <gtest/gtest.h>
#include <vector>
#include "core/position.h"

using namespace lookahead::core;

TEST(PositionTest, StartingPositionFenRoundtrip) {
    Position pos;
    EXPECT_EQ(pos.to_fen(), Position::START_FEN);
    EXPECT_EQ(pos.side_to_move(), Color::White);
    EXPECT_EQ(pos.castling_rights(), CastlingRights::All);
    EXPECT_EQ(pos.en_passant_sq(), Square::None);
    EXPECT_EQ(pos.halfmove_clock(), 0);
    EXPECT_EQ(pos.fullmove_number(), 1);
    EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist());
}

TEST(PositionTest, TrickyPositionsFenRoundtrip) {
    const std::vector<std::string> fens = {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "8/8/8/8/8/4K3/4P3/4k3 w - - 0 1"
    };

    for (const auto& fen : fens) {
        Position pos;
        std::string err;
        ASSERT_TRUE(pos.set_fen(fen, &err)) << "Failed to parse FEN: " << fen << " error: " << err;
        EXPECT_EQ(pos.to_fen(), fen);
        EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist()) << "Zobrist mismatch for: " << fen;
    }
}

TEST(PositionTest, InvalidFenParsing) {
    Position pos;
    std::string err;
    // Missing active color
    EXPECT_FALSE(pos.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", &err));
    // Invalid piece character 'X'
    EXPECT_FALSE(pos.set_fen("rnbqkbnr/ppppXppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &err));
    // Invalid rank count
    EXPECT_FALSE(pos.set_fen("8/8/8 w - - 0 1", &err));
}

TEST(PositionTest, MakeUnmakeQuietPawnPush) {
    Position pos;
    const std::string start_fen = pos.to_fen();
    const Key start_hash = pos.zobrist_key();

    Move e4(Square::E2, Square::E4, MoveFlag::DoublePawnPush);
    UndoInfo undo;
    pos.make_move(e4, undo);

    EXPECT_EQ(pos.piece_at(Square::E2), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::E4), Piece::WhitePawn);
    EXPECT_EQ(pos.side_to_move(), Color::Black);
    EXPECT_EQ(pos.en_passant_sq(), Square::E3);
    EXPECT_EQ(pos.halfmove_clock(), 0);
    EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist());

    pos.unmake_move(e4, undo);
    EXPECT_EQ(pos.to_fen(), start_fen);
    EXPECT_EQ(pos.zobrist_key(), start_hash);
}

TEST(PositionTest, MakeUnmakeCastling) {
    Position pos;
    pos.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    const std::string before_fen = pos.to_fen();
    const Key before_hash = pos.zobrist_key();

    Move ks_castle(Square::E1, Square::G1, MoveFlag::KingCastle);
    UndoInfo undo;
    pos.make_move(ks_castle, undo);

    EXPECT_EQ(pos.piece_at(Square::G1), Piece::WhiteKing);
    EXPECT_EQ(pos.piece_at(Square::F1), Piece::WhiteRook);
    EXPECT_EQ(pos.piece_at(Square::E1), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::H1), Piece::None);
    EXPECT_EQ(pos.castling_rights(), CastlingRights::BlackAll);
    EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist());

    pos.unmake_move(ks_castle, undo);
    EXPECT_EQ(pos.to_fen(), before_fen);
    EXPECT_EQ(pos.zobrist_key(), before_hash);
}

TEST(PositionTest, MakeUnmakeEnPassant) {
    Position pos;
    pos.set_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    const std::string before_fen = pos.to_fen();
    const Key before_hash = pos.zobrist_key();

    Move ep_capture(Square::E5, Square::F6, MoveFlag::EnPassant);
    UndoInfo undo;
    pos.make_move(ep_capture, undo);

    EXPECT_EQ(pos.piece_at(Square::E5), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::F5), Piece::None);  // captured black pawn
    EXPECT_EQ(pos.piece_at(Square::F6), Piece::WhitePawn);
    EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist());

    pos.unmake_move(ep_capture, undo);
    EXPECT_EQ(pos.to_fen(), before_fen);
    EXPECT_EQ(pos.zobrist_key(), before_hash);
}

TEST(PositionTest, MakeUnmakePromotionCapture) {
    Position pos;
    pos.set_fen("r1bqk2r/pPpppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 5");
    const std::string before_fen = pos.to_fen();
    const Key before_hash = pos.zobrist_key();

    Move promo_cap(Square::B7, Square::A8, MoveFlag::QueenPromoCapture);
    UndoInfo undo;
    pos.make_move(promo_cap, undo);

    EXPECT_EQ(pos.piece_at(Square::A8), Piece::WhiteQueen);
    EXPECT_EQ(pos.piece_at(Square::B7), Piece::None);
    EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist());

    pos.unmake_move(promo_cap, undo);
    EXPECT_EQ(pos.to_fen(), before_fen);
    EXPECT_EQ(pos.zobrist_key(), before_hash);
}

TEST(PositionTest, MakeUnmakeMoveChainFuzz) {
    Position pos;
    const std::string initial_fen = pos.to_fen();
    const Key initial_hash = pos.zobrist_key();

    // Sequence of moves
    std::vector<Move> moves = {
        Move(Square::E2, Square::E4, MoveFlag::DoublePawnPush),
        Move(Square::E7, Square::E5, MoveFlag::DoublePawnPush),
        Move(Square::G1, Square::F3, MoveFlag::Quiet),
        Move(Square::B8, Square::C6, MoveFlag::Quiet),
        Move(Square::F1, Square::B5, MoveFlag::Quiet),
        Move(Square::A7, Square::A6, MoveFlag::Quiet),
        Move(Square::B5, Square::C6, MoveFlag::Capture)
    };

    std::vector<UndoInfo> undo_stack;
    for (const auto& m : moves) {
        UndoInfo undo;
        pos.make_move(m, undo);
        undo_stack.push_back(undo);
        EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist()) << "Incremental hash diverged at move " << m.to_uci();
    }

    // Unmake all moves in reverse
    for (int i = static_cast<int>(moves.size()) - 1; i >= 0; --i) {
        pos.unmake_move(moves[i], undo_stack[i]);
        EXPECT_EQ(pos.zobrist_key(), pos.compute_zobrist()) << "Incremental hash diverged during unmake of " << moves[i].to_uci();
    }

    EXPECT_EQ(pos.to_fen(), initial_fen);
    EXPECT_EQ(pos.zobrist_key(), initial_hash);
}
