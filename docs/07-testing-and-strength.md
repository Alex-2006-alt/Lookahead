# 07 - Testing Methodology & Strength Measurement

## Rigorous Verification Hierarchy

1. **Perft Oracle Tests**: Every tricky position in `data/perft_positions.epd` is validated against `python-chess`.
2. **Differential Fuzzing (`tools/compare_movegen.py`)**: 10,000+ random board positions generated via random walks are compared move-for-move against `python-chess`'s move generator.
3. **Fuzzing Make/Unmake**: After executing random moves, unmaking them must return the board state, bitboards, and Zobrist hash to the exact initial state.
4. **Sanitizer Enforcement**: All tests pass under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
5. **Layer Verification**: `tools/check_layers.py` ensures 0 architecture layer violations across all translation units.

---

## Tournament Match Runner Architecture

To measure playing strength and quantify heuristic contributions, Lookahead includes an automated tournament match runner in `tools/match_runner.py`.

```
                  ┌────────────────────────┐
                  │   data/openings.epd    │
                  │   (24 Balanced ECO)    │
                  └───────────┬────────────┘
                              │
                    Color Alternation Pair
                              │
               ┌──────────────┴──────────────┐
               ▼                             ▼
       ┌──────────────┐               ┌──────────────┐
       │   Engine 1   │ <=== UCI ===> │   Engine 2   │
       │ (Lookahead A)│     Pipes     │ (Lookahead B)│
       └──────────────┘               └──────────────┘
               │                             │
               └──────────────┬──────────────┘
                              │
                   Adjudication & Tracking
                              │
               ┌──────────────┴──────────────┐
               ▼                             ▼
       ┌──────────────┐               ┌──────────────┐
       │ Elo & 95% CI │               │  PGN Archive │
       └──────────────┘               └──────────────┘
```

### Color Alternation Protocol
In chess, White holds a persistent first-move advantage (~53-55% score expectancy). To ensure unbiased strength measurement, `tools/match_runner.py` pairs every opening twice:
- Game $2k$: Engine 1 plays White, Engine 2 plays Black.
- Game $2k+1$: Engine 2 plays White, Engine 1 plays Black.

