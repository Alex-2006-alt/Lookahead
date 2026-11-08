/*
 * What: Implementation of perft recursive tree enumeration and divide reporting.
 * Why:  Provides automated benchmarking and move-generation correctness verification.
 * Layer: Layer 1 (movegen) - depends only on perft.h, movegen.h, and standard C++ headers.
 */

#include "perft.h"
#include <chrono>
#include "movegen.h"

namespace lookahead::movegen {

uint64_t perft(core::Position& pos, int depth) noexcept {
    if (depth <= 0) return 1ULL;

    MoveList list;
    generate_legal_moves(pos, list);

    if (depth == 1) return list.size();

    uint64_t total_nodes = 0ULL;
    core::UndoInfo undo;
    for (size_t i = 0; i < list.size(); ++i) {
        pos.make_move(list[i], undo);
        total_nodes += perft(pos, depth - 1);
        pos.unmake_move(list[i], undo);
    }
    return total_nodes;
}

std::vector<PerftDivideEntry> perft_divide(core::Position& pos, int depth) {
    std::vector<PerftDivideEntry> divide_results;
    if (depth <= 0) return divide_results;

    MoveList list;
    generate_legal_moves(pos, list);

    core::UndoInfo undo;
    for (size_t i = 0; i < list.size(); ++i) {
        pos.make_move(list[i], undo);
        const uint64_t sub_nodes = (depth == 1) ? 1ULL : perft(pos, depth - 1);
        pos.unmake_move(list[i], undo);
        divide_results.push_back({list[i], sub_nodes});
    }
    return divide_results;
}

PerftResult run_perft(core::Position& pos, int depth) {
    const auto start = std::chrono::high_resolution_clock::now();
    const uint64_t nodes = perft(pos, depth);
    const auto end = std::chrono::high_resolution_clock::now();

    const std::chrono::duration<double> elapsed = end - start;
    const double secs = elapsed.count();
    const double nps = (secs > 0.0) ? (static_cast<double>(nodes) / secs) : 0.0;

    return {nodes, secs, nps};
}

}  // namespace lookahead::movegen

