/*
 * What: Unit tests for UCI protocol parser, command handler, and board setup.
 * Why:  Ensures standard compliance with chess GUI communication specifications.
 * Layer: Test suite - verifies Layer 4 (interface/uci).
 */

#include <gtest/gtest.h>
#include <sstream>
#include "interface/uci.h"
#include "core/version.h"
#include "search/transposition_table.h"
#include "search/features.h"

using namespace lookahead;

class UciTest : public ::testing::Test {
protected:
    void SetUp() override {
        search::reset_features_to_default();
        search::global_tt.resize(16);
        search::global_tt.clear();
    }

    void TearDown() override {
        search::reset_features_to_default();
    }
};

TEST_F(UciTest, UciHandshake) {
    std::istringstream in("uci\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const std::string response = out.str();
    EXPECT_NE(response.find("id name Lookahead"), std::string::npos);
    EXPECT_NE(response.find("id author Alex"), std::string::npos);
    EXPECT_NE(response.find("option name Hash type spin"), std::string::npos);
    EXPECT_NE(response.find("option name Clear Hash type button"), std::string::npos);
    EXPECT_NE(response.find("uciok"), std::string::npos);
}

TEST_F(UciTest, IsReadyResponse) {
    std::istringstream in("isready\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const std::string response = out.str();
    EXPECT_NE(response.find("readyok"), std::string::npos);
}

TEST_F(UciTest, UciNewGameResetsBoard) {
    std::istringstream in("position startpos moves e2e4 e7e5\nucinewgame\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    EXPECT_EQ(uci.position().to_fen(), core::Position::START_FEN);
}

TEST_F(UciTest, PositionStartposMoves) {
    std::istringstream in("position startpos moves e2e4 e7e5 g1f3\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const auto& pos = uci.position();
    EXPECT_EQ(pos.piece_at(core::Square::E4), core::Piece::WhitePawn);
    EXPECT_EQ(pos.piece_at(core::Square::E5), core::Piece::BlackPawn);
    EXPECT_EQ(pos.piece_at(core::Square::F3), core::Piece::WhiteKnight);
    EXPECT_EQ(pos.side_to_move(), core::Color::Black);
}

TEST_F(UciTest, PositionFenAndMoves) {
    // Scholar's Mate setup position
    const std::string fen = "r1bqkb1r/pppp1ppp/2n5/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1";
    std::istringstream in("position fen " + fen + " moves h5f7\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const auto& pos = uci.position();
    EXPECT_EQ(pos.piece_at(core::Square::F7), core::Piece::WhiteQueen);
    EXPECT_EQ(pos.side_to_move(), core::Color::Black);
}

TEST_F(UciTest, SetOptionHashAndClear) {
    std::istringstream in("setoption name Hash value 32\nsetoption name Clear Hash\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    EXPECT_FALSE(uci.is_running());
}

TEST_F(UciTest, GoSearchProducesBestMove) {
    std::istringstream in("position startpos\ngo depth 2\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const std::string response = out.str();
    EXPECT_NE(response.find("info depth"), std::string::npos);
    EXPECT_NE(response.find("bestmove"), std::string::npos);
}

TEST_F(UciTest, StopInterruptsInfiniteSearch) {
    std::istringstream in("position startpos\ngo infinite\nstop\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const std::string response = out.str();
    EXPECT_NE(response.find("bestmove"), std::string::npos);
    EXPECT_FALSE(uci.is_searching());
    EXPECT_FALSE(uci.is_running());
}

TEST_F(UciTest, IsReadyHandshakeDuringSearch) {
    std::istringstream in("position startpos\ngo depth 10\nisready\nstop\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const std::string response = out.str();
    EXPECT_NE(response.find("readyok"), std::string::npos);
    EXPECT_NE(response.find("bestmove"), std::string::npos);
}

TEST_F(UciTest, QuitAbortsActiveSearchGracefully) {
    std::istringstream in("position startpos\ngo infinite\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    EXPECT_FALSE(uci.is_searching());
    EXPECT_FALSE(uci.is_running());
}

TEST_F(UciTest, AdvertisesAndTogglesSearchFeatureOptions) {
    std::istringstream in(
        "uci\n"
        "setoption name TranspositionTable value false\n"
        "setoption name QuiescenceSearch value false\n"
        "setoption name MoveOrdering value false\n"
        "setoption name NullMovePruning value false\n"
        "setoption name LateMoveReductions value false\n"
        "quit\n"
    );
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const std::string response = out.str();
    EXPECT_NE(response.find("option name TranspositionTable type check"), std::string::npos);
    EXPECT_NE(response.find("option name QuiescenceSearch type check"), std::string::npos);
    EXPECT_NE(response.find("option name MoveOrdering type check"), std::string::npos);
    EXPECT_NE(response.find("option name NullMovePruning type check"), std::string::npos);
    EXPECT_NE(response.find("option name LateMoveReductions type check"), std::string::npos);

    EXPECT_FALSE(search::active_features.use_transposition_table);
    EXPECT_FALSE(search::active_features.use_quiescence_search);
    EXPECT_FALSE(search::active_features.use_move_ordering);
    EXPECT_FALSE(search::active_features.use_null_move_pruning);
    EXPECT_FALSE(search::active_features.use_late_move_reductions);

    std::istringstream in2("setoption name TranspositionTable value true\nquit\n");
    std::ostringstream out2;
    interface::UciHandler uci2(in2, out2);
    uci2.loop();
    EXPECT_TRUE(search::active_features.use_transposition_table);
}
