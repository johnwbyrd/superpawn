// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "movegen.h"
#include "position.h"

using namespace sp;

TEST_CASE("FEN round trip") {
    const std::vector<std::string> fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "4k3/8/8/8/8/8/8/4K2R w K - 0 1",
        "8/8/8/8/8/8/6k1/4K2R w K - 0 1",
    };

    for (const auto& fen : fens) {
        Position pos;
        pos.set(fen);
        CHECK(pos.fen() == fen);
        CHECK(pos.pos_is_ok());
    }
}

TEST_CASE("en passant square is only kept when capturable") {
    Position pos;
    // e3 is the ep square, but no black pawn can capture there
    pos.set("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    CHECK(pos.ep_square() == SQ_NONE);
    CHECK(pos.fen() == "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");

    pos.set("rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 2");
    CHECK(pos.ep_square() == SQ_E3);
}

TEST_CASE("make and unmake restore the position and keys") {
    Position pos;
    pos.set("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    const std::string fen = pos.fen();
    const Key         key = pos.key();
    const Key         pk  = pos.pawn_key();

    for (const auto& m : MoveList<LEGAL>(pos)) {
        pos.do_move(m);

        // The incremental key must match the key computed from scratch
        Position fresh;
        fresh.set(pos.fen());
        CHECK(fresh.key() == pos.key());
        CHECK(fresh.pawn_key() == pos.pawn_key());
        CHECK(fresh.checkers() == pos.checkers());

        pos.undo_move(m);
        CHECK(pos.fen() == fen);
        CHECK(pos.key() == key);
        CHECK(pos.pawn_key() == pk);
    }
}

TEST_CASE("castling moves") {
    Position pos;
    pos.set("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    const Move oo = pos.uci_to_move("e1g1");
    REQUIRE(oo != Move::none());
    CHECK(oo.type() == CASTLING);

    pos.do_move(oo);
    CHECK(pos.piece_on(SQ_G1) == W_KING);
    CHECK(pos.piece_on(SQ_F1) == W_ROOK);
    CHECK(pos.empty(SQ_E1));
    CHECK(pos.empty(SQ_H1));
    CHECK_FALSE(pos.can_castle(WHITE_OO));
    CHECK_FALSE(pos.can_castle(WHITE_OOO));
    CHECK(pos.can_castle(BLACK_OO));
    pos.undo_move(oo);
    CHECK(pos.fen() == "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    // Castling through an attacked square is illegal: the rook on d2 covers
    // d1, so only the king side is available.
    pos.set("4k3/8/8/8/8/8/3r4/R3K2R w KQ - 0 1");
    CHECK(pos.uci_to_move("e1g1") != Move::none());
    CHECK(pos.uci_to_move("e1c1") == Move::none());

    // Castling out of check is illegal
    pos.set("4k3/8/8/8/8/8/4r3/R3K2R w KQ - 0 1");
    CHECK(pos.uci_to_move("e1g1") == Move::none());
    CHECK(pos.uci_to_move("e1c1") == Move::none());
}

TEST_CASE("gives_check agrees with checkers after the move") {
    const std::vector<std::string> fens = {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "4k3/8/8/3pP3/8/8/8/4K2R w K d6 0 1",
    };

    for (const auto& fen : fens) {
        Position pos;
        pos.set(fen);
        for (const auto& m : MoveList<LEGAL>(pos)) {
            const bool gc = pos.gives_check(m);
            pos.do_move(m);
            CHECK(gc == bool(pos.checkers()));
            pos.undo_move(m);
        }
    }
}

TEST_CASE("pseudo_legal accepts generated moves and rejects garbage") {
    Position pos;
    pos.set("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    for (const auto& m : MoveList<LEGAL>(pos))
        CHECK(pos.pseudo_legal(m));

    CHECK_FALSE(pos.pseudo_legal(Move(SQ_A1, SQ_A8)));  // rook blocked
    CHECK_FALSE(pos.pseudo_legal(Move(SQ_E2, SQ_E4)));  // bishop moving like a pawn
    CHECK_FALSE(pos.pseudo_legal(Move(SQ_A7, SQ_A6)));  // enemy piece
    CHECK(pos.pseudo_legal(Move(SQ_A2, SQ_A4)));        // double push with both squares free
    CHECK_FALSE(pos.pseudo_legal(Move(SQ_B2, SQ_B4)));  // double push blocked on b4
}

TEST_CASE("repetition and fifty-move draws") {
    Position pos;
    pos.set("4k3/8/8/8/8/8/8/4K2R w - - 0 1");

    // Shuffle the rook back and forth; the position repeats after four plies
    for (const char* uci : {"h1h2", "e8d8", "h2h1", "d8e8"})
        pos.do_move(pos.uci_to_move(uci));

    // A single repetition counts as a draw only when it lies inside the
    // search tree (the repetition distance is less than the ply).
    CHECK(pos.has_repeated());
    CHECK(pos.is_draw(5));
    CHECK_FALSE(pos.is_draw(3));

    // A second repetition is a draw regardless of ply
    for (const char* uci : {"h1h2", "e8d8", "h2h1", "d8e8"})
        pos.do_move(pos.uci_to_move(uci));
    CHECK(pos.is_draw(0));

    pos.set("4k3/8/8/8/8/8/8/4K2R w - - 100 1");
    CHECK(pos.is_draw(0));
    pos.set("4k3/8/8/8/8/8/8/4K2R w - - 99 1");
    CHECK_FALSE(pos.is_draw(0));
}

TEST_CASE("static exchange evaluation") {
    Position pos;

    // Pawn takes a defended knight: wins a knight, loses a pawn
    pos.set("4k3/8/3p4/4n3/3P4/8/8/4K3 w - - 0 1");
    CHECK(pos.see_ge(pos.uci_to_move("d4e5"), KnightValue - PawnValue));
    CHECK_FALSE(pos.see_ge(pos.uci_to_move("d4e5"), KnightValue - PawnValue + 1));

    // Rook takes a pawn defended by a pawn: wins a pawn, loses the rook
    pos.set("4k3/8/5p2/4p3/8/8/8/4RK2 w - - 0 1");
    REQUIRE(pos.uci_to_move("e1e5") != Move::none());
    CHECK_FALSE(pos.see_ge(pos.uci_to_move("e1e5"), 0));
    CHECK(pos.see_ge(pos.uci_to_move("e1e5"), PawnValue - RookValue));

    // The same capture with the defender pinned to its king (rook f1) is safe
    pos.set("5k2/8/5p2/4p3/8/8/8/4RRK1 w - - 0 1");
    CHECK(pos.see_ge(pos.uci_to_move("e1e5"), PawnValue));

    // An undefended piece is simply won
    pos.set("4k3/8/8/4n3/8/8/8/4RK2 w - - 0 1");
    CHECK(pos.see_ge(pos.uci_to_move("e1e5"), KnightValue));
    CHECK_FALSE(pos.see_ge(pos.uci_to_move("e1e5"), KnightValue + 1));
}

TEST_CASE("uci_to_move") {
    Position pos;
    pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    CHECK(pos.uci_to_move("e2e4") == Move(SQ_E2, SQ_E4));
    CHECK(pos.uci_to_move("e2e5") == Move::none());
    CHECK(pos.uci_to_move("0000") == Move::none());

    pos.set("4k3/1P6/8/8/8/8/8/4K3 w - - 0 1");
    CHECK(pos.uci_to_move("b7b8q") == Move::make<PROMOTION>(SQ_B7, SQ_B8, QUEEN));
    CHECK(pos.uci_to_move("b7b8N") == Move::make<PROMOTION>(SQ_B7, SQ_B8, KNIGHT));
}
