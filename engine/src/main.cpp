/*
 * What: Main entry point for the Lookahead native binary executable.
 * Why:  Provides command-line interfaces for UCI protocol, perft benchmarking, and verification.
 * Layer: Root executable - coordinates core, movegen, and interface layers.
 */

#include <iostream>
#include <string>
#include <vector>
#include "core/version.h"
#include "core/position.h"
#include "movegen/attacks.h"
#include "movegen/movegen.h"
#include "movegen/perft.h"
#include "eval/evaluate.h"
#include "search/search.h"
#include "search/transposition_table.h"

using namespace lookahead;

int main(int argc, char* argv[]) {
    // Initialize attack lookup tables once at startup
    movegen::attacks::init();
    search::global_tt.resize(16);

    if (argc < 2) {
        std::cout << "id name " << core::ENGINE_NAME << " " << core::ENGINE_VERSION << "\n";
        std::cout << "id author " << core::ENGINE_AUTHOR << "\n";
        std::cout << "Usage: lookahead [uci | perft <depth> [fen] | divide <depth> [fen] | moves [fen | -]]\n";
        return 0;
    }

    const std::string command = argv[1];

    if (command == "uci") {
        std::cout << "id name " << core::ENGINE_NAME << " " << core::ENGINE_VERSION << "\n";
        std::cout << "id author " << core::ENGINE_AUTHOR << "\n";
        std::cout << "uciok\n";
        return 0;
    }

    if (command == "perft" || command == "divide") {
        int depth = 1;
        if (argc >= 3) {
            depth = std::stoi(argv[2]);
        }
        std::string fen = std::string(core::Position::START_FEN);
        if (argc >= 4) {
            fen = argv[3];
            for (int i = 4; i < argc; ++i) {
                fen += " ";
                fen += argv[i];
            }
        }

        core::Position pos;
        if (!pos.set_fen(fen)) {
            std::cerr << "Invalid FEN: " << fen << "\n";
            return 1;
        }

        if (command == "divide") {
            const auto divide_res = movegen::perft_divide(pos, depth);
            uint64_t total = 0;
            for (const auto& entry : divide_res) {
                std::cout << entry.move.to_uci() << ": " << entry.nodes << "\n";
                total += entry.nodes;
            }
            std::cout << "\nTotal nodes: " << total << "\n";
        } else {
            const auto res = movegen::run_perft(pos, depth);
            std::cout << "Nodes: " << res.nodes << "\n";
            std::cout << "Time:  " << (res.elapsed_seconds * 1000.0) << " ms\n";
            std::cout << "NPS:   " << static_cast<uint64_t>(res.nps) << "\n";
        }
        return 0;
    }

    if (command == "moves") {
        if (argc >= 3 && std::string(argv[2]) == "-") {
            std::string line;
            while (std::getline(std::cin, line)) {
                if (line.empty()) continue;
                core::Position pos;
                if (!pos.set_fen(line)) {
                    std::cout << "INVALID_FEN\n";
                    continue;
                }
                movegen::MoveList list;
                movegen::generate_legal_moves(pos, list);
                for (size_t i = 0; i < list.size(); ++i) {
                    if (i > 0) std::cout << " ";
                    std::cout << list[i].to_uci();
                }
                std::cout << "\n";
            }
            return 0;
        }

        std::string fen = std::string(core::Position::START_FEN);
        if (argc >= 3) {
            fen = argv[2];
            for (int i = 3; i < argc; ++i) {
                fen += " ";
                fen += argv[i];
            }
        }

        core::Position pos;
        if (!pos.set_fen(fen)) {
            std::cerr << "Invalid FEN: " << fen << "\n";
            return 1;
        }

        movegen::MoveList list;
        movegen::generate_legal_moves(pos, list);
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) std::cout << " ";
            std::cout << list[i].to_uci();
        }
        std::cout << "\n";
        return 0;
    }

    if (command == "eval") {
        std::string fen = std::string(core::Position::START_FEN);
        if (argc >= 3) {
            fen = argv[2];
            for (int i = 3; i < argc; ++i) {
                fen += " ";
                fen += argv[i];
            }
        }

        core::Position pos;
        if (!pos.set_fen(fen)) {
            std::cerr << "Invalid FEN: " << fen << "\n";
            return 1;
        }

        eval::EvalBreakdown bd;
        const int score = eval::evaluate_detailed(pos, bd);
        std::cout << "Evaluation (stm): " << score << " cp\n";
        std::cout << "Game Phase:       " << bd.game_phase << " / " << eval::TOTAL_PHASE << "\n";
        std::cout << "Material (mg/eg): " << bd.material.mg << " / " << bd.material.eg << " cp\n";
        std::cout << "PSQT (mg/eg):     " << bd.psqt.mg << " / " << bd.psqt.eg << " cp\n";
        std::cout << "Pawns (mg/eg):    " << bd.pawns.mg << " / " << bd.pawns.eg << " cp\n";
        std::cout << "Safety (mg/eg):   " << bd.king_safety.mg << " / " << bd.king_safety.eg << " cp\n";
        std::cout << "Mobility (mg/eg): " << bd.mobility.mg << " / " << bd.mobility.eg << " cp\n";
        return 0;
    }

    if (command == "search" || command == "go") {
        int depth = 5;
        if (argc >= 3) {
            depth = std::stoi(argv[2]);
        }
        std::string fen = std::string(core::Position::START_FEN);
        if (argc >= 4) {
            fen = argv[3];
            for (int i = 4; i < argc; ++i) {
                fen += " ";
                fen += argv[i];
            }
        }
        core::Position pos;
        if (!pos.set_fen(fen)) {
            std::cerr << "Invalid FEN: " << fen << "\n";
            return 1;
        }
        search::SearchLimits limits;
        limits.depth = depth;
        const auto res = search::search(pos, limits);
        if (!res.pv_string.empty()) {
            std::cout << "info depth " << res.depth_reached << " score cp " << res.score
                      << " nodes " << res.nodes << " nps " << static_cast<uint64_t>(res.nps)
                      << " pv " << res.pv_string << "\n";
        } else {
            std::cout << "info depth " << res.depth_reached << " score cp " << res.score
                      << " nodes " << res.nodes << " nps " << static_cast<uint64_t>(res.nps) << "\n";
        }
        std::cout << "bestmove " << res.best_move.to_uci() << "\n";
        return 0;
    }

    std::cerr << "Unknown command: " << command << "\n";
    return 1;
}
