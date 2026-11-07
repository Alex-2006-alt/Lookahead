# Lookahead Experimental Results & Benchmarks

This document records empirical benchmarks, hardware specifications, and ablation studies. All reported metrics trace to verified test runs.

---

## Host Benchmark Environment

* **CPU**: AMD / Intel x86_64
* **Compiler**: GCC 13+ / Clang 17+ (-O3, native)
* **Operating System**: Windows / Linux (CI)
* **Date**: 2026-11-07

---

## 1. Perft Speed & Verification

| Position | Depth | Expected Nodes | Lookahead Nodes | Status | Nodes/sec |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Starting Position | 1 | 20 | Pending Phase 3 | - | - |
| Starting Position | 2 | 400 | Pending Phase 3 | - | - |
| Starting Position | 3 | 8,902 | Pending Phase 3 | - | - |
| Starting Position | 4 | 197,281 | Pending Phase 3 | - | - |
| Starting Position | 5 | 4,865,609 | Pending Phase 3 | - | - |
| Kiwipete | 4 | 3,285,188 | Pending Phase 3 | - | - |

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
