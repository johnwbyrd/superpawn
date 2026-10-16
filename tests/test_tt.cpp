// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include <cstdint>
#include <vector>

#include "doctest/doctest.h"
#include "tt.h"

using namespace sp;

namespace {

std::uint64_t splitmix(std::uint64_t& x) {
    std::uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
    z               = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z               = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

}  // namespace

TEST_CASE("transposition table stores and retrieves entries") {
    TranspositionTable tt;
    tt.resize(1, 1);
    tt.new_search();

    std::uint64_t seed = 12345;
    const Key     key  = splitmix(seed);
    bool          found;
    TTEntry*      tte = tt.probe(key, found);
    CHECK_FALSE(found);

    tte->save(key, 123, true, BOUND_EXACT, 7, Move(SQ_E2, SQ_E4), 45, tt.generation());

    tte = tt.probe(key, found);
    REQUIRE(found);
    CHECK(tte->value() == 123);
    CHECK(tte->eval() == 45);
    CHECK(tte->depth() == 7);
    CHECK(tte->bound() == BOUND_EXACT);
    CHECK(tte->is_pv());
    CHECK(tte->move() == Move(SQ_E2, SQ_E4));
}

// A 1 MB table holds about 100k entries. Writing far more than that and then
// probing fresh keys must almost never report a hit: a false hit rate above a
// fraction of a percent means the check key is correlated with the index.
TEST_CASE("transposition table false hit rate is low when full") {
    TranspositionTable tt;
    tt.resize(1, 1);
    tt.new_search();

    std::uint64_t seed = 777;
    for (int i = 0; i < 500000; ++i) {
        const Key key = splitmix(seed);
        bool      found;
        TTEntry*  tte = tt.probe(key, found);
        tte->save(key, Value(i & 0xFFF), false, BOUND_LOWER, 5, Move::none(), 0, tt.generation());
    }

    int falseHits = 0;
    for (int i = 0; i < 100000; ++i) {
        const Key key = splitmix(seed);  // keys never written
        bool      found;
        tt.probe(key, found);
        falseHits += found;
    }

    // Expected rate with an independent 16-bit check key: 3 / 65536, about 5 per 100k
    CHECK(falseHits < 50);
}
