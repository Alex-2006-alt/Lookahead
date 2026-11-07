# Layer 3: Search (`engine/src/search`)

Directs game tree exploration and pruning. Can include headers from `core/`, `movegen/`, and `eval/`.

## Files
- `features.h`: Master feature flags for all search and evaluation heuristics.
- `search.h` / `search.cpp`: Alpha-beta negamax search with iterative deepening.
- `quiescence.cpp`: Tactical capture resolution.
- `move_ordering.h` / `move_ordering.cpp`: MVV-LVA, killer moves, and history heuristic.
- `transposition_table.h` / `transposition_table.cpp`: Fixed-size hash table.
- `pruning.cpp`: Null-move pruning and late move reductions.
- `time_manager.h` / `time_manager.cpp`: Dynamic clock allocation.
- `trace.h` / `trace.cpp`: Search tree instrumentation for browser visualization.
