# Changelog

All notable changes to the Lookahead chess engine and web application will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Independent search feature toggles framework in `search/features.h` for clean heuristic ablation testing.
- Fixed-size Transposition Table with full 64-bit Zobrist collision protection, generation aging, depth replacement, and ply-adjusted mate scores in `search/transposition_table.h` and `search/transposition_table.cpp`.
- Quiescence search with stand-pat cutoff, delta pruning, and MVV-LVA capture ordering in `search/quiescence.h` and `search/quiescence.cpp`.
- Iterative deepening negamax alpha-beta search framework in `search/search.h` and `search/search.cpp`.
- Diagnostic CLI command `lookahead search <depth> [fen]` reporting bestmove, score, nodes, and NPS.
- Unit test suite in `engine/tests/search/test_search.cpp` verifying TT caching, mate score adjustment, quiescence cutoffs, and Scholar's Mate / Back-rank mate detection.
- Comprehensive search documentation in `docs/04-search-alpha-beta.md`.
- Positional pawn structure evaluator covering doubled, isolated, passed (rank-scaled), and connected pawns in `eval/pawn_structure.h` and `eval/pawn_structure.cpp`.
- King safety evaluation modeling castled pawn shelter, open/semi-open file exposure, and progressive virtual enemy attack units in `eval/king_safety.h` and `eval/king_safety.cpp`.
- Piece mobility evaluator measuring safe reachable destinations outside opposing pawn attack control for knights, bishops, rooks, and queens in `eval/mobility.h` and `eval/mobility.cpp`.
- Integrated positional terms into root `evaluate()`, `evaluate_detailed()`, and `lookahead eval` CLI reporting.
- Expanded evaluation test suite to 61 GoogleTests verifying doubled pawns, isolated pawns, passed pawns, king shelter, piece mobility, and term-by-term symmetry in `engine/tests/eval/test_evaluate.cpp`.
- Comprehensive documentation of positional evaluation terms and mathematical formulations in `docs/03-evaluation.md`.
- Hand-crafted evaluation framework with dual-phase `Score` container and smooth 24-point non-pawn material phase tapering in `eval/evaluate.h` and `eval/evaluate.cpp`.
- PeSTO material values and 64-square piece-square tables (PSQT) for all piece types with vertical rank mirroring in `eval/material_psqt.h` and `eval/material_psqt.cpp`.
- `EvalBreakdown` structure capturing component terms for web UI visualization.
- Diagnostic CLI command `lookahead eval [fen]` providing centipawn evaluation, phase, and material/PSQT breakdowns.
- Unit test suite verifying starting position evaluation, phase calculations, material dominance, central outposts, and strict mathematical symmetry across diverse positions in `engine/tests/eval/test_evaluate.cpp`.
- Documented PeSTO evaluation methodology and mathematical symmetry proof in `docs/03-evaluation.md`, `docs/REFERENCES.md`, and `docs/LICENSES.md`.
- Recursive perft tree enumerator, `perft_divide()` move analyzer, and `run_perft()` throughput benchmark harness in `movegen/perft.h` and `movegen/perft.cpp`.
- Native executable entry point in `engine/src/main.cpp` supporting `uci`, `perft`, `divide`, and batch move generation (`moves -`).
- Standard perft dataset `data/perft_positions.epd` covering Initial Position, Kiwipete, and CPW positions 3-6.
- Comprehensive perft test suite with 8 tests across 6 positions and 13M+ nodes in `engine/tests/movegen/test_perft.cpp`.
- Automated perft benchmark and ground-truth verifier script in `tools/check_perft.py`.
- Automated differential fuzzer in `tools/compare_movegen.py` verifying 10,000 random positions against `python-chess`.
- Benchmark empirical measurements and differential fuzzer validation in `docs/RESULTS.md`.
- Deepened architectural documentation in `docs/02-move-generation-and-perft.md`.
- Strictly legal move generator with pin detection, king evasions, double check optimization, castling, promotions, and en passant discovered-check edge case handling in `movegen/movegen.h` and `movegen/movegen.cpp`.
- Stack-allocated `MoveList` container supporting up to 256 moves with zero dynamic allocations on the search stack.
- Game-end rule evaluators: `is_checkmate`, `is_stalemate`, `is_fifty_move_rule`, and `is_insufficient_material`.
- Unit test suite verifying initial move counts, absolute pins, double check restrictions, castling legality, en passant discovered check prune, checkmate, and stalemate in `engine/tests/movegen/test_movegen.cpp`.
- Precomputed leaping attack tables (Pawns, Knights, Kings) and Magic Bitboards for sliding pieces (Bishops, Rooks, Queens) in `movegen/attacks.h` and `movegen/attacks.cpp`.
- Collinear ray `between` and `line` bitboard functions for fast check ray and pin detection.
- Unit test suite validating leaping/sliding attacks, blocker clipping, and collinear rays in `engine/tests/movegen/test_attacks.cpp`.
- Complete chess `Position` representation, FEN parser/serializer with strict validation, and O(1) `make_move`/`unmake_move` with incremental Zobrist hashing in `core/position.h` and `core/position.cpp`.
- Unit test suite validating standard & tricky FEN roundtrips, invalid input rejection, castling, en passant, promotions, and make/unmake move chain fuzz testing in `engine/tests/core/test_position.cpp`.
- Deterministic 64-bit Zobrist hashing with SplitMix64 PRNG in `core/zobrist.h` and `core/zobrist.cpp`.
- Unit test suite validating Zobrist key uniqueness, determinism, and XOR involution in `engine/tests/core/test_zobrist.cpp`.
- Strongly typed chess primitives (`Color`, `Piece`, `PieceType`, `Square`, `Rank`, `File`, `CastlingRights`) in `core/types.h`.
- 64-bit bitboard manipulation and hardware bit primitives in `core/bitboard.h`.
- Cache-efficient 16-bit compact move encoding with UCI serializer/parser in `core/move.h`.
- Version metadata header `core/version.h` defining v0.1.0 and author identity.
- Unit test suite for core types, bitboard operations, and move encoding.

## [0.1.0] - 2026-11-07
### Added
- Phase 0: Inspected host environment, confirmed standalone repository origin, and verified clean setup.
- Phase 1: Initialized repository structure, strict architectural guidelines, issue/PR templates, and GitHub Actions CI skeleton.
- Created `tools/check_layers.py` to enforce one-way layer dependencies, 300-line limit per file, and mandatory header comments.
- Created modular documentation stubs in `docs/` and directory structure for `engine/`, `data/`, `tools/`, and `web/`.
