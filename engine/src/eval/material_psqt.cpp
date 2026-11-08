/*
 * What: PeSTO piece-square table definitions and material evaluation functions.
 * Why:  Implements positional piece bonuses and material balance from White view.
 * Layer: Layer 2 (eval) - depends only on material_psqt.h and core types.
 */

#include "material_psqt.h"
#include "core/bitboard.h"

namespace lookahead::eval {

// PeSTO Piece-Square Tables mapped to Lookahead PieceType 0..6 and square indexing (0 = A1, 63 = H8).
inline constexpr std::array<std::array<Score, 64>, 7> PSQT_TABLE = {{
    // None
    {{
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0)
    }},
    // Pawn
    {{
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0),
        Score( -35,   13), Score(  -1,    8), Score( -20,    8), Score( -23,   10), Score( -15,   13), Score(  24,    0), Score(  38,    2), Score( -22,   -7),
        Score( -26,    4), Score(  -4,    7), Score(  -4,   -6), Score( -10,    1), Score(   3,    0), Score(   3,   -5), Score(  33,   -1), Score( -12,   -8),
        Score( -27,   13), Score(  -2,    9), Score(  -5,   -3), Score(  12,   -7), Score(  17,   -7), Score(   6,   -8), Score(  10,    3), Score( -25,   -1),
        Score( -14,   32), Score(  13,   24), Score(   6,   13), Score(  21,    5), Score(  23,   -2), Score(  12,    4), Score(  17,   17), Score( -23,   17),
        Score(  -6,   94), Score(   7,  100), Score(  26,   85), Score(  31,   67), Score(  65,   56), Score(  56,   53), Score(  25,   82), Score( -20,   84),
        Score(  98,  178), Score( 134,  173), Score(  61,  158), Score(  95,  134), Score(  68,  147), Score( 126,  132), Score(  34,  165), Score( -11,  187),
        Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0), Score(   0,    0)
    }},
    // Knight
    {{
        Score(-105,  -29), Score( -21,  -51), Score( -58,  -23), Score( -33,  -15), Score( -17,  -22), Score( -28,  -18), Score( -19,  -50), Score( -23,  -64),
        Score( -29,  -42), Score( -53,  -20), Score( -12,  -10), Score(  -3,   -5), Score(  -1,   -2), Score(  18,  -20), Score( -14,  -23), Score( -19,  -44),
        Score( -23,  -23), Score(  -9,   -3), Score(  12,   -1), Score(  10,   15), Score(  19,   10), Score(  17,   -3), Score(  25,  -20), Score( -16,  -22),
        Score( -13,  -18), Score(   4,   -6), Score(  16,   16), Score(  13,   25), Score(  28,   16), Score(  19,   17), Score(  21,    4), Score(  -8,  -18),
        Score(  -9,  -17), Score(  17,    3), Score(  19,   22), Score(  53,   22), Score(  37,   22), Score(  69,   11), Score(  18,    8), Score(  22,  -18),
        Score( -47,  -24), Score(  60,  -20), Score(  37,   10), Score(  65,    9), Score(  84,   -1), Score( 129,   -9), Score(  73,  -19), Score(  44,  -41),
        Score( -73,  -25), Score( -41,   -8), Score(  72,  -25), Score(  36,   -2), Score(  23,   -9), Score(  62,  -25), Score(   7,  -24), Score( -17,  -52),
        Score(-167,  -58), Score( -89,  -38), Score( -34,  -13), Score( -49,  -28), Score(  61,  -31), Score( -97,  -27), Score( -15,  -63), Score(-107,  -99)
    }},
    // Bishop
    {{
        Score( -33,  -23), Score(  -3,   -9), Score( -14,  -23), Score( -21,   -5), Score( -13,   -9), Score( -12,  -16), Score( -39,   -5), Score( -21,  -17),
        Score(   4,  -14), Score(  15,  -18), Score(  16,   -7), Score(   0,   -1), Score(   7,    4), Score(  21,   -9), Score(  33,  -15), Score(   1,  -27),
        Score(   0,  -12), Score(  15,   -3), Score(  15,    8), Score(  15,   10), Score(  14,   13), Score(  27,    3), Score(  18,   -7), Score(  10,  -15),
        Score(  -6,   -6), Score(  13,    3), Score(  13,   13), Score(  26,   19), Score(  34,    7), Score(  12,   10), Score(  10,   -3), Score(   4,   -9),
        Score(  -4,   -3), Score(   5,    9), Score(  19,   12), Score(  50,    9), Score(  37,   14), Score(  37,   10), Score(   7,    3), Score(  -2,    2),
        Score( -16,    2), Score(  37,   -8), Score(  43,    0), Score(  40,   -1), Score(  35,   -2), Score(  50,    6), Score(  37,    0), Score(  -2,    4),
        Score( -26,   -8), Score(  16,   -4), Score( -18,    7), Score( -13,  -12), Score(  30,   -3), Score(  59,  -13), Score(  18,   -4), Score( -47,  -14),
        Score( -29,  -14), Score(   4,  -21), Score( -82,  -11), Score( -37,   -8), Score( -25,   -7), Score( -42,   -9), Score(   7,  -17), Score(  -8,  -24)
    }},
    // Rook
    {{
        Score( -19,   -9), Score( -13,    2), Score(   1,    3), Score(  17,   -1), Score(  16,   -5), Score(   7,  -13), Score( -37,    4), Score( -26,  -20),
        Score( -44,   -6), Score( -16,   -6), Score( -20,    0), Score(  -9,    2), Score(  -1,   -9), Score(  11,   -9), Score(  -6,  -11), Score( -71,   -3),
        Score( -45,   -4), Score( -25,    0), Score( -16,   -5), Score( -17,   -1), Score(   3,   -7), Score(   0,  -12), Score(  -5,   -8), Score( -33,  -16),
        Score( -36,    3), Score( -26,    5), Score( -12,    8), Score(  -1,    4), Score(   9,   -5), Score(  -7,   -6), Score(   6,   -8), Score( -23,  -11),
        Score( -24,    4), Score( -11,    3), Score(   7,   13), Score(  26,    1), Score(  24,    2), Score(  35,    1), Score(  -8,   -1), Score( -20,    2),
        Score(  -5,    7), Score(  19,    7), Score(  26,    7), Score(  36,    5), Score(  17,    4), Score(  45,   -3), Score(  61,   -5), Score(  16,   -3),
        Score(  27,   11), Score(  32,   13), Score(  58,   13), Score(  62,   11), Score(  80,   -3), Score(  67,    3), Score(  26,    8), Score(  44,    3),
        Score(  32,   13), Score(  42,   10), Score(  32,   18), Score(  51,   15), Score(  63,   12), Score(   9,   12), Score(  31,    8), Score(  43,    5)
    }},
    // Queen
    {{
        Score(  -1,  -33), Score( -18,  -28), Score(  -9,  -22), Score(  10,  -43), Score( -15,   -5), Score( -25,  -32), Score( -24,  -20), Score( -19,  -41),
        Score( -35,  -22), Score(  -8,  -23), Score(  11,  -30), Score(   2,  -16), Score(   8,  -16), Score(  15,  -23), Score(  -3,  -36), Score(   1,  -32),
        Score( -14,  -16), Score(   2,  -27), Score( -11,   15), Score(  -2,    6), Score(  -5,    9), Score(   2,   17), Score(  14,   10), Score(   5,    5),
        Score(  -9,  -18), Score( -26,   28), Score(  -9,   19), Score( -10,   47), Score(  -2,   31), Score(  -4,   34), Score(   3,   39), Score(  -3,   18),
        Score( -27,    3), Score( -27,   22), Score( -16,   24), Score( -16,   45), Score(  -1,   57), Score(  17,   40), Score(  -2,   57), Score(   1,   36),
        Score( -13,  -20), Score( -17,    6), Score(   7,    9), Score(   8,   49), Score(  29,   47), Score(  56,   35), Score(  47,   19), Score(  57,    9),
        Score( -24,  -17), Score( -39,   20), Score(  -5,   32), Score(   1,   41), Score( -16,   58), Score(  57,   25), Score(  28,   30), Score(  54,    0),
        Score( -28,   -9), Score(   0,   22), Score(  29,   22), Score(  12,   27), Score(  59,   27), Score(  44,   19), Score(  43,   10), Score(  45,   20)
    }},
    // King
    {{
        Score( -15,  -73), Score(  36,  -40), Score(  12,  -45), Score( -54,  -39), Score(   8,  -73), Score( -28,  -44), Score(  24,  -51), Score(  14,  -84),
        Score(   1,  -48), Score(   7,  -24), Score(  -8,  -18), Score( -64,  -13), Score( -43,  -13), Score( -16,  -12), Score(   9,  -24), Score(   8,  -48),
        Score( -14,  -17), Score( -14,  -20), Score( -22,  -12), Score( -46,   -6), Score( -44,   12), Score( -30,   18), Score( -15,   -6), Score( -27,  -20),
        Score( -49,  -18), Score(  -1,   -4), Score( -27,   21), Score( -39,   24), Score( -46,   16), Score( -44,   52), Score( -33,    9), Score( -51,  -29),
        Score( -17,   -8), Score( -20,   22), Score( -12,   24), Score( -27,   27), Score( -30,   26), Score( -25,   33), Score( -14,   26), Score( -36,    3),
        Score(  -9,   10), Score(  24,   17), Score(   2,   23), Score( -16,   15), Score( -20,   20), Score(   6,   45), Score(  22,   44), Score( -22,   13),
        Score(  29,  -12), Score(  -1,   17), Score( -20,   14), Score(  -7,   17), Score(  -8,   17), Score(  -4,   38), Score( -38,   23), Score( -29,   11),
        Score( -65,  -74), Score(  23,  -35), Score(  16,  -18), Score( -15,  -18), Score( -56,  -11), Score( -34,   15), Score(   2,    4), Score(  13,  -17)
    }},
}};

