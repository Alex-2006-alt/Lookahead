/*
 * What: Unit tests for Perft (Performance Test) move tree enumeration.
 * Why:  Validates correctness of legal move generator against ground-truth perft counts.
 * Layer: Tests for Layer 1 (movegen) - verifies perft.h/cpp against known perft suites.
 */

#include <gtest/gtest.h>
#include "movegen/perft.h"
#include "movegen/attacks.h"
#include "core/position.h"

using namespace lookahead;
using namespace lookahead::core;
using namespace lookahead::movegen;

class PerftTest : public ::testing::Test {
protected:
    void SetUp() override {
        movegen::attacks::init();
    }
};

TEST_F(PerftTest, InitialPositionDepths) {
    Position pos;

    EXPECT_EQ(perft(pos, 0), 1ULL);
    EXPECT_EQ(perft(pos, 1), 20ULL);
    EXPECT_EQ(perft(pos, 2), 400ULL);
    EXPECT_EQ(perft(pos, 3), 8902ULL);
    EXPECT_EQ(perft(pos, 4), 197281ULL);
    // Depth 5 standard count: 4,865,609
    EXPECT_EQ(perft(pos, 5), 4865609ULL);
}

TEST_F(PerftTest, KiwipeteDepths) {
    // Kiwipete by Peter McKenzie: heavy with castling rights, pins, promotions, checks
    Position pos;
    ASSERT_TRUE(pos.set_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));

    EXPECT_EQ(perft(pos, 1), 48ULL);
    EXPECT_EQ(perft(pos, 2), 2039ULL);
    EXPECT_EQ(perft(pos, 3), 97862ULL);
    EXPECT_EQ(perft(pos, 4), 4085603ULL);
}

TEST_F(PerftTest, Position3PawnsAndPins) {
    Position pos;
    ASSERT_TRUE(pos.set_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"));

    EXPECT_EQ(perft(pos, 1), 14ULL);
    EXPECT_EQ(perft(pos, 2), 191ULL);
    EXPECT_EQ(perft(pos, 3), 2812ULL);
    EXPECT_EQ(perft(pos, 4), 43238ULL);
    EXPECT_EQ(perft(pos, 5), 674624ULL);
}

TEST_F(PerftTest, Position4PromotionsAndCastling) {
    Position pos;
    ASSERT_TRUE(pos.set_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"));

    EXPECT_EQ(perft(pos, 1), 6ULL);
    EXPECT_EQ(perft(pos, 2), 264ULL);
    EXPECT_EQ(perft(pos, 3), 9467ULL);
    EXPECT_EQ(perft(pos, 4), 422333ULL);
}

TEST_F(PerftTest, Position5DiscoveredChecksAndPins) {
    Position pos;
    ASSERT_TRUE(pos.set_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"));

    EXPECT_EQ(perft(pos, 1), 44ULL);
    EXPECT_EQ(perft(pos, 2), 1486ULL);
    EXPECT_EQ(perft(pos, 3), 62379ULL);
    EXPECT_EQ(perft(pos, 4), 2103487ULL);
}

TEST_F(PerftTest, Position6KingEvasions) {
    Position pos;
    ASSERT_TRUE(pos.set_fen("8/8/1k6/2b5/2n1P3/8/1K5B/8 w - - 0 1"));

    EXPECT_EQ(perft(pos, 1), 7ULL);
    EXPECT_EQ(perft(pos, 2), 154ULL);
    EXPECT_EQ(perft(pos, 3), 1709ULL);
    EXPECT_EQ(perft(pos, 4), 34243ULL);
    EXPECT_EQ(perft(pos, 5), 403657ULL);
}

TEST_F(PerftTest, PerftDivideConsistency) {
    Position pos;

    auto divide_d1 = perft_divide(pos, 1);
    EXPECT_EQ(divide_d1.size(), 20ULL);
    uint64_t sum_d1 = 0;
    for (const auto& entry : divide_d1) {
        sum_d1 += entry.nodes;
    }
    EXPECT_EQ(sum_d1, 20ULL);

    auto divide_d2 = perft_divide(pos, 2);
    EXPECT_EQ(divide_d2.size(), 20ULL);
    uint64_t sum_d2 = 0;
    for (const auto& entry : divide_d2) {
        sum_d2 += entry.nodes;
    }
    EXPECT_EQ(sum_d2, 400ULL);
}

TEST_F(PerftTest, RunPerftThroughput) {
    Position pos;
    PerftResult res = run_perft(pos, 4);

    EXPECT_EQ(res.nodes, 197281ULL);
    EXPECT_GE(res.elapsed_seconds, 0.0);
    EXPECT_GT(res.nps, 0.0);
}
