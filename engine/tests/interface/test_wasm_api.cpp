/*
 * What: Unit tests for WebAssembly C-API bridge functions.
 * Why:  Ensures C-linkage export functions correctly route commands and telemetry.
 * Layer: Test suite - verifies Layer 4 (interface/wasm_api).
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstring>
#include <mutex>
#include <chrono>
#include <thread>
#include "interface/wasm_api.h"
#include "search/features.h"
#include "search/transposition_table.h"

namespace {

std::mutex g_capture_mutex;
std::vector<std::string> g_captured_lines;

void test_callback(const char* line) {
    if (line) {
        std::lock_guard<std::mutex> lock(g_capture_mutex);
        g_captured_lines.emplace_back(line);
    }
}

}  // namespace

class WasmApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        lookahead::search::reset_features_to_default();
        lookahead::search::global_tt.clear();
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            g_captured_lines.clear();
        }
        lookahead_init(16);
        lookahead_set_output_callback(test_callback);
    }

    void TearDown() override {
        lookahead_shutdown();
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            g_captured_lines.clear();
        }
        lookahead::search::reset_features_to_default();
    }
};

TEST_F(WasmApiTest, UciHandshakeThroughCallback) {
    lookahead_send_command("uci\n");

    bool found_id = false;
    bool found_uciok = false;
    std::lock_guard<std::mutex> lock(g_capture_mutex);
    for (const auto& line : g_captured_lines) {
        if (line.find("id name Lookahead") != std::string::npos) found_id = true;
        if (line == "uciok") found_uciok = true;
    }
    EXPECT_TRUE(found_id);
    EXPECT_TRUE(found_uciok);
}

TEST_F(WasmApiTest, PollingQueueRead) {
    lookahead_set_output_callback(nullptr);
    lookahead_send_command("isready\n");

    char buffer[256];
    bool found_readyok = false;
    while (lookahead_read_output(buffer, sizeof(buffer)) == 1) {
        if (std::string(buffer) == "readyok") {
            found_readyok = true;
            break;
        }
    }
    EXPECT_TRUE(found_readyok);
}

TEST_F(WasmApiTest, TacticalSearchBestmove) {
    const char* mate_fen = "r1bqkb1r/pppp1ppp/2n5/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1";
    std::string cmd = std::string("position fen ") + mate_fen + "\ngo depth 1\n";
    lookahead_send_command(cmd.c_str());

    bool found_mate_move = false;
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < 2.0) {
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            for (const auto& line : g_captured_lines) {
                if (line.find("bestmove h5f7") != std::string::npos) {
                    found_mate_move = true;
                    break;
                }
            }
        }
        if (found_mate_move) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(found_mate_move);
}

TEST_F(WasmApiTest, CleanShutdownAndReinit) {
    lookahead_shutdown();
    lookahead_init(16);
    lookahead_set_output_callback(test_callback);
    lookahead_send_command("isready\n");

    bool found_readyok = false;
    std::lock_guard<std::mutex> lock(g_capture_mutex);
    for (const auto& line : g_captured_lines) {
        if (line == "readyok") found_readyok = true;
    }
    EXPECT_TRUE(found_readyok);
}

