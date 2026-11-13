#!/usr/bin/env python3
"""
Unit tests for Lookahead Benchmark Profiler parser.
"""

import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from run_bench import parse_benchmark_output, PositionBenchRecord, BenchmarkReport


SAMPLE_BENCHMARK_OUTPUT = """
======================================================================
Lookahead Search Benchmark (Depth 3, Hash 16 MB)
Positions: 12
======================================================================
[ 1/12] Startpos                           | D: 3  | nodes: 441      | time: 1.6    ms | nps: 276801    | move: g1f3
[ 2/12] Kiwipete (tactical complex)        | D: 3  | nodes: 4638     | time: 31.0   ms | nps: 149736    | move: d5e6
[ 3/12] Position 3 (pawns and pins)        | D: 3  | nodes: 723      | time: 2.2    ms | nps: 321719    | move: b4c4
======================================================================
Total Nodes:    5802
Total Time:     34.80 ms (0.03 s)
Aggregate NPS:  166724 nodes/sec
======================================================================
"""


class BenchmarkProfilerTest(unittest.TestCase):
    def test_parse_sample_output(self) -> None:
        report = parse_benchmark_output(SAMPLE_BENCHMARK_OUTPUT, depth=3, hash_mb=16)

        self.assertEqual(report.depth, 3)
        self.assertEqual(report.hash_mb, 16)
        self.assertEqual(len(report.positions), 3)

        pos1 = report.positions[0]
        self.assertEqual(pos1.index, 1)
        self.assertEqual(pos1.total_positions, 12)
        self.assertEqual(pos1.name, "Startpos")
        self.assertEqual(pos1.depth, 3)
        self.assertEqual(pos1.nodes, 441)
        self.assertAlmostEqual(pos1.time_ms, 1.6, places=1)
        self.assertEqual(pos1.nps, 276801)
        self.assertEqual(pos1.best_move, "g1f3")

        self.assertEqual(report.total_nodes, 5802)
        self.assertAlmostEqual(report.total_time_ms, 34.80, places=1)
        self.assertEqual(report.aggregate_nps, 166724)


if __name__ == "__main__":
    unittest.main()
