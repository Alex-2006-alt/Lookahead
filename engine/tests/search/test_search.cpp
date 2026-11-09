/*
 * What: Unit tests for Transposition Table, Quiescence Search, and Negamax alpha-beta engine.
 * Why:  Validates TT caching correctness, horizon effect mitigation, and mate-in-1 tactics.
 * Layer: Tests for Layer 3 (search) - validates search.h, transposition_table.h, and quiescence.h.
 */

#include <gtest/gtest.h>
#include "search/features.h"
#include "search/transposition_table.h"
#include "search/quiescence.h"
#include "search/search.h"
#include "core/position.h"
#include "movegen/attacks.h"

using namespace lookahead;
using namespace lookahead::core;
using namespace lookahead::search;

class SearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        movegen::attacks::init();
        global_tt.resize(4); // 4 MB for test harness
        global_tt.clear();
        reset_features_to_default();
    }
};

TEST_F(SearchTest, TTStoreAndProbeRoundtrip) {
    TranspositionTable tt(1);
    const uint64_t test_key = 0x123456789ABCDEF0ULL;
    const Move test_move(Square::E2, Square::E4, MoveFlag::Quiet);

    tt.store(test_key, test_move, 150, 4, TTFlag::Exact, 0);

    const TTEntry* entry = tt.probe(test_key);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->key, test_key);
    EXPECT_EQ(entry->move, test_move);
    EXPECT_EQ(entry->score, 150);
    EXPECT_EQ(entry->depth, 4);
    EXPECT_EQ(entry->flag, TTFlag::Exact);

    EXPECT_EQ(tt.probe(0xDEADBEEFULL), nullptr);
}

TEST_F(SearchTest, TTMateScoreAdjustmentByPly) {
    const int mate_score = SCORE_MATE - 3; // Mate in 3
    const int ply = 5;

    const int stored = score_to_tt(mate_score, ply);
    const int retrieved = score_from_tt(stored, ply);
    EXPECT_EQ(retrieved, mate_score);

    const int mated_score = -SCORE_MATE + 2; // Mated in 2
    const int stored_mated = score_to_tt(mated_score, ply);
    const int retrieved_mated = score_from_tt(stored_mated, ply);
    EXPECT_EQ(retrieved_mated, mated_score);
}

TEST_F(SearchTest, QuiescenceStandPatCutoff) {
    Position pos;
    // Massive white material advantage: White has Queen against lone King
    pos.set_fen("4k3/8/8/8/8/8/8/4K2Q w - - 0 1");
    uint64_t nodes = 0;

    // Beta below static eval triggers stand-pat cutoff immediately
    const int score = quiescence(pos, 0, 500, nodes);
    EXPECT_GE(score, 500);
}

TEST_F(SearchTest, NegamaxFindsScholarsMate) {
    Position pos;
    // White Queen on h5 and Bishop on c4 attacking f7
    ASSERT_TRUE(pos.set_fen("r1bqkb1r/pppp1ppp/2n5/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1"));

    SearchLimits limits;
    limits.depth = 2;
    const SearchResult res = search::search(pos, limits);

    EXPECT_EQ(res.best_move.to_uci(), "h5f7");
    EXPECT_GE(res.score, MATE_THRESHOLD);
}

TEST_F(SearchTest, NegamaxFindsBackRankMate) {
    Position pos;
    // White rook on e1 delivers back rank mate e1e8#
    ASSERT_TRUE(pos.set_fen("6k1/5ppp/8/8/8/8/8/4R1K1 w - - 0 1"));

    SearchLimits limits;
    limits.depth = 2;
    const SearchResult res = search::search(pos, limits);

    EXPECT_EQ(res.best_move.to_uci(), "e1e8");
    EXPECT_GE(res.score, MATE_THRESHOLD);
}

TEST_F(SearchTest, SearchWorksWithFeatureToggles) {
    Position pos;
    ASSERT_TRUE(pos.set_fen("6k1/5ppp/8/8/8/8/8/4R1K1 w - - 0 1"));

    active_features.use_transposition_table = false;
    active_features.use_quiescence_search = false;

    SearchLimits limits;
    limits.depth = 2;
    const SearchResult res = search::search(pos, limits);

    EXPECT_EQ(res.best_move.to_uci(), "e1e8");
    EXPECT_GE(res.score, MATE_THRESHOLD);
}
