// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "movegen.h"
#include "nnue.h"
#include "position.h"

using namespace sp;

namespace {

struct Rng {
    std::uint64_t s;
    std::uint64_t next() {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }
    int range(int lo, int hi) { return lo + int(next() % std::uint64_t(hi - lo + 1)); }
};

// A network with random weights in the ranges a trained one would have.
std::unique_ptr<nnue::Network> random_network(std::uint64_t seed) {
    auto net = std::make_unique<nnue::Network>();
    Rng  rng{seed};
    for (auto& w : net->featureWeights)
        w = std::int16_t(rng.range(-60, 60));
    for (auto& b : net->featureBias)
        b = std::int16_t(rng.range(-30, 30));
    for (auto& bucket : net->outputWeights)
        for (auto& w : bucket)
            w = std::int16_t(rng.range(-127, 127));
    for (auto& b : net->outputBias)
        b = std::int16_t(rng.range(-2000, 2000));
    net->name = "random";
    return net;
}

// Plays random legal moves, checking after each that the incrementally
// updated accumulator equals a fresh refresh and that the SIMD output equals
// the scalar reference.
void check_random_game(const nnue::Network& net, Position& pos, Rng& rng, int plies) {
    for (int ply = 0; ply < plies; ++ply) {
        MoveList<LEGAL> moves(pos);
        if (!moves.size() || pos.is_draw(0))
            break;

        // Occasionally a null move, which must keep the accumulator intact
        if (!pos.checkers() && rng.range(0, 9) == 0) {
            pos.do_null_move();
            const Value       v = nnue::evaluate(net, pos);
            nnue::Accumulator fresh;
            nnue::refresh(net, pos, fresh);
            const int bucket = nnue::output_bucket(popcount(pos.pieces()));
            CHECK(v == nnue::evaluate_scalar(net, fresh, pos.side_to_move(), bucket));
            pos.undo_null_move();
        }

        const Move m = *(moves.begin() + rng.range(0, int(moves.size()) - 1));
        pos.do_move(m);

        const Value v = nnue::evaluate(net, pos);  // updates incrementally

        nnue::Accumulator fresh;
        nnue::refresh(net, pos, fresh);
        const nnue::Accumulator& inc = pos.accumulators().back();
        REQUIRE(inc.computed);
        for (Color c : {WHITE, BLACK})
            for (int i = 0; i < nnue::HIDDEN; ++i) {
                if (inc.v[c][i] != fresh.v[c][i]) {
                    INFO("move " << move_to_uci(m) << " fen " << pos.fen());
                    REQUIRE(inc.v[c][i] == fresh.v[c][i]);
                }
            }

        const int bucket = nnue::output_bucket(popcount(pos.pieces()));
        CHECK(v == nnue::evaluate_scalar(net, fresh, pos.side_to_move(), bucket));
    }
}

}  // namespace

TEST_CASE("feature indices cover the input space exactly once per perspective") {
    std::vector<int> seen(nnue::INPUTS, 0);
    for (Color c : {WHITE, BLACK}) {
        std::fill(seen.begin(), seen.end(), 0);
        for (Piece pc :
             {W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING, B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING})
            for (Square s = SQ_A1; s <= SQ_H8; ++s) {
                const int idx = nnue::feature_index(c, pc, s);
                REQUIRE(idx >= 0);
                REQUIRE(idx < nnue::INPUTS);
                seen[idx]++;
            }
        for (int n : seen)
            CHECK(n == 1);
    }

    // A white pawn on e2 seen by White is a black pawn on e7 seen by Black
    CHECK(nnue::feature_index(WHITE, W_PAWN, SQ_E2) == nnue::feature_index(BLACK, B_PAWN, SQ_E7));
    CHECK(nnue::feature_index(WHITE, B_KING, SQ_E8) == nnue::feature_index(BLACK, W_KING, SQ_E1));
}

TEST_CASE("output buckets span the piece counts") {
    CHECK(nnue::output_bucket(2) == 0);
    CHECK(nnue::output_bucket(32) == nnue::OUTPUT_BUCKETS - 1);
    for (int n = 2; n <= 32; ++n) {
        CHECK(nnue::output_bucket(n) >= 0);
        CHECK(nnue::output_bucket(n) < nnue::OUTPUT_BUCKETS);
    }
}

TEST_CASE("network file round trip") {
    auto              net  = random_network(42);
    const std::string path = "test_network.nnue";
    REQUIRE(net->save(path));

    std::string error;
    auto        loaded = nnue::Network::load(path, error);
    REQUIRE_MESSAGE(loaded, error);
    CHECK(
        std::equal(std::begin(net->featureWeights), std::end(net->featureWeights), std::begin(loaded->featureWeights)));
    CHECK(std::equal(std::begin(net->featureBias), std::end(net->featureBias), std::begin(loaded->featureBias)));
    CHECK(std::equal(&net->outputWeights[0][0], &net->outputWeights[0][0] + nnue::OUTPUT_BUCKETS * 2 * nnue::HIDDEN,
                     &loaded->outputWeights[0][0]));
    CHECK(std::equal(std::begin(net->outputBias), std::end(net->outputBias), std::begin(loaded->outputBias)));

    // A corrupted file is rejected by the checksum
    {
        std::FILE* f = std::fopen(path.c_str(), "r+b");
        REQUIRE(f);
        std::fseek(f, long(nnue::HEADER_SIZE + 1000), SEEK_SET);
        std::fputc(0x55, f);
        std::fclose(f);
    }
    error.clear();
    CHECK(nnue::Network::load(path, error) == nullptr);
    CHECK(error.find("checksum") != std::string::npos);

    std::remove(path.c_str());
}

