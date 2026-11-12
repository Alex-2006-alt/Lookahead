#!/usr/bin/env python3
"""
Unit tests for Lookahead match runner and opening book utilities.
Validates FEN parsing, opening suite integrity, and Elo statistical calculations.
"""

import sys
import unittest
import math
from pathlib import Path
from match_runner import load_openings, compute_elo, parse_options
import chess


class MatchRunnerTest(unittest.TestCase):
    def setUp(self):
        self.repo_root = Path(__file__).resolve().parent.parent
        self.openings_path = self.repo_root / "data" / "openings.epd"

    def test_openings_file_exists_and_parses(self):
        self.assertTrue(self.openings_path.is_file(), "data/openings.epd not found")
        openings = load_openings(self.openings_path)
        self.assertGreaterEqual(len(openings), 20, "Expected at least 20 openings")

        for name, fen in openings:
            board = chess.Board(fen)
            self.assertTrue(board.is_valid(), f"Invalid FEN for {name}: {fen}")
            self.assertFalse(board.is_game_over(), f"Game already over in opening {name}")

    def test_parse_options(self):
        opts = parse_options("Hash=32;Clear Hash;Threads=1")
        self.assertEqual(opts.get("Hash"), "32")
        self.assertEqual(opts.get("Threads"), "1")
        self.assertNotIn("Clear Hash", opts)

    def test_compute_elo_symmetric_draw(self):
        elo, err = compute_elo(wins=10, draws=20, losses=10)
        self.assertAlmostEqual(elo, 0.0, places=2)
        self.assertGreater(err, 0.0)

    def test_compute_elo_advantage(self):
        # 75% score corresponds to ~ +190.8 Elo
        elo, err = compute_elo(wins=30, draws=0, losses=10)
        expected = -400.0 * math.log10(1.0 / 0.75 - 1.0)
        self.assertAlmostEqual(elo, expected, places=1)
        self.assertGreater(err, 0.0)

    def test_compute_elo_extremes(self):
        elo_win, _ = compute_elo(wins=10, draws=0, losses=0)
        self.assertEqual(elo_win, 800.0)
        elo_loss, _ = compute_elo(wins=0, draws=0, losses=10)
        self.assertEqual(elo_loss, -800.0)


if __name__ == "__main__":
    unittest.main()
