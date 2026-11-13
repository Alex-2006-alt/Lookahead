# 08 - WebAssembly & Web Application Architecture

## Overview

Lookahead delivers full cross-platform engine equivalence by compiling the C++20 engine codebase to WebAssembly (WASM) using Emscripten. The engine executes inside a dedicated browser Web Worker, communicating with a modern React + TypeScript frontend via asynchronous message passing.

```
┌─────────────────────────────────────────────────────────────┐
│                      Browser Window                         │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │             React Frontend (web/)                     │  │
│  │   Interactive Board • Eval Bar • Notation • PGN       │  │
│  └───────────────────────────┬───────────────────────────┘  │
│                              │ postMessage({ cmd })         │
│                              ▼ onmessage({ type, data })    │
│  ┌───────────────────────────────────────────────────────┐  │
│  │             Web Worker (engineWorker.ts)              │  │
│  │                                                       │  │
│  │    ┌─────────────────────────────────────────────┐    │  │
│  │    │      LookaheadModule (WebAssembly Instance) │    │  │
│  │    │                                             │    │  │
│  │    │  lookahead_send_command(cmd)                │    │  │
│  │    │  callback(line) -> postMessage(line)        │    │  │
│  │    │                                             │    │  │
│  │    │  [Layer 4: interface/wasm_api.cpp]          │    │  │
│  │    │  [Layer 3: search/ (Alpha-Beta / Negamax)]  │    │  │
│  │    │  [Layer 2: eval/ (Tapered Evaluation)]      │    │  │
│  │    │  [Layer 1: movegen/ (Magic Bitboards)]      │    │  │
│  │    │  [Layer 0: core/ (Bitboards & Zobrist)]     │    │  │
│  │    └─────────────────────────────────────────────┘    │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 1. WebAssembly Compilation Pipeline

### Emscripten Build Configuration
Lookahead uses `emcmake` to configure CMake with production optimizations:
- `-O3`: Full whole-program link-time optimization (LTO) and dead code elimination.
- `-sWASM=1`: Emits binary `.wasm` with a modular JavaScript loader (`lookahead.js`).
- `-sMODULARIZE=1 -sEXPORT_NAME=LookaheadModule`: Encapsulates module factory in a self-contained promise-based closure.
- `-sALLOW_MEMORY_GROWTH=1`: Enables dynamic linear memory expansion as the transposition table or search tree requires.
- `-sINITIAL_MEMORY=67108864`: Allocates an initial 64 MB heap.
- `-sENVIRONMENT=web,worker,node`: Compatible across browser windows, Web Workers, and Node.js automated test environments.

---

## 2. WebAssembly C-API Bridge (`engine/src/interface/wasm_api.h`)

To eliminate complex embind/binding wrappers, Lookahead exposes lightweight, C-linkage entry points with `EMSCRIPTEN_KEEPALIVE`:

| Function | Signature | Purpose |
| :--- | :--- | :--- |
| `lookahead_init` | `void(int hash_mb)` | Initializes attack tables, Zobrist seeds, and allocates the transposition table. |
| `lookahead_send_command` | `void(const char* cmd)` | Forwards UCI protocol commands directly into the engine's command interpreter. |
| `lookahead_set_output_callback` | `void(void (*cb)(const char*))` | Registers a C/JS callback invoked synchronously whenever the engine outputs a line. |
| `lookahead_read_output` | `int(char* buf, int max_len)` | Alternative polling interface reading queued output lines. |
| `lookahead_shutdown` | `void()` | Stops active search threads, joins worker handles, and cleans up memory. |

---

## 3. Dedicated Web Worker Concurrency

Heavy search tree exploration (evaluating hundreds of thousands of nodes per second) is computationally intensive. Running search on the browser's main JavaScript thread would block the DOM event loop, freezing animations and causing severe user interface stuttering.

To guarantee perpetual 60+ FPS UI responsiveness:
1. The engine WASM binary runs exclusively inside a dedicated **Web Worker**.
2. The UI thread and worker thread communicate solely via non-blocking asynchronous `postMessage` channels.
3. Telemetry lines (`info depth ... score cp ... nps ... pv ...`) stream smoothly without interrupting piece drag-and-drop interactions.

---

## 4. Frontend Architecture (`web/` Workspace)

The frontend is housed in a dedicated, isolated `web/` directory:
- **Build Tool**: Vite (lightning-fast HMR and ESM bundling)
- **Framework**: React with TypeScript
- **Styling**: Tailwind CSS with custom chess themes
- **State Management**: Custom `useEngine` React hook abstracting worker lifecycles, UCI commands, move history, and search metrics.

---

## 5. Web Worker Message Protocol & React State Flow

```
 UI Thread (React / useEngine)              Web Worker (engineWorker.ts)
 ─────────────────────────────              ────────────────────────────
        │                                                │
        │─── postMessage({ type: 'init' }) ─────────────>│ Instantiates WASM Module
        │<── postMessage({ type: 'status': 'ready' }) ───│ Initial UCI Handshake
        │                                                │
        │─── postMessage({ type: 'position', fen }) ────>│ lookahead_send_command(position)
        │─── postMessage({ type: 'go', depth: 6 }) ─────>│ lookahead_send_command(go depth 6)
        │                                                │
        │<── postMessage({ type: 'info', info }) ────────│ Streaming Telemetry (depth, NPS, pv)
        │<── postMessage({ type: 'bestmove', bm }) ──────│ Emits Best Move & Returns to Ready
        │                                                │
```

- **Types (`web/src/engine/types.ts`)**: Strongly typed message envelopes (`WorkerInMessage`, `WorkerOutMessage`, `SearchInfo`, `EngineStatus`).
- **Telemetry Parser (`web/src/engine/uciParser.ts`)**: Pure parsing utility transforming UCI `info` tokens (depth, cp/mate scores, nodes, nps, pv) into immutable JavaScript records with zero dependencies.
- **Worker Controller (`web/src/engine/engineWorker.ts`)**: Handles dynamic WASM factory loading, manages C callback memory boundaries, and guarantees graceful degradation in development.
- **React Hook (`web/src/engine/useEngine.ts`)**: Reusable React hook exposing state (`status`, `isSearching`, `searchInfo`, `lastBestMove`) and action dispatchers (`startSearch`, `stopSearch`, `newGame`, `sendCommand`).
