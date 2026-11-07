/*
 * What: Implementation of leaping attack tables and Magic Bitboards for sliders.
 * Why:  Computes move targets in O(1) time without looping over rays at search runtime.
 * Layer: Layer 1 (movegen) - depends only on attacks.h, core types, and core bitboard.
 */

#include "attacks.h"
#include <array>

namespace lookahead::movegen::attacks {

namespace {

constexpr std::array<uint64_t, 64> BISHOP_MAGICS = {
    0x40200411020C0210ULL, 0x0804081800408220ULL, 0x0016040409801008ULL, 0x1804104208204202ULL,
    0x0022021040240508ULL, 0x0001040240488020ULL, 0x0102020121080001ULL, 0x4052804808900844ULL,
    0x02004002082200A0ULL, 0x0008082128022044ULL, 0x0050041414004004ULL, 0x0404040420810428ULL,
    0x0002011040820C00ULL, 0x2142820822880800ULL, 0x11000500B0110808ULL, 0x00800880A8882040ULL,
    0x0008081020088082ULL, 0x8238508288080080ULL, 0x0010204802202020ULL, 0x0168021082004500ULL,
    0x0004008201210000ULL, 0x4901802102A00101ULL, 0x0004048202492400ULL, 0x00020001212C2200ULL,
    0x0008404249100140ULL, 0x20080400C8010828ULL, 0x0004040002002405ULL, 0x0132040008009060ULL,
    0x0010840008802020ULL, 0x501401A044100400ULL, 0x02C08A3111080200ULL, 0x2014004941010080ULL,
    0x0202421090A02020ULL, 0x0014240480600100ULL, 0x2100425800100020ULL, 0x0042004040540100ULL,
    0x0020008400010410ULL, 0x0522080201004062ULL, 0x0201440080010814ULL, 0x0084124204088480ULL,
    0x84121820040C0880ULL, 0x00040082D0080840ULL, 0x0028220030021200ULL, 0x081020C208000081ULL,
    0x8A3082A009000200ULL, 0x0422009013000080ULL, 0x01080148008C2A20ULL, 0x4801380209810140ULL,
    0x0802089009080002ULL, 0x04004A0811089190ULL, 0x08100020A4100800ULL, 0x6001410205041800ULL,
    0x0000044208220160ULL, 0x0040200202820048ULL, 0x1808689004004000ULL, 0x00D00101252A0040ULL,
    0x2081412409201088ULL, 0x0200082504122042ULL, 0xB130616200A40402ULL, 0x100110000884240DULL,
    0x8080058240104100ULL, 0x0200000811010200ULL, 0x00A51A9010020340ULL, 0x4024700208002480ULL,
};

constexpr std::array<uint64_t, 64> ROOK_MAGICS = {
    0xDA80104000800020ULL, 0x0040002000100042ULL, 0xC900102000400B00ULL, 0x02001020400A0004ULL,
    0x0200200810020004ULL, 0x0280020004000180ULL, 0x4200280504820014ULL, 0x40800CC080002B00ULL,
    0x0006002902004080ULL, 0x0605004001008022ULL, 0x0002001080402208ULL, 0x0002801000480080ULL,
    0x1800800800040080ULL, 0x8450800400020080ULL, 0x4221008401000200ULL, 0x0002000100540082ULL,
    0x0030208000804000ULL, 0x1000810040002104ULL, 0x2060008010008020ULL, 0x8446020020100AC0ULL,
    0x0000828004000800ULL, 0x0080808004000200ULL, 0x0401040008020110ULL, 0x8038620004004081ULL,
    0x0440400080208000ULL, 0x0800400100210080ULL, 0x4400200080801000ULL, 0x0061002300100208ULL,
    0x080A008600082010ULL, 0x0000020080040080ULL, 0x2005050C00102208ULL, 0x2040411200008044ULL,
    0x0080004000402000ULL, 0x4890042004404002ULL, 0x0600801000802000ULL, 0x0201001001002008ULL,
    0x0020080080800400ULL, 0x0002000400800280ULL, 0x0C00418804004210ULL, 0x0000008842000C01ULL,
    0x0080008040008028ULL, 0x022000303010400CULL, 0x0090008020008010ULL, 0x0070100008008080ULL,
    0x0121000801110004ULL, 0x0002000804020010ULL, 0x0040020108040010ULL, 0x7040084481020024ULL,
    0x040C8700A6084200ULL, 0x100226410E008200ULL, 0x4000200080100080ULL, 0x0010008008001080ULL,
    0xA002908801000500ULL, 0x1A22001008040200ULL, 0x0000080250218400ULL, 0x0860344401008200ULL,
    0x808020401A008102ULL, 0x0021004000201081ULL, 0x800200720A2080C2ULL, 0x1010100120090005ULL,
    0x0002002005081002ULL, 0x0012008108045002ULL, 0x000008101A090084ULL, 0x2004102110428406ULL,
};

std::array<std::array<core::Bitboard, 64>, 2> s_pawn_attacks{};
std::array<core::Bitboard, 64> s_knight_attacks{};
std::array<core::Bitboard, 64> s_king_attacks{};
std::array<core::Bitboard, 64> s_bishop_masks{};
std::array<core::Bitboard, 64> s_rook_masks{};
std::array<uint8_t, 64> s_bishop_shifts{};
std::array<uint8_t, 64> s_rook_shifts{};

std::array<std::array<core::Bitboard, 512>, 64> s_bishop_table{};
std::array<std::array<core::Bitboard, 4096>, 64> s_rook_table{};
std::array<std::array<core::Bitboard, 64>, 64> s_between{};
std::array<std::array<core::Bitboard, 64>, 64> s_line{};
bool s_initialized = false;

core::Bitboard compute_bishop_mask(int sq) noexcept {
    int r = sq / 8, f = sq % 8;
    core::Bitboard mask = 0ULL;
    for (int dr : {-1, 1}) {
        for (int df : {-1, 1}) {
            for (int cr = r + dr, cf = f + df; cr >= 1 && cr <= 6 && cf >= 1 && cf <= 6; cr += dr, cf += df) {
                mask |= core::square_bb(core::make_square(static_cast<core::File>(cf), static_cast<core::Rank>(cr)));
            }
        }
    }
    return mask;
}

core::Bitboard compute_rook_mask(int sq) noexcept {
    int r = sq / 8, f = sq % 8;
    core::Bitboard mask = 0ULL;
    for (int cr = r + 1; cr <= 6; ++cr) mask |= core::square_bb(core::make_square(static_cast<core::File>(f), static_cast<core::Rank>(cr)));
    for (int cr = r - 1; cr >= 1; --cr) mask |= core::square_bb(core::make_square(static_cast<core::File>(f), static_cast<core::Rank>(cr)));
    for (int cf = f + 1; cf <= 6; ++cf) mask |= core::square_bb(core::make_square(static_cast<core::File>(cf), static_cast<core::Rank>(r)));
    for (int cf = f - 1; cf >= 1; --cf) mask |= core::square_bb(core::make_square(static_cast<core::File>(cf), static_cast<core::Rank>(r)));
    return mask;
}

core::Bitboard ray_bishop(int sq, core::Bitboard occ) noexcept {
    int r = sq / 8, f = sq % 8;
    core::Bitboard att = 0ULL;
    for (int dr : {-1, 1}) {
        for (int df : {-1, 1}) {
            for (int cr = r + dr, cf = f + df; cr >= 0 && cr <= 7 && cf >= 0 && cf <= 7; cr += dr, cf += df) {
                core::Square s = core::make_square(static_cast<core::File>(cf), static_cast<core::Rank>(cr));
                att |= core::square_bb(s);
                if (core::test_bit(occ, s)) break;
            }
        }
    }
    return att;
}

core::Bitboard ray_rook(int sq, core::Bitboard occ) noexcept {
    int r = sq / 8, f = sq % 8;
    core::Bitboard att = 0ULL;
    for (auto [dr, df] : std::initializer_list<std::pair<int, int>>{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
        for (int cr = r + dr, cf = f + df; cr >= 0 && cr <= 7 && cf >= 0 && cf <= 7; cr += dr, cf += df) {
            core::Square s = core::make_square(static_cast<core::File>(cf), static_cast<core::Rank>(cr));
            att |= core::square_bb(s);
            if (core::test_bit(occ, s)) break;
        }
    }
    return att;
}

}  // namespace

void init() noexcept {
    if (s_initialized) return;

    for (int sq = 0; sq < 64; ++sq) {
        core::Bitboard b = core::square_bb(static_cast<core::Square>(sq));
        s_pawn_attacks[0][sq] = core::shift_north_east(b) | core::shift_north_west(b);
        s_pawn_attacks[1][sq] = core::shift_south_east(b) | core::shift_south_west(b);

        int r = sq / 8, f = sq % 8;
        core::Bitboard knight_b = 0ULL;
        for (auto [dr, df] : std::initializer_list<std::pair<int, int>>{
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}}) {
            int nr = r + dr, nf = f + df;
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) knight_b |= core::square_bb(core::make_square(static_cast<core::File>(nf), static_cast<core::Rank>(nr)));
        }
        s_knight_attacks[sq] = knight_b;