TEST_CASE("refresh matches a hand computation") {
    auto     net = random_network(7);
    Position pos;
    pos.set("4k3/8/8/8/8/8/8/4K2R w K - 0 1");

    nnue::Accumulator acc;
    nnue::refresh(*net, pos, acc);

    for (Color c : {WHITE, BLACK})
        for (int i = 0; i < nnue::HIDDEN; ++i) {
            int expected = net->featureBias[i];
            for (Square s : {SQ_E8, SQ_E1, SQ_H1})
                expected += net->featureWeights[nnue::feature_index(c, pos.piece_on(s), s) * nnue::HIDDEN + i];
            REQUIRE(acc.v[c][i] == expected);
        }
}

TEST_CASE("incremental updates match refresh and SIMD matches scalar") {
    auto net = random_network(99);
    Rng  rng{2024};

    const std::vector<std::string> starts = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",  // promotions
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                         // en passant
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",                              // castling
    };

    for (const auto& fen : starts)
        for (int game = 0; game < 6; ++game) {
            Position pos;
            pos.set(fen);
            check_random_game(*net, pos, rng, 80);
        }
}

TEST_CASE("evaluation is symmetric between the two perspectives") {
    auto net = random_network(5);

    // The same position with colours swapped and ranks flipped must evaluate
    // identically for the side to move.
    Position a, b;
    a.set("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    b.set("r3k2r/pppbbppp/2n2q1P/1P2p3/3pn3/BN2PNP1/P1PPQPB1/R3K2R b KQkq - 0 1");
    CHECK(nnue::evaluate(*net, a) == nnue::evaluate(*net, b));

    a.set("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
    b.set("4k2r/8/8/8/8/8/8/4K3 b - - 0 1");
    CHECK(nnue::evaluate(*net, a) == nnue::evaluate(*net, b));
}

TEST_CASE("accumulators survive undo and are recomputed after set") {
    auto     net = random_network(11);
    Position pos;
    pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    const Value start = nnue::evaluate(*net, pos);
    const Move  m     = pos.uci_to_move("e2e4");
    pos.do_move(m);
    nnue::evaluate(*net, pos);
    pos.undo_move(m);
    CHECK(nnue::evaluate(*net, pos) == start);
    CHECK(pos.accumulators().size() == 1);

    pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    CHECK_FALSE(pos.accumulators().back().computed);
    CHECK(nnue::evaluate(*net, pos) == start);
}

TEST_CASE("bullet export converts to the engine format") {
    auto              net  = random_network(314);
    const std::string nnue = "test_bullet_roundtrip.nnue";
    const std::string bin  = "test_bullet_quantised.bin";
    REQUIRE(net->save(nnue));

    // A bullet quantised.bin is the weights section of our file, padded to 64 bytes
    {
        std::FILE* in = std::fopen(nnue.c_str(), "rb");
        REQUIRE(in);
        std::vector<unsigned char> bytes(nnue::FILE_SIZE);
        REQUIRE(std::fread(bytes.data(), 1, bytes.size(), in) == bytes.size());
        std::fclose(in);

        std::FILE* out = std::fopen(bin.c_str(), "wb");
        REQUIRE(out);
        std::fwrite(bytes.data() + nnue::HEADER_SIZE, 1, nnue::WEIGHTS_SIZE, out);
        const std::size_t padded = (nnue::WEIGHTS_SIZE + 63) / 64 * 64;
        for (std::size_t i = nnue::WEIGHTS_SIZE; i < padded; ++i)
            std::fputc("bullet"[i % 6], out);
        std::fclose(out);
    }

    std::string error;
    auto        converted = nnue::Network::from_bullet(bin, error);
    REQUIRE_MESSAGE(converted, error);
    CHECK(std::equal(std::begin(net->featureWeights), std::end(net->featureWeights),
                     std::begin(converted->featureWeights)));
    CHECK(std::equal(std::begin(net->featureBias), std::end(net->featureBias), std::begin(converted->featureBias)));
    CHECK(std::equal(&net->outputWeights[0][0], &net->outputWeights[0][0] + nnue::OUTPUT_BUCKETS * 2 * nnue::HIDDEN,
                     &converted->outputWeights[0][0]));
    CHECK(std::equal(std::begin(net->outputBias), std::end(net->outputBias), std::begin(converted->outputBias)));

    std::remove(nnue.c_str());
    std::remove(bin.c_str());
}
