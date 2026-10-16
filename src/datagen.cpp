// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "datagen.h"

#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>

#include "bitboard.h"
#include "eval.h"
#include "misc.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"

namespace sp::datagen {

namespace {

constexpr const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::atomic<bool> stopRequested{false};

void on_interrupt(int) {
    stopRequested = true;
}

struct Rng {
    std::uint64_t s;
    std::uint64_t next() {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }
    int below(int n) { return int(next() % std::uint64_t(n)); }
};

// Counters and the output file, shared by all generator threads.
struct Shared {
    std::ofstream              file;
    std::mutex                 mutex;
    std::atomic<std::uint64_t> positions{0}, games{0};
    std::atomic<std::uint64_t> results[3]{};  // white wins, draws, black wins
};

// Draw by insufficient material: no pawns or major pieces and at most one
// minor piece on the board. (Two knights cannot force mate either.)
bool insufficient_material(const Position& pos) {
    if (pos.pieces(PAWN) || pos.pieces(ROOK) || pos.pieces(QUEEN))
        return false;
    return popcount(pos.pieces(KNIGHT, BISHOP)) <= 1
        || (pos.pieces(KNIGHT) == pos.pieces(KNIGHT, BISHOP) && popcount(pos.pieces(KNIGHT)) == 2);
}

// Searches the position with the thread's own single-threaded pool and
// returns the best move and its score from the side to move's view.
struct SearchResult {
    Move  move;
    Value score;
};

SearchResult search(ThreadPool& pool, const Position& pos, std::int64_t nodes) {
    Limits limits;
    limits.nodes     = nodes;
    limits.startTime = now();

    pool.start_thinking(pos, limits);
    pool.wait_for_search_finished();

    const RootMove& rm = pool.main_worker()->rootMoves[0];
    return {rm.pv[0], rm.score};
}

// Plays one game and appends its positions to the shared file. Returns the
// number of positions written, or 0 if the game was discarded.
std::uint64_t play_game(
    const Options& opt, Shared& shared, ThreadPool& pool, TranspositionTable& tt, Rng& rng, std::uint64_t gameIndex) {
    Position pos;
    pos.set(StartFEN);
    tt.clear(1);
    pool.clear();

    // Random opening
    const int plies = opt.randomPlies + int(gameIndex & 1);
    for (int i = 0; i < plies; ++i) {
        MoveList<LEGAL> moves(pos);
        if (!moves.size())
            return 0;
        pos.do_move(*(moves.begin() + rng.below(int(moves.size()))));
    }

    // Discard openings that are already decided
    if (std::abs(search(pool, pos, opt.nodes).score) > opt.openingLimit)
        return 0;

    std::vector<BulletBoard> game;
    game.reserve(256);

    int whiteResult2  = 1;  // 0 black wins, 1 draw, 2 white wins
    int decisivePlies = 0;

    for (int ply = 0;; ++ply) {
        MoveList<LEGAL> legal(pos);
        if (!legal.size()) {
            whiteResult2 = pos.checkers() ? (pos.side_to_move() == WHITE ? 0 : 2) : 1;
            break;
        }
        if (pos.is_draw(MAX_PLY) || insufficient_material(pos) || ply >= opt.maxPly) {
            whiteResult2 = 1;
            break;
        }

        const SearchResult sr = search(pool, pos, opt.nodes);
        if (sr.move == Move::none() || stopRequested)
            return 0;

        const Color stm = pos.side_to_move();

        // A forced mate or a long-standing decisive score ends the game early
        if (std::abs(sr.score) >= VALUE_TB_WIN_IN_MAX_PLY) {
            whiteResult2 = (sr.score > 0) == (stm == WHITE) ? 2 : 0;
            break;
        }
        decisivePlies = std::abs(sr.score) >= opt.adjudicateWin ? decisivePlies + 1 : 0;
        if (decisivePlies >= 4) {
            whiteResult2 = (sr.score > 0) == (stm == WHITE) ? 2 : 0;
            break;
        }

        // Record quiet positions only: a capture or a check makes the static
        // evaluation a poor fit for the search score.
        if (!pos.checkers() && !pos.capture(sr.move) && sr.move.type() != PROMOTION && std::abs(sr.score) < 10000)
            game.push_back(pack(pos, sr.score));

        pos.do_move(sr.move);
    }

    for (BulletBoard& b : game)
        set_result(b, Color(b.extra[0]), whiteResult2);

    {
        std::lock_guard<std::mutex> lock(shared.mutex);
        shared.file.write(reinterpret_cast<const char*>(game.data()),
                          std::streamsize(game.size() * sizeof(BulletBoard)));
        shared.results[2 - whiteResult2]++;
    }
    return game.size();
}

void generator_thread(int index, const Options& opt, Shared& shared) {
    TranspositionTable tt;
    SearchOptions      searchOptions;
    searchOptions.quiet = true;
    ThreadPool pool(tt, searchOptions);
    pool.set(1);
    tt.resize(std::size_t(opt.hashMb), 1);

    Rng rng{opt.seed + 0x9E3779B97F4A7C15ULL * std::uint64_t(index + 1)};

    while (!stopRequested) {
        if (shared.positions.load() >= opt.positions || (opt.games && shared.games.load() >= opt.games))
            break;
        const std::uint64_t written = play_game(opt, shared, pool, tt, rng, shared.games.fetch_add(1));
        shared.positions += written;
    }
}

}  // namespace

// The board as bullet stores it: from the side to move's view, so for Black
// the ranks are flipped and the colours swapped. Pieces are packed as 4-bit
// nibbles in the order of the occupancy bits: bit 3 set for the opponent's
// pieces, bits 0-2 the type with pawn = 0.
BulletBoard pack(const Position& pos, Value scoreStm) {
    const Color stm = pos.side_to_move();
    BulletBoard b{};

    const Bitboard occ = stm == WHITE ? pos.pieces() : std::byteswap(pos.pieces());
    b.occ              = occ;

    int idx = 0;
    for (Bitboard bb = occ; bb; ++idx) {
        const Square       flipped = pop_lsb(bb);
        const Square       sq      = stm == WHITE ? flipped : flip_rank(flipped);
        const Piece        pc      = pos.piece_on(sq);
        const std::uint8_t nibble  = std::uint8_t((color_of(pc) != stm) << 3 | (type_of(pc) - PAWN));
        b.pcs[idx / 2] |= std::uint8_t(nibble << (4 * (idx & 1)));
    }

    b.score    = std::int16_t(std::clamp<Value>(scoreStm, -32000, 32000));
    b.ksq      = std::uint8_t(relative_square(stm, pos.square<KING>(stm)));
    b.oppKsq   = std::uint8_t(relative_square(stm, pos.square<KING>(~stm)) ^ 56);
    b.extra[0] = std::uint8_t(stm);  // remembered until the result is known, then cleared
    return b;
}

// Rebuilds the FEN of a packed board. The side to move is always "white" in
// the packed layout; castling and en passant are not stored.
std::string unpack_fen(const BulletBoard& b) {
    char board[64] = {};
    int  idx       = 0;
    for (Bitboard occ = b.occ; occ; ++idx) {
        const Square s   = pop_lsb(occ);
        const int    nib = (b.pcs[idx / 2] >> (4 * (idx & 1))) & 15;
        const char   c   = "PNBRQK"[nib & 7];
        board[s]         = nib & 8 ? char(c + ('a' - 'A')) : c;
    }

    std::string fen;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            const char c = board[r * 8 + f];
            if (!c)
                ++empty;
            else {
                if (empty)
                    fen += char('0' + empty), empty = 0;
                fen += c;
            }
        }
        if (empty)
            fen += char('0' + empty);
        if (r)
            fen += '/';
    }
    return fen + " w - - 0 1";
}

