# Changelog

All notable changes to the Lookahead chess engine and web application will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
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
