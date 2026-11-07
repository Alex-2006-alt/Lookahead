# 05 - Search Speedups & Heuristics

## 1. Transposition Table (TT)
A fixed-size hash table indexed by Zobrist keys storing:
* 64-bit key / verification tag
* Best move found at this position
* Search depth
* Score (adjusted for distance to root for checkmate scores)
* Node bound type: `EXACT`, `LOWERBOUND` (fail-high / beta cutoff), `UPPERBOUND` (fail-low / all-node)

---

## 2. Move Ordering Heuristics
Alpha-beta search reaches optimal $O(\sqrt{b^d})$ efficiency only when the best move is examined first:
1. **TT Move**: The best move recorded in the transposition table from previous searches.
2. **MVV-LVA**: Most Valuable Victim - Least Valuable Attacker for captures (e.g., $P \times Q$ before $Q \times P$).
3. **Killer Moves**: Non-capture moves that caused beta cutoffs at the same ply in sibling branches.
4. **History Heuristic**: Butterfly table tracking moves that frequently produce cutoffs throughout the search tree.

---

## 3. Quiescence Search
Mitigates the **horizon effect** (where a tactical sequence like an exchange is cut off prematurely):
* Continues searching tactical captures and promotions until the position is "quiet".
* Incorporates a "stand-pat" lower bound so the player can refuse to capture if all available captures lose material.

---

## 4. Pruning and Reduction Techniques
* **Null-Move Pruning (NMP)**: Gives the opponent a free move. If the score is still $\ge \beta$, the position is so strong that deep search can be safely skipped (with verification against zugzwang).
* **Late Move Reductions (LMR)**: Moves searched late in the move order are likely suboptimal and are searched with reduced depth ($d - 1$ or $d - 2$). If they beat $\alpha$, they are re-searched at full depth.