void eval_stats(const std::string& file, std::uint64_t maxPositions) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "evalstats: cannot open %s\n", file.c_str());
        return;
    }

    const nnue::Network* net = Eval::network();

    struct Stats {
        double sumAbs = 0, sumSq = 0, sumSig = 0, sumX = 0, sumY = 0, sumXY = 0, sumXX = 0, sumYY = 0;
        void   add(double pred, double target) {
            const double d = pred - target;
            sumAbs += std::abs(d);
            sumSq += d * d;
            const double sp = 1.0 / (1.0 + std::exp(-pred / 400.0)), st = 1.0 / (1.0 + std::exp(-target / 400.0));
            sumSig += (sp - st) * (sp - st);
            sumX += pred, sumY += target, sumXY += pred * target, sumXX += pred * pred, sumYY += target * target;
        }
        void print(const char* name, double n) const {
            const double corr =
                (n * sumXY - sumX * sumY) / std::sqrt((n * sumXX - sumX * sumX) * (n * sumYY - sumY * sumY));
            std::printf("%-14s mean abs error %6.1f cp   rmse %6.1f cp   sigmoid mse %.5f   correlation %.3f\n", name,
                        sumAbs / n, std::sqrt(sumSq / n), sumSig / n, corr);
        }
    } hce, nn;

    Position      pos;
    BulletBoard   b;
    std::uint64_t n = 0, resultsMatch = 0;
    while (n < maxPositions && in.read(reinterpret_cast<char*>(&b), sizeof(b))) {
        pos.set(unpack_fen(b));
        const double target = b.score;
        hce.add(Eval::evaluate_hce(pos), target);
        if (net)
            nn.add(nnue::evaluate(*net, pos), target);
        // sanity: a positive score should mostly go with a win for the side to move
        resultsMatch +=
            (b.score > 0 && b.result == 2) || (b.score < 0 && b.result == 0) || (b.score == 0 && b.result == 1);
        ++n;
    }

    if (!n) {
        std::printf("evalstats: no positions read\n");
        return;
    }

    std::printf("%llu positions from %s; score sign agrees with result in %.1f%%\n", (unsigned long long)n,
                file.c_str(), 100.0 * double(resultsMatch) / double(n));
    hce.print("hand-crafted", double(n));
    if (net)
        nn.print("NNUE", double(n));
    else
        std::printf("NNUE           no network loaded\n");
}

