# 04 - Alpha-Beta Search, Transposition Table & Quiescence

Lookahead implements a high-performance Negamax $\alpha\beta$ tree search with iterative deepening, fixed-size Transposition Table caching, and tactical Quiescence search.

---

## 1. Architecture & Layer Boundaries

The search layer (`engine/src/search/`) sits at Layer 3:
$$\text{core (0)} \leftarrow \text{movegen (1)} \leftarrow \text{eval (2)} \leftarrow \text{search (3)}$$

* [`features.h`](file:///engine/src/search/features.h): Algorithmic feature toggles (`USE_TT`, `USE_QUIESCENCE`, etc.) for clean ablation testing.
* [`transposition_table.h`](file:///engine/src/search/transposition_table.h) / [`transposition_table.cpp`](file:///engine/src/search/transposition_table.cpp): 64-bit Zobrist keyed hash table with exact, lower-bound, and upper-bound flags.
* [`quiescence.h`](file:///engine/src/search/quiescence.h) / [`quiescence.cpp`](file:///engine/src/search/quiescence.cpp): Stand-pat capture search with MVV-LVA move ordering and delta pruning.
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
