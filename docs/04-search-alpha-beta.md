# 04 - Alpha-Beta Search, Transposition Table & Quiescence

Lookahead implements a high-performance Negamax $\alpha\beta$ tree search with iterative deepening, fixed-size Transposition Table caching, and tactical Quiescence search.

---

## 1. Architecture & Layer Boundaries

The search layer (`engine/src/search/`) sits at Layer 3:
$$\text{core (0)} \leftarrow \text{movegen (1)} \leftarrow \text{eval (2)} \leftarrow \text{search (3)}$$

* [`features.h`](file:///engine/src/search/features.h): Algorithmic feature toggles (`USE_TT`, `USE_QUIESCENCE`, etc.) for clean ablation testing.
* [`transposition_table.h`](file:///engine/src/search/transposition_table.h) / [`transposition_table.cpp`](file:///engine/src/search/transposition_table.cpp): 64-bit Zobrist keyed hash table with exact, lower-bound, and upper-bound flags.
* [`quiescence.h`](file:///engine/src/search/quiescence.h) / [`quiescence.cpp`](file:///engine/src/search/quiescence.cpp): Stand-pat capture search with MVV-LVA move ordering and delta pruning.
* [`move_ordering.h`](file:///engine/src/search/move_ordering.h) / [`move_ordering.cpp`](file:///engine/src/search/move_ordering.cpp): Move prioritization hierarchy (TT move, MVV-LVA, killers, history) and selection sorting.
* [`pruning.h`](file:///engine/src/search/pruning.h) / [`pruning.cpp`](file:///engine/src/search/pruning.cpp): Search branch pruning heuristics (Reverse Futility, Null-Move, Late Move Reductions).
* [`time_manager.h`](file:///engine/src/search/time_manager.h) / [`time_manager.cpp`](file:///engine/src/search/time_manager.cpp): Tournament time allocation and periodic clock polling.
* [`search.h`](file:///engine/src/search/search.h) / [`search.cpp`](file:///engine/src/search/search.cpp): Iterative deepening controller, aspiration windows, and negamax alpha-beta recursion.

---

## 2. Negamax with Alpha-Beta Pruning

Lookahead uses the mathematical identity that $\max(a, b) = -\min(-a, -b)$ to implement zero-branch symmetric minimax:
$$\text{negamax}(\text{node}) = \max_{m} (-\text{negamax}(\text{child}_m))$$

* $\alpha$ (alpha): Lower bound of guaranteed score.
* $\beta$ (beta): Upper bound allowed by the opponent.
* **Fail-High ($\ge \beta$)**: A beta cutoff occurs; remaining legal sibling moves are skipped.
* **Fail-Low ($\le \alpha$)**: The branch is worse than a previously discovered option.

---

## 3. Transposition Table (TT)

A fixed-size memory table hashes previously searched subtrees:
* **Index**: `key & size_mask` (table capacity rounded to power of 2).
* **Entry Layout**:
  - `uint64_t key`: Full 64-bit key preventing collision corruption.
  - `Move move`: Best move discovered at this node.
  - `int32_t score`: Position score adjusted for root distance.
  - `int16_t depth`: Remaining search depth.
  - `TTFlag flag`: `Exact` (PV), `LowerBound` (beta cutoff), or `UpperBound` (fail low).
  - `uint8_t age`: Generation age counter.

### Mate Score Normalization
To prevent transposition mate inaccuracies across varying search depths:
$$\text{score\_to\_tt}(\text{score}, \text{ply}) = \begin{cases} \text{score} + \text{ply} & \text{if score} \ge \text{MATE\_THRESHOLD} \\ \text{score} - \text{ply} & \text{if score} \le -\text{MATE\_THRESHOLD} \\ \text{score} & \text{otherwise} \end{cases}$$

---

## 4. Quiescence Search & Stand-Pat

To eliminate the **horizon effect** (where a disastrous piece loss is pushed just past search depth), leaf nodes transition into `quiescence()`:
1. **Stand-Pat**: Static evaluation is computed. If `stand_pat >= beta`, immediate cutoff occurs.
2. **Delta Pruning**: Captures that cannot mathematically raise $\alpha$ even with a large delta safety margin are skipped.
3. **MVV-LVA Capture Ordering**: Captures are sorted by Most Valuable Victim - Least Valuable Attacker:
   $$\text{priority} = \text{victim\_val} \times 10 - \text{attacker\_val}$$
   Ensuring $P \times Q$ is explored before $Q \times P$.

---

## 5. Move Ordering Hierarchy

Alpha-beta pruning efficiency is asymptotically optimal when the best move is explored first, reducing effective branching factor from $b$ towards $\sqrt{b}$. Lookahead organizes move evaluation into a strict 6-tier prioritization ladder:

| Tier | Heuristic | Score Range | Description |
| :--- | :--- | :--- | :--- |
| **1** | **Transposition Table (PV)** | `2,000,000` | Best move from shallower iteration or previous subtree search |
| **2** | **MVV-LVA Captures** | `1,000,000 - 1,050,000` | Winning and equal captures ranked by victim value minus attacker value |
| **3** | **Promotions** | `900,000 - 950,000` | Pawn promotions ranked by resulting piece type |
| **4** | **Killer 1** | `800,000` | Most recent quiet move causing a $\beta$-cutoff at this search ply |
| **5** | **Killer 2** | `700,000` | Secondary quiet move causing a $\beta$-cutoff at this search ply |
| **6** | **History Heuristic** | `0 - 50,000` | Butterfly table `history[color][from][to]` incremented by $\text{depth}^2$ on cutoffs |

### Lazy Selection Sorting (`pick_next_move`)
Rather than sorting all $N$ legal moves upfront with an expensive $O(N \log N)$ algorithm, Lookahead employs in-place selection sort:
- Scores are pre-computed into a stack-allocated buffer (`std::array<int, 256>`).
- On step $i$, the move with the maximum score in $[i, N)$ is swapped into index $i$.
- If a $\beta$-cutoff occurs on move 0 or move 1 (true in over 80% of chess tree positions), the remaining moves are never sorted or swapped, achieving maximum speed and zero dynamic allocations.

---

## 6. Pruning Heuristics & Reductions

Lookahead implements three forward-pruning and reduction algorithms:

### A. Reverse Futility Pruning (Static Null Move)
At frontier nodes ($d \le 3$), if static evaluation exceeds $\beta$ by a depth-dependent linear margin:
$$\text{margin} = 120 \times d$$
$$\text{static\_eval} - \text{margin} \ge \beta \implies \text{return } \beta$$
Guards: Active only when side to move is not in check and score is below checkmate threshold.

### B. Null-Move Pruning (NMP)
Simulates passing the turn to test if the opponent cannot beat $\beta$ even with two consecutive moves:
$$R = 2 + \lfloor d / 6 \rfloor$$
$$\text{score} = -\text{negamax}(\text{null\_position}, d - 1 - R, \text{ply} + 1, -\beta, -\beta + 1)$$
$$\text{score} \ge \beta \implies \text{return } \beta$$
Guards: Never performed in pure pawn endgames (`has_non_pawn_material()` verification) to avoid catastrophic zugzwang errors.

### C. Late Move Reductions (LMR)
Quiet moves searched late in the move list ($k \ge 4$) at depth $d \ge 3$ are searched with reduced depth:
$$R = \text{clamp}\left(\left\lfloor 0.75 + \frac{\ln(d) \times \ln(k)}{2.25} \right\rfloor, 1, d - 2\right)$$
Exemption Guard: Moves that give check (`gives_check == true`) are strictly exempted from reduction to guarantee tactical mate accuracy.
Re-search: If reduced search fails high ($\text{score} > \alpha$), a full-depth re-search is immediately triggered.

---

## 7. Time Management & Tournament Clock

The `TimeManager` subsystem allocates per-move search budgets:
- Exact time: Used directly when `move_time_ms` is specified.
- Tournament clock: Uses remaining time divided by estimated moves to go plus increment:
  $$\text{target\_time} = \frac{\text{time}}{\min(\text{moves\_to\_go}, 40)} + \frac{3}{4} \times \text{increment}$$
  $$\text{hard\_limit} = \min(\text{time} - 50\text{ms}, 3 \times \text{target\_time})$$
- Periodic Polling: Every 2048 nodes (`nodes & 2047 == 0`), the wall clock is checked. When time expires, search unrolls cleanly, preserving the best move and principal variation from the last fully completed iterative deepening depth.

---

## 8. Principal Variation & Streaming Telemetry

Lookahead tracks the principal variation line via a stack-allocated Triangular PV Table (`PVTable`):
- $O(1)$ stack allocation (zero heap overhead).
- On each alpha raise, the move chain is copied from `pv[ply + 1]` into `pv[ply]`.
- Provides deterministic, non-corruptible PV lines for both native UCI output and web visualization.

---

## 9. Tactical Benchmark Suite (`data/mate_puzzles.epd`)

An automated EPD suite tests the engine against classic checkmates:
- Mate in 1: Scholar's Mate, Back-Rank Mate, Fool's Mate Theme, Corner Rook Mate.
- Mate in 2: Paul Morphy's Opera House Queen Sacrifice, Queen Sacrifice Back-Rank, Double Rook Back-Rank.
- Mate in 3: Anastasia's Knight + Queen Mate.
All positions verified to solve in under 40 milliseconds total.


