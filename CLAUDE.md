# Lookahead - Project Guide & Rules for AI Agents

## Current Status
- **Current Phase**: Phase 8 (Emscripten WebAssembly & React Web Application) - Sub-PR 8.1: Emscripten WASM Build Configuration & C-API Bridge
- **Active Branch**: `phase-8-wasm-build-api`
- **Next Step**: Open PR #17, wait for user review & merge, then proceed to Sub-PR 8.2 (Web Worker WASM Controller & Hook)
- **Author**: Alex (`Alex-2006-alt`)
- **Repository**: `https://github.com/Alex-2006-alt/Lookahead.git`
- **Default Branch**: `main`

## Core Architectural Rules
1. **Layer Dependency**: Dependency direction is strictly one-way:
   `core <- movegen <- eval <- search <- interface`
   Any upward include is forbidden and caught by `tools/check_layers.py`.
2. **File Size & Focus**: One concept per file, named after that concept. No `utils` or `misc` files. No file may exceed 300 lines of code.
3. **File Header Comments**: Every source file starts with a 3 to 6 line comment block:
   - What this file does
   - Why it exists
   - How it fits in the architecture layers
4. **Feature Toggles**: All search and evaluation feature flags reside in `search/features.h` and an evaluation config, nowhere else.
5. **No Dynamic Allocations in Search**: No heap allocations (`new`, `malloc`, `std::vector::push_back` reallocation) in the search hot path.
6. **Data Separation**: Test positions live in `data/`, never hardcoded into test source code.
7. **Test Isolation & CI Stability Rules**:
   - Every test that inspects search results, nodes, depths, or PV strings MUST call `search::reset_features_to_default();` and `search::global_tt.clear();` in its fixture `SetUp()` or test body.
   - Root PV Extraction: Always call `pv.update(0, tt_move)` on root TT hits and fallback to `best_move.to_uci()` so PV strings are never empty.
   - Time Assertion Margins: The engine intentionally avoids starting new depths when $\ge 50\%$ of target movetime has elapsed. Verifier scripts must accept $[0.4 \times T, 5.0 \times T]$ bounds.

## Git Workflow Rules
1. **Never Commit to Main**: Only the initial bootstrap commit may go to `main`. All subsequent work happens in dedicated feature branches (`phase-N-name`).
2. **Phase Lifecycle**:
   - Create GitHub issue for the phase with a checklist.
   - Branch: `git switch main && git pull && git switch -c phase-N-name`.
   - Small, atomic commits with format: `type(scope): summary` (`feat`, `fix`, `test`, `docs`, `refactor`, `perf`, `chore`).
   - Run native tests, sanitizers, and `tools/check_layers.py`.
   - Open Pull Request using `.github/pull_request_template.md`.
   - **STOP** and provide the PR link to the user. Do not merge.
   - User reviews and merges via merge commit or rebase.
3. **History Integrity**: Never force push, rewrite history, or make fake/empty commits.

## Tooling
- **Engine**: C++20, CMake, Ninja, GoogleTest.
- **Python Oracle**: Python 3.12 with `python-chess` for differential verification and match runners.
- **Web App**: Vite + React + TypeScript + Tailwind (runs WASM engine in Web Worker).
