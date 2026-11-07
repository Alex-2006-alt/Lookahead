# 04 - Alpha-Beta Search & Iterative Deepening

## Minimax & Negamax

Lookahead uses the **Negamax** formulation of the Minimax algorithm:
$$\text{negamax}(\text{node}) = \max_{m} (-\text{negamax}(\text{child}_m))$$
This eliminates the need for alternating min/max code paths and simplifies the search structure.

---

## Alpha-Beta Pruning

Alpha ($\alpha$) represents the minimum score the maximizing player is assured of.
Beta ($\beta$) represents the maximum score the minimizing player allows.

If a move produces a score $v \ge \beta$, the opponent would never allow this position to be reached. This causes a **beta cutoff** (or fail-high), pruning the remaining subtrees.

```
                  Root (Alpha, Beta)
                 /                  \
            Child 1 (Searched)    Child 2 (Pruned!)
             v >= Beta cutoff
```

---

## Iterative Deepening

Rather than searching directly to depth $D$, Lookahead searches depth 1, then depth 2, ..., up to depth $D$:
* Populates the transposition table with accurate best moves from previous iterations.
* Allows move ordering at depth $N$ to be seeded with the principal variation from depth $N-1$, dramatically improving alpha-beta cutoff efficiency.
* Enables robust time management: the search can be stopped at any time after completing any full depth iteration.
