#!/usr/bin/env python3
"""
Differential Move Generation Fuzzer for Lookahead.

Plays random games against the python-chess ground-truth oracle,
asserting that Lookahead's strictly legal move generator produces
an identical legal move set in every single position.
"""

import sys
import argparse
import random
import subprocess
from pathlib import Path

try:
    import chess
except ImportError:
    print("Error: python-chess is required for compare_movegen.py", file=sys.stderr)
    print("Install via: pip install chess", file=sys.stderr)
    sys.exit(1)


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


def main():
    parser = argparse.ArgumentParser(description="Differential movegen fuzzer")
    parser.add_argument(
        "--positions",
        type=int,
        default=10000,
        help="Number of random positions to test (default: 10000)",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducibility (default: 42)",
    )
    args = parser.parse_args()

    random.seed(args.seed)
    binary = find_binary()

    print(f"Differential Fuzzer: Lookahead vs. python-chess")
    print(f"Target positions: {args.positions:,} | Seed: {args.seed}")
    print(f"Engine binary: {binary}\n" + "=" * 60)

    # Launch Lookahead process in batch mode
    proc = subprocess.Popen(
        [str(binary), "moves", "-"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

    seed_fens = [
        chess.STARTING_FEN,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "8/8/1k6/2b5/2n1P3/8/1K5B/8 w - - 0 1",
    ]

    verified = 0
    games = 0

    try:
        while verified < args.positions:
            games += 1
            board = chess.Board(random.choice(seed_fens))

            # Play random moves until game over or 120 ply
            for _ in range(120):
                if verified >= args.positions:
                    break

                fen = board.fen()
                oracle_moves = sorted(m.uci() for m in board.legal_moves)

                # Send FEN to Lookahead
                proc.stdin.write(fen + "\n")
                proc.stdin.flush()
                engine_output = proc.stdout.readline().strip()

                engine_moves = sorted(engine_output.split()) if engine_output else []

                if oracle_moves != engine_moves:
                    print(f"\n[MISMATCH DETECTED at position #{verified + 1}]", file=sys.stderr)
                    print(f"FEN: {fen}", file=sys.stderr)
                    print(f"Oracle moves ({len(oracle_moves)}): {' '.join(oracle_moves)}", file=sys.stderr)
                    print(f"Engine moves ({len(engine_moves)}): {' '.join(engine_moves)}", file=sys.stderr)
                    missing = set(oracle_moves) - set(engine_moves)
                    extra = set(engine_moves) - set(oracle_moves)
                    if missing:
                        print(f"Missing from engine: {' '.join(sorted(missing))}", file=sys.stderr)
                    if extra:
                        print(f"Unexpected extra in engine: {' '.join(sorted(extra))}", file=sys.stderr)
                    proc.terminate()
                    sys.exit(1)

                verified += 1
                if verified % 2000 == 0 or verified == args.positions:
                    print(f"  Verified {verified:>6,} / {args.positions:,} positions across {games} games... [OK]")

                if board.is_game_over() or not oracle_moves:
                    break

                # Pick a random legal move to advance
                board.push(random.choice(list(board.legal_moves)))

    finally:
        if proc.poll() is None:
            proc.stdin.close()
            proc.terminate()
            proc.wait()

    print("=" * 60)
    print(f"SUCCESS: {verified:,} consecutive random positions matched python-chess move-for-move with 0 discrepancies!")
    sys.exit(0)


if __name__ == "__main__":
    main()

