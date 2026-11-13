/*
 * What: Standardized engine search benchmark suite and throughput profiler.
 * Why:  Measures search throughput (nodes, execution time, NPS) across diverse positions.
 * Layer: Layer 3 (search) - depends on core and search layers.
 */

#ifndef LOOKAHEAD_SEARCH_BENCHMARK_H
#define LOOKAHEAD_SEARCH_BENCHMARK_H

#include <cstdint>
#include <string>
#include <vector>
#include <ostream>

namespace lookahead::search {

// Single standardized benchmark position specification.
struct BenchmarkPosition {
    std::string name;
    std::string fen;
};

// Aggregated execution metrics produced by running the benchmark suite.
struct BenchmarkResult {
    uint64_t total_nodes = 0;
    double total_time_sec = 0.0;
    double nps = 0.0;
    int positions_count = 0;
};

// Returns standard curated suite of 12 diverse benchmark positions across all game phases.
const std::vector<BenchmarkPosition>& get_benchmark_suite();

// Runs benchmark across standard positions to the specified depth.
BenchmarkResult run_benchmark(int depth, std::ostream& out, int hash_mb = 16);

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_BENCHMARK_H
