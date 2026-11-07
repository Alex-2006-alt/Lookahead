# 06 - UCI Protocol & WebAssembly Bridge

## UCI (Universal Chess Interface) Protocol

Lookahead implements the standard UCI protocol to integrate seamlessly with external GUIs (Cutechess, Arena, ChessBase, python-chess).

### Supported Commands
* `uci`: Identifies the engine (`id name Lookahead <version>`, `id author Alex`), outputs options, and sends `uciok`.
* `isready`: Synchronizes communication; answers with `readyok`.
* `ucinewgame`: Clears the transposition table and resets search state for a new game.
* `position [startpos | fen <fen>] moves <m1> <m2> ...`: Sets the board position and applies a move sequence.
* `go [depth <d>] [movetime <ms>] [wtime <ms> btime <ms> winc <ms> binc <ms>] [nodes <n>] [infinite]`: Starts the search with given constraints.
* `stop`: Signals the search thread to terminate gracefully and output `bestmove`.
* `setoption name Hash value <n>`: Dynamically resizes the transposition table.

---

## WebAssembly Architecture

To run inside web browsers at near-native speed:
* Lookahead compiles to `lookahead.wasm` via Emscripten.
* An asynchronous Web Worker hosts the WASM engine, decoupling calculation from the React UI thread to avoid stutter.
* Communication between the web app and engine uses JSON and UCI strings via `postMessage`.