void set_result(BulletBoard& b, Color stm, int whiteResult2) {
    b.result   = std::uint8_t(stm == WHITE ? whiteResult2 : 2 - whiteResult2);
    b.extra[0] = 0;
}

void run(const Options& options) {
    Options opt = options;
    if (opt.seed == 0)
        opt.seed = std::uint64_t(std::chrono::steady_clock::now().time_since_epoch().count());
    opt.threads = std::max(1, opt.threads);

    // Create the output directory if needed, then append to the file
    std::error_code ec;
    const auto      parent = std::filesystem::path(opt.output).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent, ec);

    Shared shared;
    shared.file.open(opt.output, std::ios::binary | std::ios::app);
    if (!shared.file) {
        std::fprintf(stderr, "datagen: cannot open %s for writing%s\n", opt.output.c_str(),
                     ec ? (": " + ec.message()).c_str() : "");
        return;
    }

    std::fprintf(stderr, "datagen: %d thread%s, %lld nodes per move, %d random plies, seed %llu\n", opt.threads,
                 opt.threads == 1 ? "" : "s", (long long)opt.nodes, opt.randomPlies, (unsigned long long)opt.seed);
    std::fprintf(stderr, "datagen: writing to %s until %llu positions%s; Ctrl-C stops cleanly\n", opt.output.c_str(),
                 (unsigned long long)opt.positions,
                 opt.games ? (" or " + std::to_string(opt.games) + " games").c_str() : "");

    stopRequested = false;
    std::signal(SIGINT, on_interrupt);

    std::vector<std::thread> threads;
    for (int i = 0; i < opt.threads; ++i)
        threads.emplace_back(generator_thread, i, std::cref(opt), std::ref(shared));

    const TimePoint start      = now();
    TimePoint       lastReport = start;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        bool anyRunning = false;
        for (auto& th : threads)
            anyRunning |= th.joinable();

        const bool done = stopRequested || shared.positions.load() >= opt.positions
                       || (opt.games && shared.games.load() >= opt.games);

        if (now() - lastReport >= 10000 || done) {
            lastReport                          = now();
            const double                seconds = double(now() - start) / 1000.0;
            const std::uint64_t         pos     = shared.positions.load();
            std::lock_guard<std::mutex> lock(shared.mutex);
            shared.file.flush();
            std::fprintf(stderr, "datagen: %llu positions, %llu games (+%llu =%llu -%llu), %.0f pos/s, %.1f min\n",
                         (unsigned long long)pos, (unsigned long long)shared.games.load(),
                         (unsigned long long)shared.results[0].load(), (unsigned long long)shared.results[1].load(),
                         (unsigned long long)shared.results[2].load(), seconds > 0 ? double(pos) / seconds : 0.0,
                         seconds / 60.0);
        }

        if (done || !anyRunning)
            break;
    }

    stopRequested = true;
    for (auto& th : threads)
        th.join();

    shared.file.flush();
    std::signal(SIGINT, SIG_DFL);
    std::fprintf(stderr, "datagen: done\n");
}

}  // namespace sp::datagen
