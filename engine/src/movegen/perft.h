/*
 * What: Perft (Performance Test) node counter, move divide analyzer, and throughput timer.
 * Why:  Rigorous move generation validation against standardized ground-truth trees.
 * Layer: Layer 1 (movegen) - depends only on core types, move, position, and movegen.
 */

#ifndef LOOKAHEAD_MOVEGEN_PERFT_H
#define LOOKAHEAD_MOVEGEN_PERFT_H

#include <cstdint>
#include <vector>
#include "core/types.h"
#include "core/move.h"
#include "core/position.h"

namespace lookahead::movegen {

struct PerftDivideEntry {
    core::Move move;
    uint64_t nodes = 0;
};

struct PerftResult {
    uint64_t nodes = 0;
    double elapsed_seconds = 0.0;
    double nps = 0.0;
};

// Recursively counts total legal leaf positions up to the specified depth.
[[nodiscard]] uint64_t perft(core::Position& pos, int depth) noexcept;

// Generates node counts broken down by root move for tree divergence debugging.
[[nodiscard]] std::vector<PerftDivideEntry> perft_divide(core::Position& pos, int depth);

// Executes timed perft and returns total nodes, duration, and nodes-per-second (NPS).
[[nodiscard]] PerftResult run_perft(core::Position& pos, int depth);

}  // namespace lookahead::movegen

#endif  // LOOKAHEAD_MOVEGEN_PERFT_H

