#!/usr/bin/env python3
"""
Lookahead Search Benchmark & NPS Calibration Profiler.

Executes the native benchmark suite across 12 standard positions, parses
per-position telemetry (nodes, time, NPS, best moves), and reports aggregate
search throughput metrics.
"""

import sys
import re
import json
import argparse
import subprocess
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional


@dataclass
class PositionBenchRecord:
    index: int
    total_positions: int
    name: str
    depth: int
    nodes: int
    time_ms: float
    nps: int
    best_move: str


@dataclass
class BenchmarkReport:
    depth: int
    hash_mb: int
    total_nodes: int
    total_time_ms: float
    aggregate_nps: int
    positions: list[PositionBenchRecord]


POSITION_REGEX = re.compile(
    r"^\[\s*(\d+)\s*/\s*(\d+)\s*\]\s+(.*?)\s+\|\s+D:\s*(\d+)\s+\|\s+nodes:\s*(\d+)\s+\|\s+time:\s*([\d\.]+)\s+ms\s+\|\s+nps:\s*(\d+)\s+\|\s+move:\s*(\S+)"
)


def parse_benchmark_output(output: str, depth: int, hash_mb: int) -> BenchmarkReport:
    positions: list[PositionBenchRecord] = []
    total_nodes = 0
    total_time_ms = 0.0
    agg_nps = 0

    for line in output.splitlines():
        line = line.strip()
        pos_match = POSITION_REGEX.match(line)
        if pos_match:
            idx, total, name, d, nodes, t_ms, nps, move = pos_match.groups()
            rec = PositionBenchRecord(
                index=int(idx),
                total_positions=int(total),
                name=name.strip(),
                depth=int(d),
                nodes=int(nodes),
                time_ms=float(t_ms),
                nps=int(nps),
                best_move=move.strip(),
            )
            positions.append(rec)
            continue

        if line.startswith("Total Nodes:"):
            parts = line.split(":")
            if len(parts) > 1:
                try:
                    total_nodes = int(parts[1].strip())
                except ValueError:
                    pass
        elif line.startswith("Total Time:"):
            match = re.search(r"([\d\.]+)\s+ms", line)
            if match:
                total_time_ms = float(match.group(1))
        elif line.startswith("Aggregate NPS:"):
            match = re.search(r"(\d+)\s+nodes/sec", line)
            if match:
                agg_nps = int(match.group(1))

    return BenchmarkReport(
        depth=depth,
        hash_mb=hash_mb,
        total_nodes=total_nodes,
        total_time_ms=total_time_ms,
        aggregate_nps=agg_nps,
        positions=positions,
    )


def run_benchmark(binary_path: Path, depth: int = 5, hash_mb: int = 16) -> BenchmarkReport:
    if not binary_path.is_file():
        alt = Path(str(binary_path).replace(".exe", ""))
        if alt.is_file():
            binary_path = alt
        else:
            raise FileNotFoundError(f"Binary not found: {binary_path}")

    proc = subprocess.run(
        [str(binary_path), "bench", str(depth), str(hash_mb)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=True,
    )
    return parse_benchmark_output(proc.stdout, depth, hash_mb)


def main() -> int:
    parser = argparse.ArgumentParser(description="Lookahead Engine Benchmark Profiler")
    parser.add_argument("--binary", type=Path, default=Path("build/lookahead.exe"))
    parser.add_argument("--depth", type=int, default=5, help="Search depth per position")
    parser.add_argument("--hash", type=int, default=16, help="Transposition table size in MB")
    parser.add_argument("--json", type=Path, default=None, help="Save report to JSON file")
    parser.add_argument("--min-nps", type=int, default=50000, help="Minimum acceptable aggregate NPS")
    args = parser.parse_args()

    print(f"Running Lookahead benchmark: {args.binary} (depth {args.depth}, hash {args.hash} MB)...")
    try:
        report = run_benchmark(args.binary, args.depth, args.hash)
    except Exception as e:
        print(f"Error running benchmark: {e}", file=sys.stderr)
        return 1

    print("\n" + "=" * 68)
    print(f"BENCHMARK SUMMARY (Depth {report.depth}, Hash {report.hash_mb} MB)")
    print("=" * 68)
    print(f"Positions Tested: {len(report.positions)}")
    print(f"Total Nodes:      {report.total_nodes:,}")
    print(f"Total Time:       {report.total_time_ms:.1f} ms ({report.total_time_ms / 1000.0:.2f} s)")
    print(f"Aggregate NPS:    {report.aggregate_nps:,} nodes/sec")
    print("=" * 68)

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump(asdict(report), f, indent=2)
        print(f"Report saved to: {args.json}")

    if report.aggregate_nps < args.min_nps and report.total_time_ms > 50.0:
        print(
            f"WARNING: Aggregate NPS ({report.aggregate_nps}) is below minimum target ({args.min_nps})",
            file=sys.stderr,
        )
        return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