        core::Bitboard king_b = 0ULL;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int df = -1; df <= 1; ++df) {
                if (dr == 0 && df == 0) continue;
                int nr = r + dr, nf = f + df;
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) king_b |= core::square_bb(core::make_square(static_cast<core::File>(nf), static_cast<core::Rank>(nr)));
            }
        }
        s_king_attacks[sq] = king_b;

        // Magic slider table population
        s_bishop_masks[sq] = compute_bishop_mask(sq);
        s_bishop_shifts[sq] = static_cast<uint8_t>(64 - core::popcount(s_bishop_masks[sq]));
        core::Bitboard b_sub = 0ULL;
        do {
            size_t idx = (b_sub * BISHOP_MAGICS[sq]) >> s_bishop_shifts[sq];
            s_bishop_table[sq][idx] = ray_bishop(sq, b_sub);
            b_sub = (b_sub - s_bishop_masks[sq]) & s_bishop_masks[sq];
        } while (b_sub != 0ULL);

        s_rook_masks[sq] = compute_rook_mask(sq);
        s_rook_shifts[sq] = static_cast<uint8_t>(64 - core::popcount(s_rook_masks[sq]));
        core::Bitboard r_sub = 0ULL;
        do {
            size_t idx = (r_sub * ROOK_MAGICS[sq]) >> s_rook_shifts[sq];
            s_rook_table[sq][idx] = ray_rook(sq, r_sub);
            r_sub = (r_sub - s_rook_masks[sq]) & s_rook_masks[sq];
        } while (r_sub != 0ULL);
    }

    // Between and Line rays
    for (int sq1 = 0; sq1 < 64; ++sq1) {
        for (int sq2 = 0; sq2 < 64; ++sq2) {
            if (sq1 == sq2) continue;
            core::Square s1 = static_cast<core::Square>(sq1), s2 = static_cast<core::Square>(sq2);
            core::Bitboard r_att = ray_rook(sq1, 0ULL), b_att = ray_bishop(sq1, 0ULL);
            if (core::test_bit(r_att, s2)) {
                s_line[sq1][sq2] = (ray_rook(sq1, 0ULL) & ray_rook(sq2, 0ULL)) | core::square_bb(s1) | core::square_bb(s2);
                s_between[sq1][sq2] = ray_rook(sq1, core::square_bb(s2)) & ray_rook(sq2, core::square_bb(s1));
            } else if (core::test_bit(b_att, s2)) {
                s_line[sq1][sq2] = (ray_bishop(sq1, 0ULL) & ray_bishop(sq2, 0ULL)) | core::square_bb(s1) | core::square_bb(s2);
                s_between[sq1][sq2] = ray_bishop(sq1, core::square_bb(s2)) & ray_bishop(sq2, core::square_bb(s1));
            }
        }
    }
    s_initialized = true;
}

