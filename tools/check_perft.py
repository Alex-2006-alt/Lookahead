#!/usr/bin/env python3
"""
Perft Suite Benchmark and Correctness Verifier for Lookahead.

Parses data/perft_positions.epd, executes perft via the Lookahead native binary,
asserts exact node count equality against the ground-truth oracle dataset,
and reports nodes-per-second (NPS) throughput.
"""

import sys
import subprocess
import re
from pathlib import Path


def find_binary() -> Path:
    repo_root = Path(__file__).resolve().parent.parent
    candidates = [
        repo_root / "build" / "lookahead.exe",
        repo_root / "build" / "lookahead",
        repo_root / "engine" / "build" / "lookahead.exe",
        repo_root / "engine" / "build" / "lookahead",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError("Lookahead binary not found. Build the project first.")


def run_perft(binary: Path, fen: str, depth: int) -> tuple[int, float, int]:
    cmd = [str(binary), "perft", str(depth), fen]
    res = subprocess.run(cmd, capture_output=True, text=True, check=True)
    nodes = 0
    time_ms = 0.0
    nps = 0
    for line in res.stdout.splitlines():
        if line.startswith("Nodes:"):
            nodes = int(line.split(":")[1].strip())
        elif line.startswith("Time:"):
            time_ms = float(line.split(":")[1].replace("ms", "").strip())
        elif line.startswith("NPS:"):
            nps = int(line.split(":")[1].strip())
    return nodes, time_ms, nps


def main():
    repo_root = Path(__file__).resolve().parent.parent
    epd_path = repo_root / "data" / "perft_positions.epd"
    if not epd_path.is_file():
        print(f"EPD dataset not found at {epd_path}", file=sys.stderr)
        sys.exit(1)

    binary = find_binary()
    print(f"Using Lookahead binary: {binary}")
    print(f"Reading perft test cases from: {epd_path.name}\n" + "=" * 70)

    total_nodes = 0
    total_time_ms = 0.0
    all_passed = True

    lines = epd_path.read_text(encoding="utf-8").splitlines()
    for line in lines:
        line = line.strip()
        if not line or line.startswith("#"):
            continue

        parts = line.split(";")
        fen = parts[0].strip()

        desc_match = re.search(r'id\s+"([^"]+)"', line)
        desc = desc_match.group(1) if desc_match else fen

        print(f"\nPosition: {desc}")
        print(f"FEN: {fen}")

        depth_matches = re.findall(r"D(\d+)\s+(\d+)", line)
        for depth_str, expected_nodes_str in depth_matches:
            depth = int(depth_str)
            expected_nodes = int(expected_nodes_str)

            nodes, time_ms, nps = run_perft(binary, fen, depth)
            total_nodes += nodes
            total_time_ms += time_ms

            status = "PASS" if nodes == expected_nodes else "FAIL"
            print(
                f"  Depth {depth}: {nodes:>10} nodes | {time_ms:>7.2f} ms | "
                f"{nps:>10} NPS | [{status}]"
            )

            if nodes != expected_nodes:
                print(
                    f"    ERROR: Expected {expected_nodes}, got {nodes} (diff: {nodes - expected_nodes})"
                )
                all_passed = False

    print("\n" + "=" * 70)
    avg_nps = int(total_nodes / (total_time_ms / 1000.0)) if total_time_ms > 0 else 0
    print(
        f"Summary: Total Nodes: {total_nodes:,} | Total Time: {total_time_ms:.2f} ms | Overall NPS: {avg_nps:,}"
    )

    if all_passed:
        print("\nALL PERFT SUITE POSITIONS VERIFIED SUCCESSFULLY!")
        sys.exit(0)
    else:
        print("\nPERFT SUITE VERIFICATION FAILED!", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

