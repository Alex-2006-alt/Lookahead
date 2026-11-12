#!/usr/bin/env python3
"""
Automated UCI Match Runner & Elo Calculator for Lookahead.

Executes head-to-head tournament matches between UCI chess engines using
balanced opening books, alternates colors to eliminate first-move bias,
records full PGN game transcripts, and computes relative Elo differences
with 95% statistical confidence intervals.
"""

import sys
import math
import time
import argparse
import subprocess
from pathlib import Path
from dataclasses import dataclass
from typing import Optional, Tuple
import chess
import chess.pgn


@dataclass
class EngineConfig:
    name: str
    path: Path
    options: dict[str, str]


class UciClient:
    def __init__(self, config: EngineConfig):
        self.config = config
        self.process = subprocess.Popen(
            [str(config.path)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self._init_engine()

    def _init_engine(self) -> None:
        self.send("uci")
        self.read_until("uciok", timeout_sec=3.0)
        for opt_name, opt_val in self.config.options.items():
            self.send(f"setoption name {opt_name} value {opt_val}")
        self.send("isready")
        self.read_until("readyok", timeout_sec=3.0)

    def send(self, cmd: str) -> None:
        if self.process.poll() is not None:
            raise RuntimeError(f"Engine '{self.config.name}' terminated unexpectedly.")
        assert self.process.stdin is not None
        self.process.stdin.write(f"{cmd}\n")
        self.process.stdin.flush()

    def read_until(self, token: str, timeout_sec: float = 10.0) -> list[str]:
        lines = []
        start = time.time()
        while time.time() - start < timeout_sec:
            assert self.process.stdout is not None
            line = self.process.stdout.readline()
            if not line:
                if self.process.poll() is not None:
                    break
                time.sleep(0.005)
                continue
            line = line.strip()
            if line:
                lines.append(line)
                if token in line:
                    return lines
        raise TimeoutError(f"Engine '{self.config.name}' timed out waiting for '{token}'.")

    def play_move(self, fen: str, moves: list[str], tc_cmd: str) -> str:
        self.send(f"position fen {fen} moves {' '.join(moves)}" if moves else f"position fen {fen}")
        self.send(f"go {tc_cmd}")
        lines = self.read_until("bestmove", timeout_sec=30.0)
        bestmove_line = [l for l in lines if l.startswith("bestmove")][0]
        parts = bestmove_line.split()
        if len(parts) < 2:
            raise ValueError(f"Malformed bestmove output: {bestmove_line}")
        return parts[1]

    def close(self) -> None:
        if self.process.poll() is None:
            try:
                self.send("quit")
                self.process.wait(timeout=2.0)
            except Exception:
                self.process.kill()


def load_openings(path: Path) -> list[tuple[str, str]]:
    openings = []
    if not path.is_file():
        return [("Starting Position", chess.STARTING_FEN)]
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split("id")
            fen = parts[0].strip()
            name = parts[1].strip(' ";') if len(parts) > 1 else "Opening"
            openings.append((name, fen))
    return openings or [("Starting Position", chess.STARTING_FEN)]


def play_game(white: UciClient, black: UciClient, opening_name: str,
              start_fen: str, tc_cmd: str, max_moves: int = 150) -> Tuple[str, str, chess.pgn.Game]:
    board = chess.Board(start_fen)
    moves: list[str] = []
    game = chess.pgn.Game()
    game.headers["Event"] = "Lookahead Match"
    game.headers["White"] = white.config.name
    game.headers["Black"] = black.config.name
    game.headers["FEN"] = start_fen
    game.headers["SetUp"] = "1" if start_fen != chess.STARTING_FEN else "0"
    game.headers["Opening"] = opening_name

    white.send("ucinewgame")
    black.send("ucinewgame")
    node = game
    reason, result = "Move limit reached", "1/2-1/2"

    for _ in range(max_moves):
        if board.is_checkmate():
            result, reason = ("0-1" if board.turn == chess.WHITE else "1-0"), "Checkmate"
            break
        if board.is_stalemate() or board.is_insufficient_material() or board.is_fifty_moves() or board.is_repetition(3):
            result = "1/2-1/2"
            reason = "Stalemate" if board.is_stalemate() else ("Repetition" if board.is_repetition(3) else "Draw")
            break

        active_engine = white if board.turn == chess.WHITE else black
        try:
            move_uci = active_engine.play_move(start_fen, moves, tc_cmd)
            move = chess.Move.from_uci(move_uci)
            if move not in board.legal_moves:
                result, reason = ("0-1" if board.turn == chess.WHITE else "1-0"), f"Illegal: {move_uci}"
                break
        except Exception as e:
            result, reason = ("0-1" if board.turn == chess.WHITE else "1-0"), f"Error: {e}"
            break

        board.push(move)
        moves.append(move_uci)
        node = node.add_variation(move)

    game.headers["Result"] = result
    game.headers["Termination"] = reason
    return result, reason, game


def compute_elo(wins: int, draws: int, losses: int) -> Tuple[float, float]:
    n = wins + draws + losses
    if n == 0:
        return 0.0, 0.0
    score = (wins + 0.5 * draws) / n
    if score <= 0.0:
        return -800.0, 0.0
    if score >= 1.0:
        return 800.0, 0.0

    elo_diff = -400.0 * math.log10((1.0 / score) - 1.0)
    var = ((wins * ((1.0 - score) ** 2)) +
           (draws * ((0.5 - score) ** 2)) +
           (losses * ((0.0 - score) ** 2))) / max(1, n - 1)
    error_95 = 1.96 * (math.sqrt(var) / math.sqrt(n)) * (400.0 * math.log(10.0)) / (score * (1.0 - score))
    return elo_diff, error_95


def parse_options(opts_str: Optional[str]) -> dict[str, str]:
    if not opts_str:
        return {}
    res = {}
    for part in opts_str.split(";"):
        if "=" in part:
            k, v = part.split("=", 1)
            res[k.strip()] = v.strip()
    return res


def main() -> int:
    parser = argparse.ArgumentParser(description="Lookahead UCI Head-to-Head Match Runner")
    parser.add_argument("--engine1-path", type=Path, default=Path("build/lookahead.exe"))
    parser.add_argument("--engine1-name", type=str, default="Lookahead-A")
    parser.add_argument("--engine1-options", type=str, default="")
    parser.add_argument("--engine2-path", type=Path, default=Path("build/lookahead.exe"))
    parser.add_argument("--engine2-name", type=str, default="Lookahead-B")
    parser.add_argument("--engine2-options", type=str, default="")
    parser.add_argument("--openings", type=Path, default=Path("data/openings.epd"))
    parser.add_argument("--games", type=int, default=10)
    parser.add_argument("--tc", type=str, default="depth 3")
    parser.add_argument("--pgn", type=Path, default=None)
    args = parser.parse_args()

    for attr in ["engine1_path", "engine2_path"]:
        p = getattr(args, attr)
        if not p.is_file() and Path(str(p).replace(".exe", "")).is_file():
            setattr(args, attr, Path(str(p).replace(".exe", "")))

    cfg1 = EngineConfig(args.engine1_name, args.engine1_path, parse_options(args.engine1_options))
    cfg2 = EngineConfig(args.engine2_name, args.engine2_path, parse_options(args.engine2_options))
    openings = load_openings(args.openings)

    print("=" * 60)
    print(f"Match: {cfg1.name} vs {cfg2.name}")
    print(f"Time Control: go {args.tc} | Openings: {len(openings)} | Games: {args.games}")
    print("=" * 60)

    e1, e2 = UciClient(cfg1), UciClient(cfg2)
    wins = draws = losses = 0
    all_games: list[chess.pgn.Game] = []

    try:
        for i in range(args.games):
            op_name, op_fen = openings[i % len(openings)]
            is_e1_white = (i % 2 == 0) if (i // 2) % 2 == 0 else (i % 2 != 0)
            white, black = (e1, e2) if is_e1_white else (e2, e1)
            result_str, reason, pgn_game = play_game(white, black, op_name, op_fen, args.tc)
            all_games.append(pgn_game)

            if result_str == "1-0":
                if is_e1_white: wins += 1
                else: losses += 1
            elif result_str == "0-1":
                if is_e1_white: losses += 1
                else: wins += 1
            else:
                draws += 1

            winner = "Draw"
            if (result_str == "1-0" and is_e1_white) or (result_str == "0-1" and not is_e1_white):
                winner = cfg1.name
            elif (result_str == "0-1" and is_e1_white) or (result_str == "1-0" and not is_e1_white):
                winner = cfg2.name

            print(f"Game {i+1:02d}/{args.games:02d}: {cfg1.name} ({'W' if is_e1_white else 'B'}) vs "
                  f"{cfg2.name} ({'B' if is_e1_white else 'W'}) | {result_str:7s} -> {winner:12s} "
                  f"({reason}) [Score: {wins}-{draws}-{losses}]")
    finally:
        e1.close()
        e2.close()

    if args.pgn:
        args.pgn.parent.mkdir(parents=True, exist_ok=True)
        with open(args.pgn, "w", encoding="utf-8") as f:
            for g in all_games:
                f.write(str(g) + "\n\n")
        print(f"\nSaved PGN transcript with {len(all_games)} games to {args.pgn}")

    elo_diff, elo_err = compute_elo(wins, draws, losses)
    score_pct = ((wins + 0.5 * draws) / max(1, args.games)) * 100.0

    print("\n" + "=" * 60)
    print("FINAL TOURNAMENT RESULTS")
    print("=" * 60)
    print(f"Match: {cfg1.name} vs {cfg2.name}")
    print(f"Total Games: {args.games}")
    print(f"Score:       {wins} W - {draws} D - {losses} L ({score_pct:.1f}%)")
    print(f"Elo Diff:    {elo_diff:+.1f} +/- {elo_err:.1f} (95% CI)")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