core::Bitboard pawn(core::Color c, core::Square sq) noexcept {
    if (!s_initialized) init();
    return s_pawn_attacks[static_cast<uint8_t>(c)][static_cast<uint8_t>(sq)];
}

core::Bitboard knight(core::Square sq) noexcept {
    if (!s_initialized) init();
    return s_knight_attacks[static_cast<uint8_t>(sq)];
}

core::Bitboard king(core::Square sq) noexcept {
    if (!s_initialized) init();
    return s_king_attacks[static_cast<uint8_t>(sq)];
}

core::Bitboard bishop(core::Square sq, core::Bitboard occupied) noexcept {
    if (!s_initialized) init();
    const auto s = static_cast<uint8_t>(sq);
    const size_t idx = ((occupied & s_bishop_masks[s]) * BISHOP_MAGICS[s]) >> s_bishop_shifts[s];
    return s_bishop_table[s][idx];
}

core::Bitboard rook(core::Square sq, core::Bitboard occupied) noexcept {
    if (!s_initialized) init();
    const auto s = static_cast<uint8_t>(sq);
    const size_t idx = ((occupied & s_rook_masks[s]) * ROOK_MAGICS[s]) >> s_rook_shifts[s];
    return s_rook_table[s][idx];
}

core::Bitboard between(core::Square sq1, core::Square sq2) noexcept {
    if (!s_initialized) init();
    return s_between[static_cast<uint8_t>(sq1)][static_cast<uint8_t>(sq2)];
}

core::Bitboard line(core::Square sq1, core::Square sq2) noexcept {
    if (!s_initialized) init();
    return s_line[static_cast<uint8_t>(sq1)][static_cast<uint8_t>(sq2)];
}

}  // namespace lookahead::movegen::attacks
