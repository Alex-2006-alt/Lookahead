# 06 - UCI Protocol & WebAssembly Bridge

## UCI (Universal Chess Interface) Protocol

The Universal Chess Interface (UCI) is an open, line-oriented text communication protocol that connects chess engines to graphical user interfaces (such as Cutechess, Arena, ChessBase, BanksiaGUI, and python-chess).

Lookahead implements the standard UCI protocol in Layer 4 (`interface/`), coordinating search, move generation, and core board state without violating one-way layer dependencies.

---

## Architecture & Design

### Asynchronous Search Worker Thread
In real tournament play, GUIs require engines to search asynchronously so that the communication channel remains responsive to control signals:
- `isready`: Probes engine health and synchronization while calculation is underway.
- `stop`: Instructs the engine to terminate search early and return its current best move.
- `quit`: Instructs the engine to abort calculation immediately and exit cleanly.

Lookahead fulfills these requirements with a dedicated worker thread decoupled from the command dispatch loop:

```cpp
class UciHandler {
public:
    explicit UciHandler(std::istream& in = std::cin, std::ostream& out = std::cout);
    ~UciHandler();

    void loop();
    void handle_command(std::string_view line);
    void stop();

    [[nodiscard]] const core::Position& position() const noexcept;
    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] bool is_searching() const noexcept;

private:
    std::atomic<bool> searching_{false};
    std::atomic<bool> stop_signal_{false};
    std::thread search_thread_;
    std::mutex out_mutex_;
};
```

When `go` is received, `UciHandler` takes a thread-isolated snapshot of the root `Position` and spawns `search_thread_`. The search engine checks `stop_signal_` periodically within its node countdown and iterative deepening loop. When `stop` or `quit` is signaled, `stop_signal_` flips to `true`, causing `search::search()` to exit gracefully, emit `bestmove`, and join cleanly with zero resource leaks.

### Stream Injection for Deterministic Testing
Traditional UCI engines read directly from `std::cin` and write to `std::cout`, making automated unit testing dependent on external process spawning or OS-level file descriptor redirection.

In Lookahead, `UciHandler` decouples protocol interpretation from physical standard streams via stream dependency injection (`std::istream& in`, `std::ostream& out`). Output streams are guarded by `std::mutex out_mutex_` to eliminate race conditions between asynchronous worker telemetry and synchronous command responses.

---

## Supported Commands & Options

### Commands

| Command | Arguments | Description |
|---|---|---|
| `uci` | none | Engine handshake. Outputs `id name`, `id author`, available options, and terminates with `uciok`. |
| `isready` | none | Synchronization command. Responds with `readyok`, even during active asynchronous searches. |
| `ucinewgame` | none | Prepares engine for a new game; stops any active search, resets board to startpos, and clears the TT. |
| `position` | `[startpos \| fen <fen>] [moves <m1> ...]` | Sets internal board state and applies subsequent UCI-formatted move list. |
| `go` | `[depth] [movetime] [wtime] [btime] [winc] [binc] [nodes] [movestogo] [infinite]` | Initiates asynchronous iterative deepening search in a background worker thread. |
| `stop` | none | Signals the background search worker to abort and immediately output `bestmove`. |
| `setoption` | `name <name> [value <val>]` | Adjusts runtime engine configuration options. |
| `quit` | none | Aborts any active search worker, terminates the command loop, and exits cleanly. |

### Configurable Options

* **`Hash`** (`type spin`, default `16`, min `1`, max `1024`): Resizes the global Transposition Table dynamically to the specified megabyte capacity.
* **`Clear Hash`** (`type button`): Clears all entries from the Transposition Table without changing its size.

---

## CLI Integration & Backward Compatibility

Lookahead's native CLI (`engine/src/main.cpp`) seamlessly balances standard UCI GUI communication with developer diagnostics:
- Running `lookahead` with no arguments or `lookahead uci` activates interactive UCI loop mode.
- Running diagnostic subcommands (`perft`, `divide`, `moves`, `eval`, `search`) allows instant developer inspection and testing from shell environments or profiling tools.

---

## Testing & Verification

### Unit Tests (`engine/tests/interface/test_uci.cpp`)
Ten GoogleTests verify the protocol layer:
1. `UciTest.UciHandshake`: Verifies engine identification, author attribution, option declarations, and `uciok`.
2. `UciTest.IsReadyResponse`: Validates immediate `readyok` handshake response.
3. `UciTest.UciNewGameResetsBoard`: Verifies board and TT reset after multi-ply move sequences.
4. `UciTest.PositionStartposMoves`: Validates sequential move parsing on starting position.
5. `UciTest.PositionFenAndMoves`: Validates arbitrary FEN setup (e.g. Scholar's Mate) and subsequent moves.
6. `UciTest.SetOptionHashAndClear`: Verifies dynamic hash resizing and clearing.
7. `UciTest.GoSearchProducesBestMove`: Validates `go` search limit ingestion, streaming `info` telemetry lines, and terminal `bestmove` emission.
8. `UciTest.StopInterruptsInfiniteSearch`: Validates that `stop` halts infinite search and emits `bestmove`.
9. `UciTest.IsReadyHandshakeDuringSearch`: Confirms `readyok` response while background search is actively computing.
10. `UciTest.QuitAbortsActiveSearchGracefully`: Ensures `quit` aborts worker thread without leaks or aborts.

### Automated End-to-End Verifier (`tools/verify_uci.py`)
An automated Cutechess-style harness connects to the compiled engine binary via standard I/O pipes and executes 7 integration scenarios:
- Handshake negotiation
- Synchronization probe
- Dynamic hash reconfiguration
- Fixed-depth search execution and telemetry format verification
- Asynchronous infinite search abort with latency measurement (< 500ms)
- Tactical position checkmate resolution (`h5f7#`)
- Time control adherence (`movetime 200`)
