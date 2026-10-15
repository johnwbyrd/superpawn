// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "movegen.h"

#include "bitboard.h"
#include "position.h"

namespace sp {

namespace {

template <GenType Type, Direction D, bool Enemy>
ExtMove* make_promotions(ExtMove* moveList, Square to) {
    constexpr bool all = Type == EVASIONS || Type == NON_EVASIONS;

    if constexpr (Type == CAPTURES || all)
        *moveList++ = Move::make<PROMOTION>(to - D, to, QUEEN);

    // Capturing underpromotions go with the captures, quiet ones with the quiets.
    if constexpr ((Type == CAPTURES && Enemy) || (Type == QUIETS && !Enemy) || all) {
        *moveList++ = Move::make<PROMOTION>(to - D, to, ROOK);
        *moveList++ = Move::make<PROMOTION>(to - D, to, BISHOP);
        *moveList++ = Move::make<PROMOTION>(to - D, to, KNIGHT);
    }

    return moveList;
}

template <Color Us, GenType Type>
ExtMove* generate_pawn_moves(const Position& pos, ExtMove* moveList, Bitboard target) {
    constexpr Color     Them     = ~Us;
    constexpr Bitboard  TRank7BB = Us == WHITE ? Rank7BB : Rank2BB;
    constexpr Bitboard  TRank3BB = Us == WHITE ? Rank3BB : Rank6BB;
    constexpr Direction Up       = pawn_push(Us);
    constexpr Direction UpRight  = Us == WHITE ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction UpLeft   = Us == WHITE ? NORTH_WEST : SOUTH_EAST;

    const Bitboard emptySquares = ~pos.pieces();
    const Bitboard enemies      = Type == EVASIONS ? pos.checkers() : pos.pieces(Them);

    const Bitboard pawnsOn7    = pos.pieces(Us, PAWN) & TRank7BB;
    const Bitboard pawnsNotOn7 = pos.pieces(Us, PAWN) & ~TRank7BB;

    // Single and double pushes, no promotions
    if constexpr (Type != CAPTURES) {
        Bitboard b1 = shift<Up>(pawnsNotOn7) & emptySquares;
        Bitboard b2 = shift<Up>(b1 & TRank3BB) & emptySquares;

        if constexpr (Type == EVASIONS) {
            // Only pushes that block the check
            b1 &= target;
            b2 &= target;
        }

        while (b1) {
            const Square to = pop_lsb(b1);
            *moveList++     = Move(to - Up, to);
        }

        while (b2) {
            const Square to = pop_lsb(b2);
            *moveList++     = Move(to - Up - Up, to);
        }
    }

    // Promotions
    if (pawnsOn7) {
        Bitboard b1 = shift<UpRight>(pawnsOn7) & enemies;
        Bitboard b2 = shift<UpLeft>(pawnsOn7) & enemies;
        Bitboard b3 = shift<Up>(pawnsOn7) & emptySquares;

        if constexpr (Type == EVASIONS)
            b3 &= target;

        while (b1)
            moveList = make_promotions<Type, UpRight, true>(moveList, pop_lsb(b1));

        while (b2)
            moveList = make_promotions<Type, UpLeft, true>(moveList, pop_lsb(b2));

        while (b3)
            moveList = make_promotions<Type, Up, false>(moveList, pop_lsb(b3));
    }

    // Standard and en passant captures
    if constexpr (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS) {
        Bitboard b1 = shift<UpRight>(pawnsNotOn7) & enemies;
        Bitboard b2 = shift<UpLeft>(pawnsNotOn7) & enemies;

        while (b1) {
            const Square to = pop_lsb(b1);
            *moveList++     = Move(to - UpRight, to);
        }

        while (b2) {
            const Square to = pop_lsb(b2);
            *moveList++     = Move(to - UpLeft, to);
        }

        if (pos.ep_square() != SQ_NONE) {
            assert(rank_of(pos.ep_square()) == relative_rank(Us, RANK_6));

            // An en passant capture cannot resolve a discovered check: if the
            // square the double-pushed pawn vacated lies on the check line, the
            // checker is a slider behind it.
            if (Type == EVASIONS && (target & (pos.ep_square() + Up)))
                return moveList;

            b1 = pawnsNotOn7 & pawn_attacks_bb(Them, pos.ep_square());
            assert(b1);

            while (b1)
                *moveList++ = Move::make<EN_PASSANT>(pop_lsb(b1), pos.ep_square());
        }
    }

    return moveList;
}

template <Color Us, PieceType Pt>
ExtMove* generate_moves(const Position& pos, ExtMove* moveList, Bitboard target) {
    static_assert(Pt != KING && Pt != PAWN, "Unsupported piece type in generate_moves()");

    Bitboard bb = pos.pieces(Us, Pt);

    while (bb) {
        const Square from = pop_lsb(bb);
        Bitboard     b    = attacks_bb<Pt>(from, pos.pieces()) & target;

        while (b)
            *moveList++ = Move(from, pop_lsb(b));
    }

    return moveList;
}

template <Color Us, GenType Type>
ExtMove* generate_all(const Position& pos, ExtMove* moveList) {
    static_assert(Type != LEGAL, "Unsupported type in generate_all()");

    const Square ksq = pos.square<KING>(Us);
    Bitboard     target;

    // In double check only the king can move.
    if (Type != EVASIONS || !more_than_one(pos.checkers())) {
        target = Type == EVASIONS     ? between_bb(ksq, lsb(pos.checkers()))
               : Type == NON_EVASIONS ? ~pos.pieces(Us)
               : Type == CAPTURES     ? pos.pieces(~Us)
                                      : ~pos.pieces();  // QUIETS

        moveList = generate_pawn_moves<Us, Type>(pos, moveList, target);
        moveList = generate_moves<Us, KNIGHT>(pos, moveList, target);
        moveList = generate_moves<Us, BISHOP>(pos, moveList, target);
        moveList = generate_moves<Us, ROOK>(pos, moveList, target);
        moveList = generate_moves<Us, QUEEN>(pos, moveList, target);
    }

    Bitboard b = attacks_bb<KING>(ksq) & (Type == EVASIONS ? ~pos.pieces(Us) : target);

    while (b)
        *moveList++ = Move(ksq, pop_lsb(b));

    if ((Type == QUIETS || Type == NON_EVASIONS) && pos.can_castle(Us & ANY_CASTLING))
        for (CastlingRights cr : {Us & KING_SIDE, Us & QUEEN_SIDE})
            if (!pos.castling_impeded(cr) && pos.can_castle(cr))
                *moveList++ = Move::make<CASTLING>(ksq, pos.castling_king_to(cr));

    return moveList;
}

}  // namespace

// Generates pseudo-legal moves of the requested kind. CAPTURES and QUIETS
// partition the non-evasion moves; EVASIONS requires the side to move to be
// in check and the others require it not to be.
template <GenType Type>
ExtMove* generate(const Position& pos, ExtMove* moveList) {
    static_assert(Type != LEGAL, "Unsupported type in generate()");
    assert((Type == EVASIONS) == bool(pos.checkers()));

    const Color us = pos.side_to_move();

    return us == WHITE ? generate_all<WHITE, Type>(pos, moveList) : generate_all<BLACK, Type>(pos, moveList);
}

template ExtMove* generate<CAPTURES>(const Position&, ExtMove*);
template ExtMove* generate<QUIETS>(const Position&, ExtMove*);
template ExtMove* generate<EVASIONS>(const Position&, ExtMove*);
template ExtMove* generate<NON_EVASIONS>(const Position&, ExtMove*);

// Generates all legal moves. Pseudo-legal moves are filtered with
// Position::legal() only where a move could be illegal.
template <>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList) {
    const Color    us     = pos.side_to_move();
    const Bitboard pinned = pos.blockers_for_king(us) & pos.pieces(us);
    const Square   ksq    = pos.square<KING>(us);
    ExtMove*       cur    = moveList;

    moveList = pos.checkers() ? generate<EVASIONS>(pos, moveList) : generate<NON_EVASIONS>(pos, moveList);

    while (cur != moveList)
        if (((pinned & cur->from()) || cur->from() == ksq || cur->type() == EN_PASSANT) && !pos.legal(*cur))
            *cur = *(--moveList);
        else
            ++cur;

    return moveList;
}

}  // namespace sp
