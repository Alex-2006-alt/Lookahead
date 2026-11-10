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
* [`search.h`](file:///engine/src/search/search.h) / [`search.cpp`](file:///engine/src/search/search.cpp): Iterative deepening controller and negamax alpha-beta recursion.

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

