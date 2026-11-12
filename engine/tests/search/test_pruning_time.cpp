/*
 * What: Unit tests for pruning algorithms, reductions, time management, and EPD mate puzzles.
 * Why:  Validates search termination guarantees, pruning safety, and tactical mate accuracy.
 * Layer: Test suite - verifies Layer 3 (search/pruning, search/time_manager, search/search).
 */

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include "search/pruning.h"
#include "search/time_manager.h"
#include "search/search.h"
#include "search/transposition_table.h"
#include "search/features.h"
#include "core/position.h"

using namespace lookahead;

TEST(PruningTest, HasNonPawnMaterialDetection) {
    core::Position pawn_ending;
    ASSERT_TRUE(pawn_ending.set_fen("8/5k2/8/8/8/8/5K2/4P3 w - - 0 1"));
    EXPECT_FALSE(search::has_non_pawn_material(pawn_ending, core::Color::White));
    EXPECT_FALSE(search::has_non_pawn_material(pawn_ending, core::Color::Black));

    core::Position with_knight;
    ASSERT_TRUE(with_knight.set_fen("8/5k2/8/8/8/5N2/5K2/4P3 w - - 0 1"));
    EXPECT_TRUE(search::has_non_pawn_material(with_knight, core::Color::White));
    EXPECT_FALSE(search::has_non_pawn_material(with_knight, core::Color::Black));
}

TEST(PruningTest, ReverseFutilityPruningConditions) {
    EXPECT_TRUE(search::can_reverse_futility_prune(1, 500, 300));
    EXPECT_FALSE(search::can_reverse_futility_prune(1, 200, 300));
    EXPECT_FALSE(search::can_reverse_futility_prune(4, 1000, 300));
}

TEST(PruningTest, LateMoveReductionCalculation) {
    EXPECT_EQ(search::late_move_reduction(2, 5), 0);
    EXPECT_EQ(search::late_move_reduction(4, 2), 0);
    EXPECT_GE(search::late_move_reduction(5, 8), 1);
    EXPECT_LE(search::late_move_reduction(5, 8), 3);
}

TEST(TimeManagerTest, ExactMoveTimeHardLimit) {
    search::TimeManager tm;
    search::SearchLimits limits;
    limits.move_time_ms = 50;

    tm.init(limits, core::Color::White);
    EXPECT_FALSE(tm.should_stop(0));

    // After 70ms, periodic node check triggers stop
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    EXPECT_TRUE(tm.should_stop(2048));
}

TEST(TimeManagerTest, NodeCountLimit) {
    search::TimeManager tm;
    search::SearchLimits limits;
    limits.max_nodes = 5000;

    tm.init(limits, core::Color::White);
    EXPECT_FALSE(tm.should_stop(4000));
    EXPECT_TRUE(tm.should_stop(5000));
    EXPECT_TRUE(tm.is_stopped());
}

TEST(SearchIntegrationTest, SearchCallbackReportsProgress) {
    search::reset_features_to_default();
    search::global_tt.clear();

    core::Position pos;
    ASSERT_TRUE(pos.set_fen(core::Position::START_FEN));

    search::SearchLimits limits;
    limits.depth = 3;

    int callback_invocations = 0;
    int last_depth_seen = 0;

    const auto res = search::search(pos, limits, [&](const search::SearchInfo& info) {
        ++callback_invocations;
        last_depth_seen = info.depth;
        EXPECT_GT(info.nodes, 0ULL);
        EXPECT_FALSE(info.pv.empty());
    });

    EXPECT_GE(callback_invocations, 3);
    EXPECT_EQ(last_depth_seen, 3);
    EXPECT_EQ(res.depth_reached, 3);
    EXPECT_FALSE(res.pv_string.empty());
}

TEST(SearchIntegrationTest, SolvesAllMatePuzzlesFromEpd) {
    search::reset_features_to_default();
    search::global_tt.clear();

    std::ifstream file("data/mate_puzzles.epd");
    if (!file.is_open()) {
        file.open("../data/mate_puzzles.epd");
    }
    ASSERT_TRUE(file.is_open());

    std::string line;
    int puzzles_tested = 0;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const size_t bm_pos = line.find("bm ");
        ASSERT_NE(bm_pos, std::string::npos);

        const std::string fen = line.substr(0, bm_pos - 1);
        const std::string rest = line.substr(bm_pos + 3);
        const size_t semi_pos = rest.find(';');
        ASSERT_NE(semi_pos, std::string::npos);
        const std::string expected_move = rest.substr(0, semi_pos);

        core::Position pos;
        ASSERT_TRUE(pos.set_fen(fen));

        search::SearchLimits limits;
        limits.depth = 5;

        const auto result = search::search(pos, limits);
        EXPECT_EQ(result.best_move.to_uci(), expected_move)
            << "Failed on FEN: " << fen << " expected: " << expected_move
            << " got: " << result.best_move.to_uci();

        ++puzzles_tested;
    }

    EXPECT_GE(puzzles_tested, 8);
}
