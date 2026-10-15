// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "bitboard.h"
#include "doctest/doctest.h"

using namespace sp;

TEST_CASE("square helpers") {
    CHECK(make_square(FILE_E, RANK_4) == SQ_E4);
    CHECK(file_of(SQ_H8) == FILE_H);
    CHECK(rank_of(SQ_H8) == RANK_8);
    CHECK(relative_square(BLACK, SQ_E1) == SQ_E8);
    CHECK(flip_rank(SQ_A1) == SQ_A8);
    CHECK(square_to_string(SQ_C7) == "c7");
}

TEST_CASE("move encoding") {
    const Move m = Move::make<PROMOTION>(SQ_E7, SQ_E8, QUEEN);
    CHECK(m.from() == SQ_E7);
    CHECK(m.to() == SQ_E8);
    CHECK(m.type() == PROMOTION);
    CHECK(m.promotion_type() == QUEEN);
    CHECK(move_to_uci(m) == "e7e8q");

    CHECK(move_to_uci(Move(SQ_E2, SQ_E4)) == "e2e4");
    CHECK(move_to_uci(Move::none()) == "(none)");
    CHECK_FALSE(Move::none().is_ok());
    CHECK_FALSE(Move::null().is_ok());
    CHECK(Move(SQ_A1, SQ_B1) != Move::null());
}

TEST_CASE("attack tables") {
    CHECK(popcount(attacks_bb<KNIGHT>(SQ_A1)) == 2);
    CHECK(popcount(attacks_bb<KNIGHT>(SQ_D4)) == 8);
    CHECK(popcount(attacks_bb<KING>(SQ_E1)) == 5);
    CHECK(popcount(attacks_bb<KING>(SQ_E4)) == 8);

    // Sliders on an empty board
    CHECK(popcount(attacks_bb<ROOK>(SQ_A1, 0)) == 14);
    CHECK(popcount(attacks_bb<BISHOP>(SQ_A1, 0)) == 7);
    CHECK(popcount(attacks_bb<BISHOP>(SQ_D4, 0)) == 13);
    CHECK(popcount(attacks_bb<QUEEN>(SQ_D4, 0)) == 27);

    // A blocker stops the ray behind it
    const Bitboard occ = square_bb(SQ_D6);
    CHECK((attacks_bb<ROOK>(SQ_D4, occ) & SQ_D6) != 0);
    CHECK((attacks_bb<ROOK>(SQ_D4, occ) & SQ_D7) == 0);
    CHECK(attacks_bb<ROOK>(SQ_D4, occ) == attacks_bb(ROOK, SQ_D4, occ));

    CHECK(pawn_attacks_bb(WHITE, SQ_E4) == (square_bb(SQ_D5) | square_bb(SQ_F5)));
    CHECK(pawn_attacks_bb(BLACK, SQ_A5) == square_bb(SQ_B4));
}

TEST_CASE("lines and between") {
    CHECK(aligned(SQ_A1, SQ_D4, SQ_H8));
    CHECK_FALSE(aligned(SQ_A1, SQ_D4, SQ_H7));
    CHECK(between_bb(SQ_A1, SQ_D4) == (square_bb(SQ_B2) | square_bb(SQ_C3) | square_bb(SQ_D4)));
    CHECK(between_bb(SQ_A1, SQ_B3) == square_bb(SQ_B3));  // not aligned: just the target
    CHECK(line_bb(SQ_A1, SQ_B3) == 0);
    CHECK(distance(SQ_A1, SQ_H8) == 7);
    CHECK(distance<File>(SQ_A1, SQ_H8) == 7);
}

TEST_CASE("bit scans") {
    Bitboard b = square_bb(SQ_C3) | square_bb(SQ_F6);
    CHECK(lsb(b) == SQ_C3);
    CHECK(msb(b) == SQ_F6);
    CHECK(pop_lsb(b) == SQ_C3);
    CHECK(b == square_bb(SQ_F6));
    CHECK(more_than_one(square_bb(SQ_A1) | square_bb(SQ_A2)));
    CHECK_FALSE(more_than_one(square_bb(SQ_A1)));
    CHECK(shift<NORTH>(Rank1BB) == Rank2BB);
    CHECK(shift<EAST>(FileHBB) == 0);
}
