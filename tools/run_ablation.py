#!/usr/bin/env python3
"""
Lookahead Automated Search Heuristic Ablation Study Runner.

Executes controlled matches between the full baseline engine and ablated
configurations (with individual search heuristics disabled) over curated
tournament openings, computing empirical Elo differences and statistical
confidence intervals.
"""

import sys
import json
import argparse
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional

# Support importing match_runner when invoked from root or tools/
TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from match_runner import (
    EngineConfig,
    UciClient,
    load_openings,
    play_game,
    compute_elo,
)


@dataclass
class AblationConfig:
    key: str
    name: str
    options: dict[str, str]
    description: str


ABLATION_SUITE: list[AblationConfig] = [
    AblationConfig(
        key="no_tt",
        name="No Transposition Table",
        options={"TranspositionTable": "false"},
        description="Search without transposition table cache cutoffs or move ordering hints",
    ),
    AblationConfig(
        key="no_ordering",
        name="No Move Ordering",
        options={"MoveOrdering": "false"},
        description="Search without MVV-LVA, killer moves, history heuristics, or TT moves",
    ),
    AblationConfig(
        key="no_qsearch",
        name="No Quiescence Search",
        options={"QuiescenceSearch": "false"},
        description="Search without tactical quiescence extension at horizon",
    ),
    AblationConfig(
        key="no_nmp",
        name="No Null-Move Pruning",
        options={"NullMovePruning": "false"},
        description="Search without adaptive null-move forward pruning",
    ),
    AblationConfig(
        key="no_lmr",
        name="No Late Move Reductions",
        options={"LateMoveReductions": "false"},
        description="Search without logarithmic late move reductions on quiet moves",
    ),
]


@dataclass
class AblationResult:
    config_name: str
    games: int
    baseline_wins: int
    draws: int
    variant_wins: int
    baseline_elo_advantage: float
    elo_margin_95: float

    @property
    def score_percent(self) -> float:
        return ((self.baseline_wins + 0.5 * self.draws) / max(1, self.games)) * 100.0


def run_ablation_match(
    binary_path: Path,
    ablation: AblationConfig,
    openings: list[tuple[str, str]],
    games: int,
    tc: str,
    verbose: bool = True,
) -> AblationResult:
    baseline_cfg = EngineConfig("Baseline", binary_path, {})
    variant_cfg = EngineConfig(ablation.name, binary_path, ablation.options)

    e_base = UciClient(baseline_cfg)
    e_var = UciClient(variant_cfg)

    wins = draws = losses = 0

    try:
        for i in range(games):
            op_name, op_fen = openings[i % len(openings)]
            is_base_white = (i % 2 == 0) if (i // 2) % 2 == 0 else (i % 2 != 0)
            white, black = (e_base, e_var) if is_base_white else (e_var, e_base)

            res, reason, _ = play_game(white, black, op_name, op_fen, tc)

            if res == "1-0":
                if is_base_white:
                    wins += 1
                else:
                    losses += 1
            elif res == "0-1":
                if is_base_white:
                    losses += 1
                else:
                    wins += 1
            else:
                draws += 1

            if verbose:
                print(
                    f"  [{ablation.key}] Game {i+1:02d}/{games:02d}: "
                    f"Result {res:7s} ({reason}) | Score: {wins}W - {draws}D - {losses}L"
                )
    finally:
        e_base.close()
        e_var.close()

    elo_diff, elo_err = compute_elo(wins, draws, losses)
    return AblationResult(
        config_name=ablation.name,
        games=games,
        baseline_wins=wins,
        draws=draws,
        variant_wins=losses,
        baseline_elo_advantage=elo_diff,
        elo_margin_95=elo_err,
    )


def format_markdown_table(results: list[AblationResult]) -> str:
    lines = [
        "| Configuration | Games Played | Win / Draw / Loss | Baseline Elo Advantage (±95%) | Baseline Score |",
        "| :--- | :---: | :---: | :---: | :---: |",
        "| Full Engine (Baseline) | - | - | Baseline (0.0) | 50.0% |",
    ]
    for r in results:
        record = f"{r.baseline_wins} / {r.draws} / {r.variant_wins}"
        elo = f"+{r.baseline_elo_advantage:.1f} ± {r.elo_margin_95:.1f}"
        lines.append(f"| {r.config_name} | {r.games} | {record} | {elo} | {r.score_percent:.1f}% |")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Lookahead Heuristic Ablation Study Runner")
    parser.add_argument("--binary", type=Path, default=Path("build/lookahead.exe"))
    parser.add_argument("--openings", type=Path, default=Path("data/openings.epd"))
    parser.add_argument("--games", type=int, default=10, help="Number of games per ablation match")
    parser.add_argument("--tc", type=str, default="depth 3", help="Time control (e.g., 'depth 3' or 'movetime 100')")
    parser.add_argument(
        "--heuristic",
        type=str,
        default="all",
        choices=["all", "no_tt", "no_ordering", "no_qsearch", "no_nmp", "no_lmr"],
    )
    parser.add_argument("--output-json", type=Path, default=None)
    parser.add_argument("--output-md", type=Path, default=None)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    binary_path = args.binary
    if not binary_path.is_file() and Path(str(binary_path).replace(".exe", "")).is_file():
        binary_path = Path(str(binary_path).replace(".exe", ""))

    openings = load_openings(args.openings)
    suite = ABLATION_SUITE if args.heuristic == "all" else [c for c in ABLATION_SUITE if c.key == args.heuristic]

    print("=" * 68)
    print("LOOKAHEAD SEARCH HEURISTIC ABLATION STUDY")
    print(f"Binary: {binary_path} | TC: go {args.tc} | Games/Match: {args.games}")
    print(f"Configurations to evaluate: {len(suite)}")
    print("=" * 68)

    results: list[AblationResult] = []
    for cfg in suite:
        print(f"\nEvaluating: {cfg.name} ({cfg.description})")
        res = run_ablation_match(binary_path, cfg, openings, args.games, args.tc, verbose=not args.quiet)
        results.append(res)
        print(
            f"Result for {cfg.name}: {res.baseline_wins}W - {res.draws}D - {res.variant_wins}L "
            f"({res.score_percent:.1f}%) | Baseline Elo: +{res.baseline_elo_advantage:.1f} ± {res.elo_margin_95:.1f}"
        )

    md_table = format_markdown_table(results)
    print("\n" + "=" * 68)
    print("SUMMARY ABLATION TABLE")
    print("=" * 68)
    print(md_table)
    print("=" * 68)

    if args.output_md:
        args.output_md.parent.mkdir(parents=True, exist_ok=True)
        with open(args.output_md, "w", encoding="utf-8") as f:
            f.write(md_table + "\n")
        print(f"Saved Markdown table to {args.output_md}")

    if args.output_json:
        args.output_json.parent.mkdir(parents=True, exist_ok=True)
        with open(args.output_json, "w", encoding="utf-8") as f:
            json.dump([asdict(r) for r in results], f, indent=2)
        print(f"Saved JSON results to {args.output_json}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
