// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include <string>

#include "doctest/doctest.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"

using namespace sp;

namespace {

struct Result {
    std::string move;
    Value       score;
};

// Searches a position to a fixed depth and returns the best move and score.
Result search(const std::string& fen, int depth, int threadCount = 1) {
    TranspositionTable tt;
    SearchOptions      options;
    ThreadPool         threads(tt, options);

    threads.set(threadCount);
    tt.resize(16, threadCount);

    Position pos;
    pos.set(fen);

    Limits limits;
    limits.depth     = depth;
    limits.startTime = now();

    threads.start_thinking(pos, limits);
    threads.wait_for_search_finished();

    const RootMove& rm = threads.best_thread()->worker->rootMoves[0];
    return {move_to_uci(rm.pv[0]), rm.score};
}

std::string best_move(const std::string& fen, int depth, int threadCount = 1) {
    return search(fen, depth, threadCount).move;
}

}  // namespace

TEST_CASE("mate in one") {
    Result r = search("6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", 4);
    CHECK(r.move == "d1d8");
    CHECK(r.score == mate_in(1));

    r = search("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4", 4);
    CHECK(r.move == "h5f7");
    CHECK(r.score == mate_in(1));
}

// Shallow searches prune most quiet moves, so even short mates need a few
// iterations before the mating line is tried. These depths leave a margin.
TEST_CASE("mate in two") {
    // Ladder mate with two rooks; several first moves mate in two
    const Result r = search("7k/8/8/8/8/8/R7/1R5K w - - 0 1", 14);
    CHECK(r.score == mate_in(3));
}

TEST_CASE("mated side sees the mate coming") {
    // Black to move, mated next move whatever it does
    const Result r = search("7k/R7/1R6/8/8/8/8/7K b - - 0 1", 12);
    CHECK(r.score == mated_in(2));
}

TEST_CASE("free piece") {
    // A hanging queen must be taken
    CHECK(best_move("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1", 3) == "d1d5");
}

TEST_CASE("multithreaded search returns a legal move") {
    const std::string bm = best_move("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 8, 4);
    Position          pos;
    pos.set("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    CHECK(pos.uci_to_move(bm) != Move::none());
}

TEST_CASE("stalemate and checkmate at the root") {
    // No legal moves: the search must still answer
    TranspositionTable tt;
    SearchOptions      options;
    ThreadPool         threads(tt, options);
    threads.set(1);
    tt.resize(1, 1);

    Position pos;
    pos.set("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");  // stalemate
    Limits limits;
    limits.depth     = 3;
    limits.startTime = now();
    threads.start_thinking(pos, limits);
    threads.wait_for_search_finished();
    CHECK(threads.main_worker()->rootMoves[0].pv[0] == Move::none());
}
