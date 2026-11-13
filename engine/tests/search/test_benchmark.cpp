/*
 * What: Unit tests for engine benchmark suite and UCI benchmark dispatch.
 * Why:  Ensures benchmark execution produces valid node metrics and preserves test isolation.
 * Layer: Test suite - verifies Layer 3 (search/benchmark) and Layer 4 (interface/uci).
 */

#include <gtest/gtest.h>
#include <sstream>
#include "search/benchmark.h"
#include "search/transposition_table.h"
#include "search/features.h"
#include "core/position.h"
#include "interface/uci.h"

using namespace lookahead;

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        search::reset_features_to_default();
        search::global_tt.resize(16);
        search::global_tt.clear();
    }

    void TearDown() override {
        search::reset_features_to_default();
        search::global_tt.clear();
    }
};

TEST_F(BenchmarkTest, BenchmarkSuiteDefinition) {
    const auto& suite = search::get_benchmark_suite();
    EXPECT_EQ(suite.size(), 12U);

    for (const auto& item : suite) {
        EXPECT_FALSE(item.name.empty());
        core::Position pos;
        EXPECT_TRUE(pos.set_fen(item.fen)) << "Failed to parse FEN for: " << item.name;
    }
}

TEST_F(BenchmarkTest, RunBenchmarkExecution) {
    std::ostringstream out;
    const auto res = search::run_benchmark(2, out, 8);

    EXPECT_EQ(res.positions_count, 12);
    EXPECT_GT(res.total_nodes, 0ULL);
    EXPECT_GE(res.total_time_sec, 0.0);
    EXPECT_GT(res.nps, 0.0);

    const std::string text = out.str();
    EXPECT_NE(text.find("Lookahead Search Benchmark"), std::string::npos);
    EXPECT_NE(text.find("Total Nodes:"), std::string::npos);
    EXPECT_NE(text.find("Aggregate NPS:"), std::string::npos);
}

TEST_F(BenchmarkTest, UciBenchCommandDispatch) {
    std::istringstream in("bench 2\nquit\n");
    std::ostringstream out;

    interface::UciHandler uci(in, out);
    uci.loop();

    const std::string text = out.str();
    EXPECT_NE(text.find("Lookahead Search Benchmark"), std::string::npos);
    EXPECT_NE(text.find("Total Nodes:"), std::string::npos);
    EXPECT_NE(text.find("Aggregate NPS:"), std::string::npos);
}

