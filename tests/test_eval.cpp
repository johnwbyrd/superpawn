// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "eval.h"
#include "position.h"

using namespace sp;

namespace {

// Swaps the colours in a FEN: pieces change case, ranks are reversed, the side
// to move and castling rights flip. The evaluation must be identical for the
// side to move in both positions.
std::string mirror_fen(const std::string& fen) {
    std::istringstream is(fen);
    std::string        board, stm, castling, ep, rest;
    is >> board >> stm >> castling >> ep;
    std::getline(is, rest);

    // Reverse the rank order and swap piece case
    std::vector<std::string> ranks;
    std::string              rank;
    for (char c : board) {
        if (c == '/') {
            ranks.push_back(rank);
            rank.clear();
        } else
            rank += std::isupper(c) ? char(std::tolower(c)) : char(std::toupper(c));
    }
    ranks.push_back(rank);

    std::string out;
    for (auto it = ranks.rbegin(); it != ranks.rend(); ++it)
        out += *it + (it + 1 != ranks.rend() ? "/" : "");

    std::string newCastling;
    for (char c : castling)
        newCastling += c == '-' ? c : std::isupper(c) ? char(std::tolower(c)) : char(std::toupper(c));

    // Castling rights are written white first; reorder after the case swap
    std::string ordered;
    for (char c : {'K', 'Q', 'k', 'q'})
        if (newCastling.find(c) != std::string::npos)
            ordered += c;
    if (ordered.empty())
        ordered = "-";

    std::string newEp = ep;
    if (ep != "-")
        newEp[1] = char('1' + ('8' - ep[1]));

    return out + " " + (stm == "w" ? "b" : "w") + " " + ordered + " " + newEp + rest;
}

}  // namespace

TEST_CASE("evaluation is colour symmetric") {
    const std::vector<std::string> fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "4k3/8/8/8/8/8/8/4K2R w - - 0 1",
        "rnbqkbnr/pppp2pp/8/4p3/2B1N3/8/PPPP1PPP/R1BQK1NR b KQkq - 0 4",
        "8/8/3P3k/8/1p6/8/1P6/1K3n2 b - - 0 1",
    };

    for (const auto& fen : fens) {
        Position pos, mirrored;
        pos.set(fen);
        mirrored.set(mirror_fen(fen));
        INFO(fen << " vs " << mirror_fen(fen));
        CHECK(Eval::evaluate(pos) == Eval::evaluate(mirrored));
    }
}

TEST_CASE("evaluation basics") {
    Position pos;

    // The starting position is roughly balanced, with a small tempo bonus
    pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const Value start = Eval::evaluate(pos);
    CHECK(start >= 0);
    CHECK(start < 50);

    // An extra rook is worth roughly a rook
    pos.set("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
    CHECK(Eval::evaluate(pos) > 400);
    CHECK(Eval::evaluate(pos) < 600);

    // The same from the other side
    pos.set("4k3/8/8/8/8/8/8/4K2R b - - 0 1");
    CHECK(Eval::evaluate(pos) < -400);
}
