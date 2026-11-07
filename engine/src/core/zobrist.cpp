/*
 * What: Implementation of deterministic 64-bit Zobrist key tables using SplitMix64.
 * Why:  Guarantees identical, cross-platform pseudo-random hash keys across native and WASM.
 * Layer: Layer 0 (core) - depends only on zobrist.h and standard C++ headers.
 */

#include "zobrist.h"
#include <array>

namespace lookahead::core::zobrist {

namespace {

// SplitMix64 generator with a fixed seed for 100% reproducible hash keys.
uint64_t splitmix64(uint64_t& state) noexcept {
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Fixed seed guaranteeing identical keys across compiles and targets
constexpr uint64_t SEED = 0x123456789ABCDEF0ULL;

std::array<std::array<Key, 64>, 16> s_piece_keys{};
std::array<Key, 16> s_castling_keys{};
std::array<Key, 8> s_en_passant_keys{};
Key s_side_key = 0ULL;
bool s_initialized = false;

}  // namespace

void init() noexcept {
    if (s_initialized) {
        return;
    }

    uint64_t state = SEED;

    for (size_t piece = 0; piece < 16; ++piece) {
        for (size_t sq = 0; sq < 64; ++sq) {
            s_piece_keys[piece][sq] = splitmix64(state);
        }
    }

    for (size_t c = 0; c < 16; ++c) {
        s_castling_keys[c] = splitmix64(state);
    }

    for (size_t f = 0; f < 8; ++f) {
        s_en_passant_keys[f] = splitmix64(state);
    }

    s_side_key = splitmix64(state);
    s_initialized = true;
}

Key piece(Piece p, Square sq) noexcept {
    if (!s_initialized) {
        init();
    }
    const auto p_idx = static_cast<uint8_t>(p);
    const auto sq_idx = static_cast<uint8_t>(sq);
    if (p_idx >= 16 || sq_idx >= 64) {
        return KeyEmpty;
    }
    return s_piece_keys[p_idx][sq_idx];
}

Key castling(uint8_t rights) noexcept {
    if (!s_initialized) {
        init();
    }
    return s_castling_keys[rights & 0x0F];
}

Key en_passant(File f) noexcept {
    if (!s_initialized) {
        init();
    }
    const auto f_idx = static_cast<uint8_t>(f);
    if (f_idx >= 8) {
        return KeyEmpty;
    }
    return s_en_passant_keys[f_idx];
}

Key side_to_move() noexcept {
    if (!s_initialized) {
        init();
    }
    return s_side_key;
}

}  // namespace lookahead::core::zobrist
