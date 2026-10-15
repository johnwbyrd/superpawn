// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "types.h"

#if defined(SP_USE_PEXT) && (defined(__BMI2__) || defined(_MSC_VER))
    #include <immintrin.h>
#endif

namespace sp {

namespace Bitboards {
void        init();
std::string pretty(Bitboard b);
}  // namespace Bitboards

constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

constexpr Bitboard Rank1BB = 0xFF;
constexpr Bitboard Rank2BB = Rank1BB << (8 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (8 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (8 * 3);
constexpr Bitboard Rank5BB = Rank1BB << (8 * 4);
constexpr Bitboard Rank6BB = Rank1BB << (8 * 5);
constexpr Bitboard Rank7BB = Rank1BB << (8 * 6);
constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);

// Lookup tables filled by Bitboards::init().
extern Bitboard     BetweenBB[SQUARE_NB][SQUARE_NB];  // squares strictly between, plus the second square
extern Bitboard     LineBB[SQUARE_NB][SQUARE_NB];     // full line through both squares, or 0
extern Bitboard     PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard     PawnAttacks[COLOR_NB][SQUARE_NB];
extern std::uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

// Fancy magic bitboards for sliding pieces. With SP_USE_PEXT the index is a
// hardware PEXT instead of a multiply and shift.
struct Magic {
    Bitboard  mask;
    Bitboard  magic;
    Bitboard* attacks;
    unsigned  shift;

    unsigned index(Bitboard occupied) const {
        if constexpr (HasPext) {
#if defined(SP_USE_PEXT) && (defined(__BMI2__) || defined(_MSC_VER))
            return unsigned(_pext_u64(occupied, mask));
#endif
        }
        return unsigned(((occupied & mask) * magic) >> shift);
    }
};

extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];

constexpr Bitboard square_bb(Square s) {
    assert(is_ok(s));
    return 1ULL << s;
}

inline Bitboard operator&(Bitboard b, Square s) {
    return b & square_bb(s);
}
inline Bitboard operator|(Bitboard b, Square s) {
    return b | square_bb(s);
}
inline Bitboard operator^(Bitboard b, Square s) {
    return b ^ square_bb(s);
}
inline Bitboard& operator|=(Bitboard& b, Square s) {
    return b |= square_bb(s);
}
inline Bitboard& operator^=(Bitboard& b, Square s) {
    return b ^= square_bb(s);
}
inline Bitboard operator&(Square s, Bitboard b) {
    return b & s;
}
inline Bitboard operator|(Square s, Bitboard b) {
    return b | s;
}
inline Bitboard operator|(Square s1, Square s2) {
    return square_bb(s1) | s2;
}

constexpr bool more_than_one(Bitboard b) {
    return b & (b - 1);
}

constexpr Bitboard rank_bb(Rank r) {
    return Rank1BB << (8 * r);
}
constexpr Bitboard rank_bb(Square s) {
    return rank_bb(rank_of(s));
}
constexpr Bitboard file_bb(File f) {
    return FileABB << f;
}
constexpr Bitboard file_bb(Square s) {
    return file_bb(file_of(s));
}

template <Direction D>
constexpr Bitboard shift(Bitboard b) {
    return D == NORTH         ? b << 8
         : D == SOUTH         ? b >> 8
         : D == NORTH + NORTH ? b << 16
         : D == SOUTH + SOUTH ? b >> 16
         : D == EAST          ? (b & ~FileHBB) << 1
         : D == WEST          ? (b & ~FileABB) >> 1
         : D == NORTH_EAST    ? (b & ~FileHBB) << 9
         : D == NORTH_WEST    ? (b & ~FileABB) << 7
         : D == SOUTH_EAST    ? (b & ~FileHBB) >> 7
         : D == SOUTH_WEST    ? (b & ~FileABB) >> 9
                              : 0;
}

template <Color C>
constexpr Bitboard pawn_attacks_bb(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b) : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}

inline Bitboard pawn_attacks_bb(Color c, Square s) {
    assert(is_ok(s));
    return PawnAttacks[c][s];
}

inline Bitboard line_bb(Square s1, Square s2) {
    return LineBB[s1][s2];
}
inline Bitboard between_bb(Square s1, Square s2) {
    return BetweenBB[s1][s2];
}

// True when s1, s2 and s3 lie on a common rank, file or diagonal.
inline bool aligned(Square s1, Square s2, Square s3) {
    return line_bb(s1, s2) & s3;
}

template <typename T1 = Square>
inline int distance(Square x, Square y);
template <>
inline int distance<File>(Square x, Square y) {
    return std::abs(file_of(x) - file_of(y));
}
template <>
inline int distance<Rank>(Square x, Square y) {
    return std::abs(rank_of(x) - rank_of(y));
}
template <>
inline int distance<Square>(Square x, Square y) {
    return SquareDistance[x][y];
}

inline int edge_distance(File f) {
    return std::min(f, File(FILE_H - f));
}

// Attacks of a piece type from a square, given the occupancy for sliders.
template <PieceType Pt>
inline Bitboard attacks_bb(Square s) {
    assert(Pt != PAWN && is_ok(s));
    return PseudoAttacks[Pt][s];
}

template <PieceType Pt>
inline Bitboard attacks_bb(Square s, Bitboard occupied) {
    assert(Pt != PAWN && is_ok(s));
    switch (Pt) {
    case BISHOP :
        return BishopMagics[s].attacks[BishopMagics[s].index(occupied)];
    case ROOK :
        return RookMagics[s].attacks[RookMagics[s].index(occupied)];
    case QUEEN :
        return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
    default :
        return PseudoAttacks[Pt][s];
    }
}

inline Bitboard attacks_bb(PieceType pt, Square s, Bitboard occupied) {
    assert(pt != PAWN && is_ok(s));
    switch (pt) {
    case BISHOP :
        return attacks_bb<BISHOP>(s, occupied);
    case ROOK :
        return attacks_bb<ROOK>(s, occupied);
    case QUEEN :
        return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
    default :
        return PseudoAttacks[pt][s];
    }
}

inline int popcount(Bitboard b) {
    return std::popcount(b);
}

inline Square lsb(Bitboard b) {
    assert(b);
    return Square(std::countr_zero(b));
}

inline Square msb(Bitboard b) {
    assert(b);
    return Square(63 ^ std::countl_zero(b));
}

inline Bitboard least_significant_square_bb(Bitboard b) {
    assert(b);
    return b & -b;
}

inline Square pop_lsb(Bitboard& b) {
    assert(b);
    const Square s = lsb(b);
    b &= b - 1;
    return s;
}

}  // namespace sp
