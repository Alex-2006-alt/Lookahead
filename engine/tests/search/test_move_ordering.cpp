/*
 * What: Unit tests for move ordering heuristics, MVV-LVA, killers, and history.
 * Why:  Ensures move sorting correctness and verifies node reduction in alpha-beta search.
 * Layer: Test suite - verifies Layer 3 (search/move_ordering).
 */

#include <gtest/gtest.h>
#include "search/move_ordering.h"
#include "search/search.h"
#include "search/transposition_table.h"
#include "search/features.h"
#include "core/position.h"

using namespace lookahead;

TEST(MoveOrderingTest, TTMoveScoresHighest) {
    core::Position pos;
    ASSERT_TRUE(pos.set_fen(core::Position::START_FEN));

    const core::Move tt_move(core::Square::E2, core::Square::E4);
    const core::Move quiet_move(core::Square::D2, core::Square::D4);
    const search::KillerTable killers;
    const search::HistoryTable history;

    const int tt_score = search::score_move(pos, tt_move, tt_move, killers, history, 0);
    const int quiet_score = search::score_move(pos, quiet_move, tt_move, killers, history, 0);

    EXPECT_EQ(tt_score, search::SCORE_TT_MOVE);
    EXPECT_GT(tt_score, quiet_score);
    EXPECT_GT(tt_score, search::SCORE_CAPTURE_BASE);
}

TEST(MoveOrderingTest, MvvLvaPrioritizesHighVictimLowAttacker) {
    // Position with pawn capturing queen vs queen capturing pawn
    core::Position pos;
    // Black Queen on d5, White Pawn on e4, White Queen on d1, Black Pawn on d5
    ASSERT_TRUE(pos.set_fen("rnb1kbnr/ppp1pppp/8/3q4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1"));

    const core::Move pawn_takes_queen(core::Square::E4, core::Square::D5, core::MoveFlag::Capture);
    const int score_pxq = search::mvv_lva_score(pos, pawn_takes_queen);

    // Position with Queen capturing Pawn
    core::Position pos2;
    ASSERT_TRUE(pos2.set_fen("rnbqkbnr/ppp1pppp/8/3p4/4Q3/8/PPPP1PPP/RNB1KBNR w KQkq - 0 1"));
    const core::Move queen_takes_pawn(core::Square::E4, core::Square::D5, core::MoveFlag::Capture);
    const int score_qxp = search::mvv_lva_score(pos2, queen_takes_pawn);

    EXPECT_GT(score_pxq, score_qxp);
}

TEST(MoveOrderingTest, KillerMovesRankAboveHistory) {
    core::Position pos;
    ASSERT_TRUE(pos.set_fen(core::Position::START_FEN));

    search::KillerTable killers;
    search::HistoryTable history;

    const core::Move k1(core::Square::G1, core::Square::F3);
    const core::Move k2(core::Square::B1, core::Square::C3);
    const core::Move quiet(core::Square::E2, core::Square::E4);

    killers.store(2, k1);
    killers.store(2, k2);

    history.record(core::Color::White, core::Square::E2, core::Square::E4, 5);

    const int s_k1 = search::score_move(pos, k1, core::Move{}, killers, history, 2);
    const int s_k2 = search::score_move(pos, k2, core::Move{}, killers, history, 2);
    const int s_quiet = search::score_move(pos, quiet, core::Move{}, killers, history, 2);

    EXPECT_EQ(s_k2, search::SCORE_KILLER_1); // k2 was stored last, so it became primary killer
    EXPECT_EQ(s_k1, search::SCORE_KILLER_2); // k1 shifted to secondary killer
    EXPECT_GT(s_k2, s_k1);
    EXPECT_GT(s_k1, s_quiet);
    EXPECT_EQ(s_quiet, history.get(core::Color::White, core::Square::E2, core::Square::E4));
}

TEST(MoveOrderingTest, HistoryAccumulationAndAging) {
    search::HistoryTable history;
    history.clear();

    EXPECT_EQ(history.get(core::Color::White, core::Square::E2, core::Square::E4), 0);

    history.record(core::Color::White, core::Square::E2, core::Square::E4, 4); // +16
    EXPECT_EQ(history.get(core::Color::White, core::Square::E2, core::Square::E4), 16);

    history.record(core::Color::White, core::Square::E2, core::Square::E4, 5); // +25
    EXPECT_EQ(history.get(core::Color::White, core::Square::E2, core::Square::E4), 41);
}

TEST(MoveOrderingTest, PickNextMoveSelectsHighestScoredFirst) {
    movegen::MoveList moves;
    const core::Move m1(core::Square::A2, core::Square::A3);
    const core::Move m2(core::Square::B2, core::Square::B4);
    const core::Move m3(core::Square::C2, core::Square::C4);

    moves.push_back(m1);
    moves.push_back(m2);
    moves.push_back(m3);

    std::array<int, movegen::MoveList::MAX_CAPACITY> scores{};
    scores[0] = 100;
    scores[1] = 500; // Best move is at index 1
    scores[2] = 200;

    search::pick_next_move(moves, scores, 0);

    EXPECT_EQ(moves[0], m2);
    EXPECT_EQ(scores[0], 500);

    // Pick for next index 1
    search::pick_next_move(moves, scores, 1);
    EXPECT_EQ(moves[1], m3);
    EXPECT_EQ(scores[1], 200);

    EXPECT_EQ(moves[2], m1);
    EXPECT_EQ(scores[2], 100);
}

TEST(MoveOrderingTest, MoveOrderingReducesSearchNodes) {
    search::reset_features_to_default();

    core::Position pos;
    ASSERT_TRUE(pos.set_fen(core::Position::START_FEN));

    search::SearchLimits limits;
    limits.depth = 4;

    // Search with move ordering enabled
    search::active_features.use_move_ordering = true;
    search::global_tt.clear();
    const auto result_ordered = search::search(pos, limits);

    // Search with move ordering disabled
    search::active_features.use_move_ordering = false;
    search::global_tt.clear();
    const auto result_unordered = search::search(pos, limits);

    // Restore feature toggles to defaults
    search::reset_features_to_default();

    // Move ordering should significantly reduce total nodes searched
    EXPECT_GT(result_unordered.nodes, result_ordered.nodes);
}
