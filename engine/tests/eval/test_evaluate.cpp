/*
 * What: Unit tests for material values, PeSTO piece-square tables, and evaluation symmetry.
 * Why:  Guarantees mathematical symmetry, correct game phase tapering, and term sanity.
 * Layer: Tests for Layer 2 (eval) - validates evaluate.h and material_psqt.h against positions.
 */

#include <gtest/gtest.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include "eval/evaluate.h"
#include "eval/material_psqt.h"
#include "core/position.h"
#include "movegen/attacks.h"

using namespace lookahead;
using namespace lookahead::core;
using namespace lookahead::eval;

class EvalTest : public ::testing::Test {
protected:
    void SetUp() override {
        movegen::attacks::init();
    }

    // Helper to mirror a chess position vertically and swap sides to test mathematical symmetry.
    static Position mirror_position(const Position& pos) {
        std::string fen = pos.to_fen();
        std::istringstream iss(fen);
        std::string board_part, active_color, castling, ep_sq;
        int halfmove = 0, fullmove = 1;
        iss >> board_part >> active_color >> castling >> ep_sq >> halfmove >> fullmove;

        // Split ranks
        std::vector<std::string> ranks;
        std::stringstream ss(board_part);
        std::string rank_str;
        while (std::getline(ss, rank_str, '/')) {
            ranks.push_back(rank_str);
        }

        // Reverse ranks and swap case of pieces
        std::reverse(ranks.begin(), ranks.end());
        std::string mirrored_board;
        for (size_t i = 0; i < ranks.size(); ++i) {
            if (i > 0) mirrored_board += '/';
            for (char ch : ranks[i]) {
                if (std::isupper(static_cast<unsigned char>(ch))) {
                    mirrored_board += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                } else if (std::islower(static_cast<unsigned char>(ch))) {
                    mirrored_board += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                } else {
                    mirrored_board += ch;
                }
            }
        }

        std::string mirrored_color = (active_color == "w") ? "b" : "w";

        std::string mirrored_castling;
        if (castling == "-") {
            mirrored_castling = "-";
        } else {
            for (char ch : castling) {
                if (std::isupper(static_cast<unsigned char>(ch))) {
                    mirrored_castling += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                } else if (std::islower(static_cast<unsigned char>(ch))) {
                    mirrored_castling += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                }
            }
        }

        std::string mirrored_ep = "-";
        if (ep_sq != "-") {
            char file = ep_sq[0];
            char rank = ep_sq[1];
            char mirrored_rank = (rank == '3') ? '6' : (rank == '6') ? '3' : rank;
            mirrored_ep = std::string(1, file) + mirrored_rank;
        }

        std::string mirrored_fen = mirrored_board + " " + mirrored_color + " " +
                                   mirrored_castling + " " + mirrored_ep + " " +
                                   std::to_string(halfmove) + " " + std::to_string(fullmove);

        Position mirrored_pos;
        mirrored_pos.set_fen(mirrored_fen);
        return mirrored_pos;
    }
};

TEST_F(EvalTest, StartingPositionIsSymmetricAndZero) {
    Position pos;
    EvalBreakdown breakdown;
    const int score = evaluate_detailed(pos, breakdown);

    EXPECT_EQ(score, 0);
    EXPECT_EQ(breakdown.material.mg, 0);
    EXPECT_EQ(breakdown.material.eg, 0);
    EXPECT_EQ(breakdown.psqt.mg, 0);
    EXPECT_EQ(breakdown.psqt.eg, 0);
    EXPECT_EQ(breakdown.game_phase, TOTAL_PHASE);
}

TEST_F(EvalTest, GamePhaseCalculation) {
    Position pos_start;
    EXPECT_EQ(calculate_game_phase(pos_start), 24);

    Position pos_kings;
    pos_kings.set_fen("8/8/8/4k3/8/8/4K3/8 w - - 0 1");
    EXPECT_EQ(calculate_game_phase(pos_kings), 0);

    // Two rooks each: 2 * 2 + 2 * 2 = 8
    Position pos_rooks;
    pos_rooks.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");
    EXPECT_EQ(calculate_game_phase(pos_rooks), 8);
}

TEST_F(EvalTest, MaterialAdvantageReflectedInScore) {
    Position pos;
    // White has queen and king vs black king
    pos.set_fen("8/8/8/8/8/8/8/Q1k1K3 w - - 0 1");
    const int white_score = evaluate(pos);
    EXPECT_GT(white_score, 800);

    // Black to move in identical material situation has negative evaluation
    Position pos_black;
    pos_black.set_fen("8/8/8/8/8/8/8/Q1k1K3 b - - 0 1");
    const int black_score = evaluate(pos_black);
    EXPECT_LT(black_score, -800);
}

TEST_F(EvalTest, CentralKnightValuedAboveRimKnight) {
    Position pos_center;
    // White knight on central square d4
    pos_center.set_fen("8/8/8/8/3N4/8/8/4K2k w - - 0 1");
    const int score_center = evaluate(pos_center);

    Position pos_rim;
    // White knight on rim square a1
    pos_rim.set_fen("8/8/8/8/8/8/8/N3K2k w - - 0 1");
    const int score_rim = evaluate(pos_rim);

    EXPECT_GT(score_center, score_rim);
}

TEST_F(EvalTest, EvaluationSymmetryAcrossDiversePositions) {
    const std::vector<std::string> test_fens = {
        Position::START_FEN.data(),
        "r1bqk2r/pppp1ppp/2n5/4p3/1bB1P1n1/2NP1N2/PPP2PPP/R1BQK2R w KQkq - 1 6",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "8/8/1k6/2b5/2n1P3/8/1K5B/8 w - - 0 1"
    };

    for (const auto& fen : test_fens) {
        Position pos;
        ASSERT_TRUE(pos.set_fen(fen));
        Position mirrored = mirror_position(pos);

        EvalBreakdown bd, mirrored_bd;
        const int score = evaluate_detailed(pos, bd);
        const int mirrored_score = evaluate_detailed(mirrored, mirrored_bd);

        // Score from perspective of side to move must be identical when sides swapped
        EXPECT_EQ(score, mirrored_score) << "Failed for FEN: " << fen;
        EXPECT_EQ(bd.material.mg, -mirrored_bd.material.mg) << "MG Material mismatch for FEN: " << fen;
        EXPECT_EQ(bd.material.eg, -mirrored_bd.material.eg) << "EG Material mismatch for FEN: " << fen;
        EXPECT_EQ(bd.psqt.mg, -mirrored_bd.psqt.mg) << "MG PSQT mismatch for FEN: " << fen;
        EXPECT_EQ(bd.psqt.eg, -mirrored_bd.psqt.eg) << "EG PSQT mismatch for FEN: " << fen;
        EXPECT_EQ(bd.game_phase, mirrored_bd.game_phase) << "Phase mismatch for FEN: " << fen;
    }
}
