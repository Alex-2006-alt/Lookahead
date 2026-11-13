/*
 * What: Implementation of standardized search benchmark suite.
 * Why:  Provides deterministic throughput calibration and regression profiling across game phases.
 * Layer: Layer 3 (search) - depends on benchmark.h, core/position, and search/search.
 */

#include "benchmark.h"
#include <chrono>
#include <iomanip>
#include "core/position.h"
#include "search/search.h"
#include "search/transposition_table.h"
#include "search/features.h"

namespace lookahead::search {

const std::vector<BenchmarkPosition>& get_benchmark_suite() {
    static const std::vector<BenchmarkPosition> suite = {
        {"Startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
        {"Kiwipete (tactical complex)", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"},
        {"Position 3 (pawns and pins)", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"},
        {"Position 4 (promotions & castling)", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"},
        {"Position 5 (fine tactical endgame)", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"},
        {"Position 6 (middlegame complex)", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"},
        {"King and Pawn Endgame", "8/8/4k3/8/8/4K3/4P3/8 w - - 0 1"},
        {"Rook Endgame", "r5k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1"},
        {"Queen and Pawn Endgame", "8/5pk1/8/8/8/8/1Q4K1/8 b - - 0 1"},
        {"Knight Endgame", "8/8/2n5/4k3/8/8/4NK2/8 w - - 0 1"},
        {"Bishop Endgame", "8/8/2b5/4k3/8/8/4BK2/8 w - - 0 1"},
        {"King Safety & Castling", "r1bq1rk1/pp2ppbp/2np1np1/8/3NP3/2N1BP2/PPP3PP/2KR1B1R b - - 0 9"},
    };
    return suite;
}

BenchmarkResult run_benchmark(int depth, std::ostream& out, int hash_mb) {
    if (depth <= 0) depth = 5;
    if (hash_mb <= 0) hash_mb = 16;

    reset_features_to_default();
    global_tt.resize(hash_mb);
    global_tt.clear();

    const auto& suite = get_benchmark_suite();
    BenchmarkResult aggregate;
    aggregate.positions_count = static_cast<int>(suite.size());

    out << "======================================================================\n";
    out << "Lookahead Search Benchmark (Depth " << depth << ", Hash " << hash_mb << " MB)\n";
    out << "Positions: " << suite.size() << "\n";
    out << "======================================================================\n";

    const auto start_total = std::chrono::steady_clock::now();

    for (size_t i = 0; i < suite.size(); ++i) {
        const auto& item = suite[i];
        core::Position pos;
        if (!pos.set_fen(item.fen)) {
            out << "[" << std::setw(2) << (i + 1) << "/" << suite.size() << "] "
                << item.name << " -> FAILED (Invalid FEN)\n";
            continue;
        }

        SearchLimits limits;
        limits.depth = depth;

        const auto pos_start = std::chrono::steady_clock::now();
        const auto res = search(pos, limits);
        const auto pos_end = std::chrono::steady_clock::now();
        const double pos_sec = std::chrono::duration<double>(pos_end - pos_start).count();
        const double pos_nps = (pos_sec > 0.0) ? (static_cast<double>(res.nodes) / pos_sec) : 0.0;

        aggregate.total_nodes += res.nodes;

        out << "[" << std::right << std::setw(2) << (i + 1) << "/" << suite.size() << "] "
            << std::left << std::setw(34) << item.name
            << " | D: " << std::setw(2) << res.depth_reached
            << " | nodes: " << std::setw(8) << res.nodes
            << " | time: " << std::fixed << std::setprecision(1) << std::setw(6) << (pos_sec * 1000.0) << " ms"
            << " | nps: " << std::setw(9) << static_cast<uint64_t>(pos_nps)
            << " | move: " << res.best_move.to_uci() << "\n";
        out.flush();
    }

    const auto end_total = std::chrono::steady_clock::now();
    aggregate.total_time_sec = std::chrono::duration<double>(end_total - start_total).count();
    aggregate.nps = (aggregate.total_time_sec > 0.0)
                        ? (static_cast<double>(aggregate.total_nodes) / aggregate.total_time_sec)
                        : 0.0;

    out << "======================================================================\n";
    out << "Total Nodes:    " << aggregate.total_nodes << "\n";
    out << "Total Time:     " << std::fixed << std::setprecision(2) << (aggregate.total_time_sec * 1000.0) << " ms ("
        << aggregate.total_time_sec << " s)\n";
    out << "Aggregate NPS:  " << static_cast<uint64_t>(aggregate.nps) << " nodes/sec\n";
    out << "======================================================================\n";
    out.flush();

    return aggregate;
}

}  // namespace lookahead::search

