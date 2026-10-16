// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "bitboard.h"
#include "datagen.h"
#include "doctest/doctest.h"
#include "position.h"

using namespace sp;

namespace {

// Unpacks the nibble of the piece on square `sq` of a packed board.
int nibble_at(const datagen::BulletBoard& b, int sq) {
    int idx = 0;
    for (Bitboard occ = b.occ; occ;) {
        const int s = pop_lsb(occ);
        if (s == sq)
            return (b.pcs[idx / 2] >> (4 * (idx & 1))) & 15;
        ++idx;
    }
    return -1;
}

}  // namespace

TEST_CASE("bulletformat packing, white to move") {
    Position pos;
    pos.set("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
    datagen::BulletBoard b = datagen::pack(pos, 123);

    CHECK(b.occ == (square_bb(SQ_E1) | square_bb(SQ_H1) | square_bb(SQ_E8)));
    CHECK(b.score == 123);
    CHECK(b.ksq == SQ_E1);
    CHECK(b.oppKsq == (SQ_E8 ^ 56));
    CHECK(nibble_at(b, SQ_E1) == 5);        // own king: colour bit clear, type 5
    CHECK(nibble_at(b, SQ_H1) == 3);        // own rook
    CHECK(nibble_at(b, SQ_E8) == (8 | 5));  // their king

    datagen::set_result(b, WHITE, 2);
    CHECK(b.result == 2);
    CHECK(b.extra[0] == 0);
}

TEST_CASE("bulletformat packing flips the board for Black") {
    Position pos;
    pos.set("4k2r/8/8/8/8/8/8/4K3 b k - 0 1");
    datagen::BulletBoard b = datagen::pack(pos, -50);

    // Black's king on e8 becomes the side to move's king on e1; White's king
    // on e1 becomes the opponent's king on e8.
    CHECK(b.occ == (square_bb(SQ_E1) | square_bb(SQ_H1) | square_bb(SQ_E8)));
    CHECK(b.ksq == SQ_E1);
    CHECK(b.oppKsq == (SQ_E8 ^ 56));
    CHECK(nibble_at(b, SQ_E1) == 5);
    CHECK(nibble_at(b, SQ_H1) == 3);
    CHECK(nibble_at(b, SQ_E8) == (8 | 5));
    CHECK(b.score == -50);

    // White won: from Black's view that is a loss
    datagen::set_result(b, BLACK, 2);
    CHECK(b.result == 0);
}

TEST_CASE("bulletformat packing of a full position") {
    Position pos;
    pos.set("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    datagen::BulletBoard b = datagen::pack(pos, 0);

    CHECK(b.occ == pos.pieces());
    CHECK(popcount(b.occ) == 32);
    int own = 0, theirs = 0;
    for (Square s = SQ_A1; s <= SQ_H8; ++s)
        if (!pos.empty(s)) {
            const int nib = nibble_at(b, s);
            CHECK((nib & 7) == type_of(pos.piece_on(s)) - PAWN);
            (nib & 8 ? theirs : own)++;
        }
    CHECK(own == 16);
    CHECK(theirs == 16);
}
