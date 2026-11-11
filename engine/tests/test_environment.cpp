/*
 * What: Global test environment initialization for GoogleTest runner.
 * Why:  Ensures Zobrist tables, attack bitboards, and TT are initialized before any test runs.
 * Layer: Test harness infrastructure.
 */

#include <gtest/gtest.h>
#include "core/zobrist.h"
#include "movegen/attacks.h"
#include "search/transposition_table.h"
#include "search/features.h"

namespace {

class GlobalTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        lookahead::core::zobrist::init();
        lookahead::movegen::attacks::init();
        lookahead::search::global_tt.resize(16);
        lookahead::search::reset_features_to_default();
    }
};

[[maybe_unused]] ::testing::Environment* const s_env =
    ::testing::AddGlobalTestEnvironment(new GlobalTestEnvironment());

}  // namespace
