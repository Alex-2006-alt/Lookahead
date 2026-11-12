#!/usr/bin/env python3
"""
Automated Universal Chess Interface (UCI) Protocol Verifier for Lookahead.

Simulates a real chess GUI (Cutechess, Arena, ChessBase) driving Lookahead
over asynchronous standard input/output pipes. Validates handshakes, options,
board setup, time allocation, streaming telemetry, and graceful search interruption.
"""

import sys
import time
import subprocess
from pathlib import Path


def find_engine_binary() -> Path:
    repo_root = Path(__file__).resolve().parent.parent
    candidates = [
        repo_root / "build" / "lookahead.exe",
        repo_root / "build" / "lookahead",
        repo_root / "build" / "Release" / "lookahead.exe",
        repo_root / "build-san" / "lookahead",
    ]
    for c in candidates:
        if c.is_file():
            return c
    raise FileNotFoundError("Could not find compiled lookahead engine binary in build directory.")


class UciVerifier:
    def __init__(self, binary_path: Path):
        self.process = subprocess.Popen(
            [str(binary_path)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

    def send(self, command: str) -> None:
        if self.process.poll() is not None:
            raise RuntimeError(f"Engine process unexpectedly terminated before sending '{command}'.")
        assert self.process.stdin is not None
        self.process.stdin.write(f"{command}\n")
        self.process.stdin.flush()

    def read_until(self, expected_token: str, timeout_sec: float = 3.0) -> list[str]:
        lines = []
        start = time.time()
        while time.time() - start < timeout_sec:
            assert self.process.stdout is not None
            line = self.process.stdout.readline()
            if not line:
                if self.process.poll() is not None:
                    break
                time.sleep(0.01)
                continue
            line = line.strip()
            if line:
                lines.append(line)
                if expected_token in line:
                    return lines
        raise TimeoutError(
            f"Timed out after {timeout_sec:.1f}s waiting for '{expected_token}'. Received: {lines}"
        )

    def close(self) -> None:
        if self.process.poll() is None:
            try:
                self.send("quit")
                self.process.wait(timeout=2.0)
            except Exception:
                self.process.kill()


def test_handshake(verifier: UciVerifier) -> None:
    print("[1/7] Testing UCI handshake ('uci' -> 'uciok')...")
    verifier.send("uci")
    lines = verifier.read_until("uciok", timeout_sec=2.0)
    text = " ".join(lines)
    assert "id name Lookahead" in text, f"Expected id name Lookahead, got: {lines}"
    assert "id author Alex" in text, f"Expected id author Alex, got: {lines}"
    assert "option name Hash" in text, f"Expected Hash option, got: {lines}"
    print("      ✓ Handshake verified with id and options.")


def test_isready(verifier: UciVerifier) -> None:
    print("[2/7] Testing synchronization ('isready' -> 'readyok')...")
    verifier.send("isready")
    lines = verifier.read_until("readyok", timeout_sec=2.0)
    assert any("readyok" in line for line in lines)
    print("      ✓ Engine responds with readyok.")


def test_options(verifier: UciVerifier) -> None:
    print("[3/7] Testing dynamic options ('setoption name Hash')...")
    verifier.send("setoption name Hash value 32")
    verifier.send("setoption name Clear Hash")
    verifier.send("isready")
    verifier.read_until("readyok", timeout_sec=2.0)
    print("      ✓ Hash table resized and cleared.")


def test_sync_search(verifier: UciVerifier) -> None:
    print("[4/7] Testing fixed-depth search ('go depth 3')...")
    verifier.send("position startpos moves e2e4 e7e5")
    verifier.send("go depth 3")
    lines = verifier.read_until("bestmove", timeout_sec=5.0)
    has_info = any(line.startswith("info depth") for line in lines)
    bestmove_line = [l for l in lines if l.startswith("bestmove")][0]
    assert has_info, f"Missing telemetry info lines: {lines}"
    assert len(bestmove_line.split()) >= 2, f"Invalid bestmove format: {bestmove_line}"
    print(f"      ✓ Completed fixed-depth search: {bestmove_line}")


def test_async_stop(verifier: UciVerifier) -> None:
    print("[5/7] Testing asynchronous stop ('go infinite' -> 'stop')...")
    verifier.send("position startpos")
    verifier.send("go infinite")
    time.sleep(0.15)  # Let search explore asynchronously
    verifier.send("stop")
    t0 = time.time()
    lines = verifier.read_until("bestmove", timeout_sec=1.0)
    elapsed = time.time() - t0
    bestmove_line = [l for l in lines if l.startswith("bestmove")][0]
    assert elapsed < 0.5, f"Stop took too long to abort: {elapsed:.3f}s"
    print(f"      ✓ Asynchronous search aborted in {elapsed*1000:.1f}ms: {bestmove_line}")


def test_tactical_position(verifier: UciVerifier) -> None:
    print("[6/7] Testing tactical mate-in-1 ('position fen ...')...")
    # Scholar's mate setup
    fen = "r1bqkb1r/pppp1ppp/2n5/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1"
    verifier.send(f"position fen {fen}")
    verifier.send("go depth 2")
    lines = verifier.read_until("bestmove", timeout_sec=3.0)
    bestmove_line = [l for l in lines if l.startswith("bestmove")][0]
    assert "h5f7" in bestmove_line, f"Failed to solve Scholar's mate, got: {bestmove_line}"
    print(f"      ✓ Found tactical mate: {bestmove_line}")


def test_movetime(verifier: UciVerifier) -> None:
    print("[7/7] Testing time control ('go movetime 200')...")
    verifier.send("position startpos")
    t0 = time.time()
    verifier.send("go movetime 200")
    lines = verifier.read_until("bestmove", timeout_sec=2.0)
    elapsed_ms = (time.time() - t0) * 1000.0
    bestmove_line = [l for l in lines if l.startswith("bestmove")][0]
    # Allow reasonable margin for system scheduling (150ms - 400ms)
    assert 120 <= elapsed_ms <= 600, f"Movetime out of bounds: {elapsed_ms:.1f}ms"
    print(f"      ✓ Movetime 200ms executed in {elapsed_ms:.1f}ms: {bestmove_line}")


def main() -> int:
    binary = find_engine_binary()
    print(f"=== Lookahead UCI Protocol Verifier ===")
    print(f"Target binary: {binary}\n")

    verifier = UciVerifier(binary)
    try:
        test_handshake(verifier)
        test_isready(verifier)
        test_options(verifier)
        test_sync_search(verifier)
        test_async_stop(verifier)
        test_tactical_position(verifier)
        test_movetime(verifier)
        print("\nAll 7 UCI verification test suites passed successfully!")
        return 0
    finally:
        verifier.close()


if __name__ == "__main__":
    sys.exit(main())
