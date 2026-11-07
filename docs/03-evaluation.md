# 03 - Position Evaluation

## Evaluation Philosophy

The evaluation function maps a board position to a numerical score in centipawns ($100 \text{ cp} \approx 1 \text{ pawn}$). A positive score indicates an advantage for the side to move (negamax convention).

---

## Tapered Evaluation

Chess values change drastically between the opening and the endgame (e.g. king safety is critical in the middlegame, while king activity and passed pawns dominate the endgame). Lookahead computes two scores simultaneously:
* **Middlegame Score ($S_{MG}$)**
* **Endgame Score ($S_{EG}$)**

The game phase $\phi \in [0, 24]$ is derived from non-pawn material:
$$\text{Phase} = \sum (\text{count}(N) \times 1 + \text{count}(B) \times 1 + \text{count}(R) \times 2 + \text{count}(Q) \times 4)$$
$$\text{Eval} = \frac{S_{MG} \times \text{Phase} + S_{EG} \times (24 - \text{Phase})}{24}$$

---

## Evaluation Terms

1. **Material & Piece-Square Tables (PSQT)**: Base values combined with positional incentives (e.g. knights in the center, rooks on open files, kings castled).
2. **Pawn Structure**: Penalties for doubled and isolated pawns; bonuses for passed and connected pawns.
3. **King Safety**: Pawn shield evaluation, open file penalties near the king, and attacking piece proximity.
4. **Mobility**: Safe destination counts for minor and major pieces.

All terms are individually toggleable via feature flags for ablation testing.
