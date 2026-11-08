# Lookahead Experimental Results & Benchmarks

This document records empirical benchmarks, hardware specifications, and ablation studies. All reported metrics trace to verified test runs.

---

## Host Benchmark Environment

* **CPU**: x86_64
* **Compiler**: GCC 16.2.0 (MinGW-w64 w64devkit)
* **Operating System**: Windows 11
* **Date**: 2026-11-08

---

## 1. Perft Speed & Verification

Measured using `tools/check_perft.py` against `data/perft_positions.epd`:

| Position | Depth | Expected Nodes | Lookahead Nodes | Status | Time (ms) | Nodes/sec |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| Starting Position | 1 | 20 | 20 | **PASS** | 0.01 | 2,597,402 |
| Starting Position | 2 | 400 | 400 | **PASS** | 0.06 | 6,389,776 |
| Starting Position | 3 | 8,902 | 8,902 | **PASS** | 0.70 | 12,632,325 |
| Starting Position | 4 | 197,281 | 197,281 | **PASS** | 18.85 | 10,463,837 |
| Starting Position | 5 | 4,865,609 | 4,865,609 | **PASS** | 358.99 | 13,553,806 |
| Kiwipete | 1 | 48 | 48 | **PASS** | 0.01 | 4,848,484 |
| Kiwipete | 2 | 2,039 | 2,039 | **PASS** | 0.17 | 12,100,890 |
| Kiwipete | 3 | 97,862 | 97,862 | **PASS** | 5.63 | 17,381,003 |
| Kiwipete | 4 | 4,085,603 | 4,085,603 | **PASS** | 248.71 | 16,427,090 |
| Position 3 (Pawns & Pins) | 5 | 674,624 | 674,624 | **PASS** | 71.61 | 9,420,346 |
| Position 4 (Promotions & Castling) | 4 | 422,333 | 422,333 | **PASS** | 24.99 | 16,903,191 |
| Position 5 (Discovered Checks & Pins) | 4 | 2,103,487 | 2,103,487 | **PASS** | 140.50 | 14,971,970 |
| Position 6 (King Evasions) | 5 | 403,657 | 403,657 | **PASS** | 55.16 | 7,317,969 |
| **Full Suite Total** | - | **13,017,879** | **13,017,879** | **PASS** | **939.30** | **13,859,154** |

### Differential Fuzzer (vs python-chess Oracle)
* **Tool**: `tools/compare_movegen.py`
* **Positions Tested**: 10,000 random positions sampled across 92 games from varied seed positions
* **Discrepancies**: **0** (100.00% exact move set match)

---

## 2. Feature Ablation Study

*(Will be populated during Phase 7 using `tools/run_ablation.py`)*

| Configuration | Games Played | Win / Draw / Loss | Relative Elo ($\pm 95\%$) | NPS |
| :--- | :---: | :---: | :---: | :---: |
| Full Engine (Baseline) | - | - | Baseline (0) | - |
| No Transposition Table | - | - | Pending Phase 7 | - |
| No Move Ordering | - | - | Pending Phase 7 | - |
| No Quiescence Search | - | - | Pending Phase 7 | - |
| No Null-Move Pruning | - | - | Pending Phase 7 | - |
| No Late Move Reductions | - | - | Pending Phase 7 | - |

---

## 3. Equivalence Verification (Native vs WebAssembly)

*(Will be measured in Phase 8 over 200 fixed positions at depth 6)*
* Positions Tested: 200
* Best Move Match Rate: Pending Phase 8
* Node Count Match Rate: Pending Phase 8
