/*
 * What: Quiescence search module exploring capturing moves to eliminate horizon effect.
 * Why:  Stabilizes static evaluation at leaf nodes via stand-pat and capture resolution.
 * Layer: Layer 3 (search) - depends on core, movegen, eval, and features.
 */

#ifndef LOOKAHEAD_SEARCH_QUIESCENCE_H
#define LOOKAHEAD_SEARCH_QUIESCENCE_H

#include <cstdint>
#include "core/position.h"

namespace lookahead::search {

// Explores tactical capture chains until a stable quiescent position is reached.
[[nodiscard]] int quiescence(core::Position& pos, int alpha, int beta, uint64_t& nodes) noexcept;

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_QUIESCENCE_H
