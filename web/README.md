# Lookahead Web Application (`web/`)

Interactive web interface for Lookahead built with React 19, TypeScript, Vite, and Tailwind CSS.

---

## Architecture

* **Web Worker Engine**: Lookahead's WebAssembly binary runs entirely inside a dedicated Web Worker (`src/engine/worker.ts`), preventing UI frame drops during intensive searches.
* **Three Dedicated Views**:
  1. **Play**: Play against calibrated bot difficulty levels mapped to measured Elo ratings.
  2. **Analysis**: Full board analysis with evaluation gauge, principal variation lines, and FEN/PGN import/export.
  3. **Inside Lookahead**: Interactive search tree visualizer displaying alpha-beta windows, pruning cutoffs, and transposition table hits.

---

## Design System

* Canvas: `#0B0D12` (dark slate)
* Accent: `#7FA8D1` (muted blue)
* Positive / Changed State: `#8FC4A8` (sage)
* Glass Panels: `rgba(11, 13, 18, 0.80)` with hairline borders `rgba(255, 255, 255, 0.09)`
* Typography: *Instrument Serif* (headings), *Inter* (body), *JetBrains Mono* (numbers and FEN strings)
