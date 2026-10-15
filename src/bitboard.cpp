// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "bitboard.h"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <string>

namespace sp {

Bitboard     BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard     LineBB[SQUARE_NB][SQUARE_NB];
Bitboard     PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
Bitboard     PawnAttacks[COLOR_NB][SQUARE_NB];
std::uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

Magic RookMagics[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];

namespace {

Bitboard RookTable[0x19000];
Bitboard BishopTable[0x1480];

// xorshift64* generator, used only to find magic numbers at startup.
class PRNG {
    std::uint64_t s;

    std::uint64_t rand64() {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }

   public:
    explicit PRNG(std::uint64_t seed) : s(seed) { assert(seed); }

    std::uint64_t rand() { return rand64(); }

    // Numbers with few set bits make good magic candidates.
    std::uint64_t sparse_rand() { return rand64() & rand64() & rand64(); }
};

Bitboard safe_destination(Square s, int step) {
    const Square to = Square(s + step);
    return is_ok(to) && distance(s, to) <= 2 ? square_bb(to) : Bitboard(0);
}

// Attacks of a rook or bishop from square sq with the given occupancy, computed
// the slow way. Used to fill the magic tables.
Bitboard sliding_attack(PieceType pt, Square sq, Bitboard occupied) {
    Bitboard  attacks             = 0;
    Direction rookDirections[4]   = {NORTH, SOUTH, EAST, WEST};
    Direction bishopDirections[4] = {NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};

    for (Direction d : (pt == ROOK ? rookDirections : bishopDirections)) {
        Square s = sq;
        while (safe_destination(s, d)) {
            attacks |= (s += d);
            if (occupied & s)
                break;
        }
    }
    return attacks;
}

// Computes all rook and bishop attacks at startup. The magic numbers are found
// by trial and error from sparse random candidates; the seeds below make the
// search converge quickly.
void init_magics(PieceType pt, Bitboard table[], Magic magics[]) {
    constexpr int seeds[RANK_NB] = {728, 10316, 55013, 32803, 12281, 15100, 16645, 255};

    Bitboard occupancy[4096], reference[4096], edges, b;
    int      epoch[4096] = {}, cnt = 0, size = 0;

    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        // Board edges are not considered in the relevant occupancies: a piece on
        // an edge square blocks nothing further along the ray.
        edges = ((Rank1BB | Rank8BB) & ~rank_bb(s)) | ((FileABB | FileHBB) & ~file_bb(s));

        Magic& m = magics[s];
        m.mask   = sliding_attack(pt, s, 0) & ~edges;
        m.shift  = 64 - unsigned(popcount(m.mask));

        // Each square's attack table follows the previous square's in the shared table.
        m.attacks = s == SQ_A1 ? table : magics[s - 1].attacks + size;

        // Enumerate every subset of the mask with the Carry-Rippler trick and
        // store the reference attacks for it.
        b = size = 0;
        do {
            occupancy[size] = b;
            reference[size] = sliding_attack(pt, s, b);

            if constexpr (HasPext)
                m.attacks[m.index(b)] = reference[size];

            size++;
            b = (b - m.mask) & m.mask;
        } while (b);

        if constexpr (HasPext)
            continue;

        PRNG rng(seeds[rank_of(s)]);

        // Find a magic that maps every relevant occupancy to a distinct index,
        // or to the same index when the attack sets happen to be identical.
        for (int i = 0; i < size;) {
            for (m.magic = 0; popcount((m.magic * m.mask) >> 56) < 6;)
                m.magic = rng.sparse_rand();

            // The epoch counter lets us reuse the attack table across attempts
            // without clearing it.
            for (++cnt, i = 0; i < size; ++i) {
                unsigned idx = m.index(occupancy[i]);

                if (epoch[idx] < cnt) {
                    epoch[idx]     = cnt;
                    m.attacks[idx] = reference[i];
                } else if (m.attacks[idx] != reference[i])
                    break;
            }
        }
    }
}

}  // namespace

void Bitboards::init() {
    for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
        for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
            SquareDistance[s1][s2] = std::uint8_t(std::max(distance<File>(s1, s2), distance<Rank>(s1, s2)));

    init_magics(ROOK, RookTable, RookMagics);
    init_magics(BISHOP, BishopTable, BishopMagics);

    for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1) {
        PawnAttacks[WHITE][s1] = pawn_attacks_bb<WHITE>(square_bb(s1));
        PawnAttacks[BLACK][s1] = pawn_attacks_bb<BLACK>(square_bb(s1));

        for (int step : {-9, -8, -7, -1, 1, 7, 8, 9})
            PseudoAttacks[KING][s1] |= safe_destination(s1, step);

        for (int step : {-17, -15, -10, -6, 6, 10, 15, 17})
            PseudoAttacks[KNIGHT][s1] |= safe_destination(s1, step);

        PseudoAttacks[QUEEN][s1] = PseudoAttacks[BISHOP][s1] = attacks_bb<BISHOP>(s1, 0);
        PseudoAttacks[QUEEN][s1] |= PseudoAttacks[ROOK][s1]  = attacks_bb<ROOK>(s1, 0);

        for (PieceType pt : {BISHOP, ROOK})
            for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
                if (PseudoAttacks[pt][s1] & s2) {
                    LineBB[s1][s2]    = (attacks_bb(pt, s1, 0) & attacks_bb(pt, s2, 0)) | s1 | s2;
                    BetweenBB[s1][s2] = attacks_bb(pt, s1, square_bb(s2)) & attacks_bb(pt, s2, square_bb(s1));
                }

        for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
            BetweenBB[s1][s2] |= s2;
    }
}

std::string Bitboards::pretty(Bitboard b) {
    std::string s = "+---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_H; ++f)
            s += b & make_square(f, r) ? "| X " : "|   ";
        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
    }
    s += "  a   b   c   d   e   f   g   h\n";
    return s;
}

std::string square_to_string(Square s) {
    if (!is_ok(s))
        return "-";
    return std::string{char('a' + file_of(s)), char('1' + rank_of(s))};
}

std::string move_to_uci(Move m) {
    if (m == Move::none())
        return "(none)";
    if (m == Move::null())
        return "0000";

    std::string s = square_to_string(m.from()) + square_to_string(m.to());
    if (m.type() == PROMOTION)
        s += " pnbrqk"[m.promotion_type()];
    return s;
}

}  // namespace sp
