# 06 - UCI Protocol & WebAssembly Bridge

## UCI (Universal Chess Interface) Protocol

The Universal Chess Interface (UCI) is an open, line-oriented text communication protocol that connects chess engines to graphical user interfaces (such as Cutechess, Arena, ChessBase, BanksiaGUI, and python-chess).

Lookahead implements the standard UCI protocol in Layer 4 (`interface/`), coordinating search, move generation, and core board state without violating one-way layer dependencies.

---

## Architecture & Design

### Stream Injection for Deterministic Testing
Traditional UCI engines read directly from `std::cin` and write to `std::cout`, making automated unit testing dependent on external process spawning or OS-level file descriptor redirection.

In Lookahead, `UciHandler` decouples protocol interpretation from physical standard streams via stream dependency injection:

```cpp
class UciHandler {
public:
    explicit UciHandler(std::istream& in = std::cin, std::ostream& out = std::cout);
    void loop();
    void handle_command(std::string_view line);
    [[nodiscard]] const core::Position& position() const noexcept;
    [[nodiscard]] bool is_running() const noexcept;
};
```

This architecture allows GoogleTest suites to pass simulated GUI command sequences via `std::istringstream` and inspect engine responses in `std::ostringstream` synchronously with sub-millisecond execution times.

### Command Dispatch Loop
The UCI parser normalizes incoming command lines by stripping leading and trailing whitespace, extracting the primary command token, and dispatching to specialized handlers:
- `handle_position`: Parses `startpos` or arbitrary 6-field FEN strings and executes the subsequent `moves` list sequentially using `movegen::generate_legal_moves()`.
- `handle_go`: Parses search constraints (`depth`, `movetime`, `wtime`, `btime`, `winc`, `binc`, `nodes`, `movestogo`, `infinite`), binds a real-time `SearchCallback` to stream UCI `info` telemetry, runs iterative deepening, and outputs `bestmove`.
- `handle_setoption`: Handles dynamic engine configuration such as memory allocation for the Transposition Table.

---

## Supported Commands & Options

### Commands

| Command | Arguments | Description |
|---|---|---|
| `uci` | none | Engine handshake. Outputs `id name`, `id author`, available options, and terminates with `uciok`. |
| `isready` | none | Synchronization command. Flushes pending state and responds with `readyok`. |
| `ucinewgame` | none | Prepares engine for a new game; resets board to starting position and clears the Transposition Table. |
| `position` | `[startpos \| fen <fen>] [moves <m1> ...]` | Sets internal board state and applies subsequent UCI-formatted move list. |
| `go` | `[depth] [movetime] [wtime] [btime] [winc] [binc] [nodes] [movestogo] [infinite]` | Initiates iterative deepening search subject to time or node limits. |
| `setoption` | `name <name> [value <val>]` | Adjusts runtime engine configuration options. |
| `quit` | none | Sets loop termination flag and exits the UCI session cleanly. |

### Configurable Options

* **`Hash`** (`type spin`, default `16`, min `1`, max `1024`): Resizes the global Transposition Table dynamically to the specified megabyte capacity.
* **`Clear Hash`** (`type button`): Clears all entries from the Transposition Table without changing its size.

---

## CLI Integration & Backward Compatibility

Lookahead's native CLI (`engine/src/main.cpp`) seamlessly balances standard UCI GUI communication with developer diagnostics:
- Running `lookahead` with no arguments or `lookahead uci` activates interactive UCI loop mode.
- Running diagnostic subcommands (`perft`, `divide`, `moves`, `eval`, `search`) allows instant developer inspection and testing from shell environments or profiling tools.

---

## Unit Testing & Verification

The UCI protocol compliance is verified via GoogleTest in `engine/tests/interface/test_uci.cpp`:
1. `UciTest.UciHandshake`: Verifies engine identification, author attribution, option declarations, and `uciok`.
2. `UciTest.IsReadyResponse`: Validates immediate `readyok` handshake response.
3. `UciTest.UciNewGameResetsBoard`: Verifies board and TT reset after multi-ply move sequences.
4. `UciTest.PositionStartposMoves`: Validates sequential move parsing on starting position.
5. `UciTest.PositionFenAndMoves`: Validates arbitrary FEN setup (e.g. Scholar's Mate) and subsequent moves.
6. `UciTest.SetOptionHashAndClear`: Verifies dynamic hash resizing and clearing.
7. `UciTest.GoSearchProducesBestMove`: Validates `go` search limit ingestion, streaming `info` telemetry lines, and terminal `bestmove` emission.