Score psqt_value(core::PieceType pt, core::Square sq) noexcept {
    return PSQT_TABLE[static_cast<size_t>(pt)][static_cast<size_t>(sq)];
}

std::pair<Score, Score> evaluate_material_and_psqt(const core::Position& pos) noexcept {
    Score material_diff{0, 0};
    Score psqt_diff{0, 0};

    for (uint8_t pt_val = 1; pt_val <= 6; ++pt_val) {
        const auto pt = static_cast<core::PieceType>(pt_val);
        const Score val = PIECE_VALUES[pt_val];

        core::Bitboard white_pieces = pos.pieces(core::Color::White, pt);
        while (white_pieces) {
            const auto sq = core::pop_lsb(white_pieces);
            material_diff += val;
            psqt_diff += psqt_value(pt, sq);
        }

        core::Bitboard black_pieces = pos.pieces(core::Color::Black, pt);
        while (black_pieces) {
            const auto sq = core::pop_lsb(black_pieces);
            material_diff -= val;
            // Mirror rank for Black (sq ^ 56) to maintain exact symmetry
            const auto mirrored_sq = static_cast<core::Square>(static_cast<uint8_t>(sq) ^ 56);
            psqt_diff -= psqt_value(pt, mirrored_sq);
        }
    }
    return {material_diff, psqt_diff};
}

int calculate_game_phase(const core::Position& pos) noexcept {
    int phase = 0;
    // Non-pawn pieces: Knight(2), Bishop(3), Rook(4), Queen(5)
    for (uint8_t pt_val = 2; pt_val <= 5; ++pt_val) {
        const auto pt = static_cast<core::PieceType>(pt_val);
        const int count = core::popcount(pos.pieces(pt));
        phase += count * PIECE_PHASE[pt_val];
    }
    return (phase > TOTAL_PHASE) ? TOTAL_PHASE : phase;
}

}  // namespace lookahead::eval
