# Lookahead

> **A chess engine you can see inside.**

Lookahead is an original, pedagogical chess engine written from scratch in C++20 with an alpha-beta search tree, running natively via UCI and in the browser via WebAssembly, accompanied by an interactive search visualizer.

[![CI](https://github.com/Alex-2006-alt/Lookahead/actions/workflows/ci.yml/badge.svg)](https://github.com/Alex-2006-alt/Lookahead/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

---

## Architecture Overview

Lookahead enforces a strict one-way layered architecture:

```
+-------------------------------------------------------------+
|                        web / wasm                           |
|       (React + Vite + Tailwind + Web Worker UI)             |
+-------------------------------------------------------------+
                              |
+-------------------------------------------------------------+
|                      interface                              |
|           (UCI Protocol & WebAssembly API)                  |
+-------------------------------------------------------------+
                              |
+-------------------------------------------------------------+
|                        search                               |
| (Alpha-Beta, Iterative Deepening, TT, Pruning, Ordering)    |
+-------------------------------------------------------------+
                              |
+-------------------------------------------------------------+
|                         eval                                |
|    (Tapered Material, PSQT, King Safety, Pawn Structure)    |
+-------------------------------------------------------------+
                              |
+-------------------------------------------------------------+
|                        movegen                              |
|       (Magic Bitboards, Legal Movegen, Perft)               |
+-------------------------------------------------------------+
                              |
+-------------------------------------------------------------+
|                         core                                |
|   (Bitboard primitives, 16-bit Move, FEN, Zobrist Hash)     |
+-------------------------------------------------------------+
```

Each lower layer has zero knowledge of the layers above it. Layer boundaries are automatically validated on every commit by `tools/check_layers.py`.

---

## Design Principles

1. **Written from Scratch**: Every component—from bitboards and attack tables to alpha-beta pruning and the WebAssembly frontend—is implemented from first principles. No third-party engine code is copied.
2. **Zero Dynamic Allocation in Search**: The search hot path uses stack memory, preallocated transposition tables, and fixed-depth move buffers.
3. **Web-First & Single-Threaded**: Built with standard C++20 and compiled via Emscripten to run smoothly in standard browsers without requiring `SharedArrayBuffer` headers.
4. **Transparent & Visualizable**: Includes an instrumented tree tracer mode that powers "Inside Lookahead"—an interactive browser inspection tool showing alpha/beta windows, pruning cutoffs, and transposition table hits.

---

## Directory Roadmap

| Directory | Purpose | Start Reading Here |
| :--- | :--- | :--- |
| [`engine/`](engine/README.md) | C++20 Engine core, move generation, eval, search, and UCI/WASM interfaces | [engine/README.md](engine/README.md) |
| [`docs/`](docs/README.md) | In-depth engineering design docs, theory, and interview notes | [docs/00-overview.md](docs/00-overview.md) |
| [`tools/`](tools/README.md) | Python test harnesses, perft oracles, and layer checkers | [tools/README.md](tools/README.md) |
| [`data/`](data/README.md) | Standardized test datasets (perft suites, mate puzzles, openings) | [data/README.md](data/README.md) |
| [`web/`](web/README.md) | React + TypeScript web application with WebAssembly worker | [web/README.md](web/README.md) |

---

## Quick Start (Native Engine)

### Prerequisites
- Modern C++20 compiler (GCC 13+, Clang 16+, or MSVC 2022)
- CMake 3.25+
- Ninja build system
- Python 3.12+ (with `python-chess` for testing)

### Build Instructions
```bash
# Configure release build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release engine

# Build native binary
cmake --build build --config Release

# Run test suite
ctest --test-dir build --output-on-failure
```

---

## Verification & Integrity

* **Perft Oracle**: All perft suites in `data/perft_positions.epd` are verified against `python-chess`.
* **Sanitizers**: Continuous integration tests clean execution under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
* **Layer Checking**: `python tools/check_layers.py` ensures 0 upward include violations across all files.

---

## License

This project is released under the [MIT License](LICENSE).
Author: **Alex** (`Alex-2006-alt`).
