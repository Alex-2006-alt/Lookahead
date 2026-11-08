# 03 - Position Evaluation

Lookahead implements a Hand-Crafted Evaluation (HCE) with continuous phase tapering, strict mathematical symmetry, and side-to-move score perspective.

---

## 1. Architecture & Layer Boundaries

The evaluation layer (`engine/src/eval/`) resides at Layer 2:
$$\text{core (0)} \leftarrow \text{movegen (1)} \leftarrow \text{eval (2)}$$

* [`evaluate.h`](file:///engine/src/eval/evaluate.h) / [`evaluate.cpp`](file:///engine/src/eval/evaluate.cpp): Primary evaluation interface, phase tapering, and UI term breakdown container.
* [`material_psqt.h`](file:///engine/src/eval/material_psqt.h) / [`material_psqt.cpp`](file:///engine/src/eval/material_psqt.cpp): PeSTO material values, game phase weights, and 64-square piece-square tables.

---

## 2. Score Representation & Tapering

Lookahead evaluates every position in dual-phase coordinates via the lightweight struct `Score`:
```cpp
struct Score {
    int32_t mg = 0; // Middlegame value (centipawns)
    int32_t eg = 0; // Endgame value (centipawns)
};
```

### Game Phase Calculation
The non-pawn material phase $\text{phase} \in [0, 24]$ transitions smoothly between the opening/middlegame and the endgame:
$$\text{phase} = \min(24, \sum_{\text{piece}} \text{count}(\text{pt}) \times \text{weight}(\text{pt}))$$

Weights:
* Knight: 1
* Bishop: 1
* Rook: 2
* Queen: 4
* Pawn & King: 0

$$\text{score}_{\text{tapered}} = \frac{S_{\text{mg}} \times \text{phase} + S_{\text{eg}} \times (24 - \text{phase})}{24}$$

---

## 3. PeSTO Piece-Square Tables (PSQT)

Positional piece bonuses derive from the PeSTO evaluation tables by Tomasz Michniewski and Róbert Houdart (credited in [`docs/REFERENCES.md`](file:///docs/REFERENCES.md)):
* Pawns are rewarded for advancing towards promotion ranks and maintaining center control.
* Knights and Bishops are rewarded for occupying central outposts over rim squares.
* Rooks are rewarded for 7th rank infiltration.
* Kings are rewarded for castled shelter in the middlegame and central activity in the endgame.

### Mathematical Symmetry & Rank Inversion
Tables are indexed by square $[0, 63]$ ($0 = \text{A1}, 63 = \text{H8}$).
* White accesses `table[sq]`.
* Black accesses `table[sq ^ 56]` (vertical rank flip: rank 1 $\leftrightarrow$ rank 8, rank 2 $\leftrightarrow$ rank 7).

This guarantees that:
$$\text{eval}(\text{position}) = -\text{eval}(\text{mirrored\_position})$$
Every evaluation component is verified against vertical flip symmetry across diverse positions.

---

## 4. Detailed Term Breakdown for Visualization

For the "Inside Lookahead" search visualizer in Phase 9, `evaluate_detailed()` populates `EvalBreakdown`:
```cpp
struct EvalBreakdown {
    Score material;
    Score psqt;
    Score pawns;
    Score king_safety;
    Score mobility;
    int game_phase;
    int tapered_total;
};
```
All components are exposed in the engine CLI via `./lookahead eval [fen]` and to the WebAssembly worker.
