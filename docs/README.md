# Lookahead Documentation

Welcome to the engineering documentation for **Lookahead**. This documentation is written for software engineers, interviewers, and students who want to understand the design, mathematics, and systems engineering behind a modern chess engine.

---

## Reading Order

We recommend reading these documents in architectural order:

1. [00-overview.md](00-overview.md): System architecture, layer isolation, and design decisions.
2. [01-board-and-bitboards.md](01-board-and-bitboards.md): 64-bit board representation, bitboard math, square mapping, and Zobrist hashing.
3. [02-move-generation-and-perft.md](02-move-generation-and-perft.md): Sliding attack generation, magic bitboards, legal move generation, and perft testing.
4. [03-evaluation.md](03-evaluation.md): Tapered eval, piece-square tables, pawn structure analysis, mobility, and king safety.
5. [04-search-alpha-beta.md](04-search-alpha-beta.md): Minimax, Negamax, alpha-beta pruning, and iterative deepening.
6. [05-search-speedups.md](05-search-speedups.md): Transposition tables, move ordering (MVV-LVA, killer moves, history), quiescence search, null-move pruning, and late move reductions.
7. [06-uci-protocol.md](06-uci-protocol.md): UCI interface implementation, command parsing, and WebAssembly bridge.
8. [07-testing-and-strength.md](07-testing-and-strength.md): Testing methodology, perft differential fuzzing with python-chess, and Elo benchmarking.

---

## Supplementary Documents

* [RESULTS.md](RESULTS.md): Benchmark results, hardware details, perft speeds, and ablation experiments.
* [INTERVIEW_NOTES.md](INTERVIEW_NOTES.md): 15 comprehensive technical interview questions and deep-dive answers based on this codebase.
* [REFERENCES.md](REFERENCES.md): Conceptual references, academic papers, and chess programming wiki citations.
* [LICENSES.md](LICENSES.md): Third-party asset licenses and attributions.
