#!/usr/bin/env python3
"""
Unit tests for Lookahead Ablation Study Runner.
"""

import sys
import unittest
from pathlib import Path

# Add tools directory to path
TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from run_ablation import (
    ABLATION_SUITE,
    AblationConfig,
    AblationResult,
    format_markdown_table,
)


class AblationRunnerTest(unittest.TestCase):
    def test_ablation_suite_contains_all_core_heuristics(self) -> None:
        keys = [cfg.key for cfg in ABLATION_SUITE]
        self.assertIn("no_tt", keys)
        self.assertIn("no_ordering", keys)
        self.assertIn("no_qsearch", keys)
        self.assertIn("no_nmp", keys)
        self.assertIn("no_lmr", keys)

    def test_ablation_configs_have_valid_options(self) -> None:
        for cfg in ABLATION_SUITE:
            self.assertTrue(len(cfg.options) >= 1)
            for opt_name, opt_val in cfg.options.items():
                self.assertIn(opt_val, ["false", "0"])

    def test_ablation_result_score_percentage(self) -> None:
        res = AblationResult(
            config_name="Test Variant",
            games=10,
            baseline_wins=7,
            draws=2,
            variant_wins=1,
            baseline_elo_advantage=240.5,
            elo_margin_95=45.2,
        )
        self.assertAlmostEqual(res.score_percent, 80.0, places=2)

    def test_format_markdown_table(self) -> None:
        results = [
            AblationResult(
                config_name="No Transposition Table",
                games=10,
                baseline_wins=8,
                draws=1,
                variant_wins=1,
                baseline_elo_advantage=280.0,
                elo_margin_95=60.0,
            )
        ]
        table = format_markdown_table(results)
        self.assertIn("Full Engine (Baseline)", table)
        self.assertIn("No Transposition Table", table)
        self.assertIn("8 / 1 / 1", table)
        self.assertIn("+280.0 ± 60.0", table)


if __name__ == "__main__":
    unittest.main()
