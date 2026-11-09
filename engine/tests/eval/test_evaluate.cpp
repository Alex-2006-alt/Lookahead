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
#include "eval/pawn_structure.h"
#include "eval/king_safety.h"
#include "eval/mobility.h"
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
    EXPECT_EQ(breakdown.pawns.mg, 0);
    EXPECT_EQ(breakdown.pawns.eg, 0);
    EXPECT_EQ(breakdown.king_safety.mg, 0);
    EXPECT_EQ(breakdown.king_safety.eg, 0);
    EXPECT_EQ(breakdown.mobility.mg, 0);
    EXPECT_EQ(breakdown.mobility.eg, 0);
    EXPECT_EQ(breakdown.game_phase, TOTAL_PHASE);
}

TEST_F(EvalTest, GamePhaseCalculation) {
    Position pos_start;
    EXPECT_EQ(calculate_game_phase(pos_start), 24);

    Position pos_kings;
    pos_kings.set_fen("8/8/8/4k3/8/8/4K3/8 w - - 0 1");
    EXPECT_EQ(calculate_game_phase(pos_kings), 0);

    // Two rooks each: 4 rooks * 2 = 8
    Position pos_rooks;
    pos_rooks.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");
    EXPECT_EQ(calculate_game_phase(pos_rooks), 8);
}

TEST_F(EvalTest, MaterialAdvantageReflectedInScore) {
    Position pos;
    pos.set_fen("8/8/8/8/8/8/8/Q1k1K3 w - - 0 1");
    const int white_score = evaluate(pos);
    EXPECT_GT(white_score, 800);

    Position pos_black;
    pos_black.set_fen("8/8/8/8/8/8/8/Q1k1K3 b - - 0 1");
    const int black_score = evaluate(pos_black);
    EXPECT_LT(black_score, -800);
}

TEST_F(EvalTest, CentralKnightValuedAboveRimKnight) {
    Position pos_center;
    pos_center.set_fen("8/8/8/8/3N4/8/8/4K2k w - - 0 1");
    const int score_center = evaluate(pos_center);

    Position pos_rim;
    pos_rim.set_fen("8/8/8/8/8/8/8/N3K2k w - - 0 1");
    const int score_rim = evaluate(pos_rim);

    EXPECT_GT(score_center, score_rim);
}

TEST_F(EvalTest, DoubledPawnPenalty) {
    Position pos_clean;
    // White pawns on d2, e2; Black pawns on d7, e7
    pos_clean.set_fen("4k3/3pp3/8/8/8/8/3PP3/4K3 w - - 0 1");
    EvalBreakdown bd_clean;
    static_cast<void>(evaluate_detailed(pos_clean, bd_clean));
    EXPECT_EQ(bd_clean.pawns.mg, 0);

    Position pos_doubled;
    // White pawns on e2, e3 (doubled on e-file); Black pawns on d7, e7
    pos_doubled.set_fen("4k3/3pp3/8/8/8/4P3/4P3/4K3 w - - 0 1");
    EvalBreakdown bd_doubled;
    static_cast<void>(evaluate_detailed(pos_doubled, bd_doubled));
    EXPECT_LT(bd_doubled.pawns.mg, 0);
}

TEST_F(EvalTest, IsolatedPawnPenalty) {
    Position pos_isolated;
    // White has isolated d4 pawn (no c or e pawns); Black has d7, e7 connected
    pos_isolated.set_fen("4k3/3pp3/8/8/3P4/8/8/4K3 w - - 0 1");
    EvalBreakdown bd;
    static_cast<void>(evaluate_detailed(pos_isolated, bd));
    EXPECT_LT(bd.pawns.mg, 0);
}

TEST_F(EvalTest, PassedPawnRankBonus) {
    Position pos_passer;
    // White has passed pawn on d6; Black has pawn on a7
    pos_passer.set_fen("4k3/p7/3P4/8/8/8/8/4K3 w - - 0 1");
    EvalBreakdown bd;
    static_cast<void>(evaluate_detailed(pos_passer, bd));
    EXPECT_GT(bd.pawns.mg, 0);
    EXPECT_GT(bd.pawns.eg, 50);
}

TEST_F(EvalTest, KingShelterProtection) {
    Position pos_sheltered;
    // Castled White king on g1 behind f2, g2, h2 shield; Black king on e8
    pos_sheltered.set_fen("4k3/8/8/8/8/8/5PPP/6K1 w - - 0 1");
    EvalBreakdown bd_sheltered;
    static_cast<void>(evaluate_detailed(pos_sheltered, bd_sheltered));

    Position pos_exposed;
    // White king on g1 with open files and no shield pawns; Black king on e8
    pos_exposed.set_fen("4k3/8/8/8/8/8/8/6K1 w - - 0 1");
    EvalBreakdown bd_exposed;
    static_cast<void>(evaluate_detailed(pos_exposed, bd_exposed));

    EXPECT_GT(bd_sheltered.king_safety.mg, bd_exposed.king_safety.mg);
}

TEST_F(EvalTest, MobilityOpenVsRestricted) {
    Position pos_open;
    // White bishop on e4 with open diagonals
    pos_open.set_fen("4k3/8/8/8/4B3/8/8/4K3 w - - 0 1");
    EvalBreakdown bd_open;
    static_cast<void>(evaluate_detailed(pos_open, bd_open));

    Position pos_blocked;
    // White bishop on e4 blocked by friendly pawns on d5, f5, d3, f3
    pos_blocked.set_fen("4k3/8/8/3P1P2/4B3/3P1P2/8/4K3 w - - 0 1");
    EvalBreakdown bd_blocked;
    static_cast<void>(evaluate_detailed(pos_blocked, bd_blocked));

    EXPECT_GT(bd_open.mobility.mg, bd_blocked.mobility.mg);
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
        EXPECT_EQ(bd.pawns.mg, -mirrored_bd.pawns.mg) << "MG Pawns mismatch for FEN: " << fen;
        EXPECT_EQ(bd.pawns.eg, -mirrored_bd.pawns.eg) << "EG Pawns mismatch for FEN: " << fen;
        EXPECT_EQ(bd.king_safety.mg, -mirrored_bd.king_safety.mg) << "MG KingSafety mismatch for FEN: " << fen;
        EXPECT_EQ(bd.king_safety.eg, -mirrored_bd.king_safety.eg) << "EG KingSafety mismatch for FEN: " << fen;
        EXPECT_EQ(bd.mobility.mg, -mirrored_bd.mobility.mg) << "MG Mobility mismatch for FEN: " << fen;
        EXPECT_EQ(bd.mobility.eg, -mirrored_bd.mobility.eg) << "EG Mobility mismatch for FEN: " << fen;
        EXPECT_EQ(bd.game_phase, mirrored_bd.game_phase) << "Phase mismatch for FEN: " << fen;
    }
}