### Balanced Opening Book (`data/openings.epd`)
Contains 24 curated positions spanning ECO classifications A through E:
- **ECO C (Open)**: Italian Game, Ruy Lopez, Scotch Game
- **ECO B (Semi-Open)**: Sicilian (Open & Closed), French (Classical & Advance), Caro-Kann (Main Line & Advance), Scandinavian, Alekhine, Pirc
- **ECO D (Closed)**: Queen's Gambit Declined, Queen's Gambit Accepted, Slav Defense, King's Indian Defense, Nimzo-Indian, Queen's Indian, Grünfeld
- **ECO A (Flank)**: Dutch Defense, English (Symmetrical & King's English), Réti Opening, King's Fianchetto

---

## Statistical Elo & Confidence Interval Formulation

Match outcomes are scored standardly: $W = 1.0$, $D = 0.5$, $L = 0.0$.
Given $N$ games with wins $W$, draws $D$, and losses $L$, the sample mean score is:

$$\mu = \frac{W + 0.5D}{N}$$

The relative Elo rating difference $\Delta \text{Elo}$ between Engine 1 and Engine 2 is derived from the logistic distribution:

$$\Delta \text{Elo} = -400 \log_{10}\left(\frac{1}{\mu} - 1\right)$$

The sample variance $s^2$ across individual game scores $x_i \in \{0.0, 0.5, 1.0\}$ is:

$$s^2 = \frac{W(1 - \mu)^2 + D(0.5 - \mu)^2 + L(0 - \mu)^2}{N - 1}$$

Using the delta method for nonlinear parameter transformation $\Delta \text{Elo}(\mu)$, the standard error of the Elo estimate is:

$$\text{SE}(\Delta \text{Elo}) = \frac{s}{\sqrt{N}} \times \left|\frac{d(\Delta \text{Elo})}{d\mu}\right| = \frac{s}{\sqrt{N}} \times \frac{400 \ln 10}{\mu(1 - \mu)}$$

The 95% confidence interval half-width is:

$$\text{Margin}_{95\%} = 1.96 \times \text{SE}(\Delta \text{Elo})$$

Reported as: $\Delta \text{Elo} \pm \text{Margin}_{95\%}$.

---

## Automated Adjudication Rules

1. **Checkmate**: Verified by `python-chess` legal move exhaustion under check.
2. **Stalemate**: Move exhaustion without check.
3. **Threefold Repetition**: Automatic draw adjudication on 3-fold position recurrence.
4. **Fifty-Move Rule**: Automatic draw adjudication when halfmove clock reaches 100.
5. **Insufficient Material**: Automatic draw adjudication on K vs K, K+N vs K, K+B vs K, K+B vs K+B (same color).
6. **Move Limit Cutoff**: Default 150 moves per game to avoid infinite endgames.

---

## Heuristic Ablation Study Framework (`tools/run_ablation.py`)

Lookahead exposes every search heuristic as an independent boolean UCI option (`type check default true`):
- `TranspositionTable`: TT transposition probe cutoffs and bestmove extraction
- `MoveOrdering`: Move ranking by TT move, MVV-LVA, killers, and history heuristic
- `QuiescenceSearch`: Stand-pat tactical capture extensions at leaf nodes
- `NullMovePruning`: Adaptive null-move forward pruning ($R=2$ or $R=3$)
- `LateMoveReductions`: Logarithmic reductions on late quiet moves
- `ReverseFutility`: Static evaluation margin pruning in pre-frontier nodes
- `AspirationWindows`: Iterative search narrowing around previous depth scores

The automated ablation runner (`tools/run_ablation.py`) executes head-to-head matches between full Baseline Lookahead and single-heuristic disabled variants across the 24 balanced openings in `data/openings.epd`, isolating the empirical contribution and Elo value of each component.

---

## Test Isolation & CI Stability Guidelines

To ensure tests and CI checks never experience regressions or intermittent failures:

1. **Search & Cache Isolation**: Any test invoking search or inspecting PV strings, nodes, or depth MUST invoke `search::reset_features_to_default()` and `search::global_tt.clear()` in its fixture setup (`SetUp()`) or body. Caching bleed between tests can cause root TT cutoffs and alter expected search outputs.
2. **Root PV Extraction on TT Cutoffs**: When a search probe hits the Transposition Table at root (`ply == 0`), `pv.update(0, tt_move)` must be explicitly called to ensure PV chains are populated even on immediate cutoffs.
3. **Adaptive Time Bounds in Verifiers**: The tournament `TimeManager` intentionally aborts initiating deeper plies when $\ge 50\%$ of target movetime has expired to protect clock safety. External verification assertions (e.g. `verify_uci.py`) must accept elapsed execution time within $[0.4 \times T_{\text{target}}, 5.0 \times T_{\text{target}}]$ to handle both early half-time depth terminations and runner process scheduling delays.

---

## Standardized Search Benchmark Suite (`bench`)

To measure raw search throughput (nodes per second) and ensure performance stability across code changes, Lookahead includes a standardized 12-position benchmark suite in `engine/src/search/benchmark.cpp`:

1. **Startpos**: Standard opening board
2. **Kiwipete**: Highly tactical middlegame with heavy captures and checks
3. **Position 3**: Pawns and pins complex
4. **Position 4**: Pawn promotions and castling rights
5. **Position 5**: Fine tactical endgame
6. **Position 6**: Symmetrical middlegame complex
7. **King & Pawn Endgame**: Deep zugzwang testing
8. **Rook Endgame**: Fast horizon resolution
9. **Queen & Pawn Endgame**: High mobility testing
10. **Knight Endgame**: Precise minor piece stepping
11. **Bishop Endgame**: Color complex testing
12. **King Safety & Castling**: Sharp opposite-side castling attack

### Invocation Commands
- **CLI**: `lookahead bench [depth] [hash_mb]` (e.g. `lookahead bench 5 16`)
- **UCI Protocol**: `bench [depth]` (invoked directly over stdin/stdout)
- **Profiler Script**: `python tools/run_bench.py --binary build/lookahead.exe --depth 5`
- **Output Metrics**: per-position nodes, elapsed ms, NPS, and bestmove, followed by total aggregated nodes, total duration, and overall search NPS.
