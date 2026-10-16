// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "uci.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "benchmark.h"
#include "datagen.h"
#include "eval.h"
#include "misc.h"
#include "movegen.h"
#include "perft.h"

namespace sp {

namespace {
constexpr const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
}

UCIEngine::UCIEngine(int argc, char** argv) : threads(tt, searchOptions) {

    for (int i = 1; i < argc; ++i)
        cli.emplace_back(argv[i]);

    // Threads must precede Hash: clearing the table uses all threads.
    options.add("Threads", Option(1, 1, 1024, [this](const Option& o) { threads.set(int(o)); }));
    options.add("Hash", Option(16, 1, 33554432,
                               [this](const Option& o) { tt.resize(std::size_t(int(o)), searchOptions.threads); }));
    options.add("Clear Hash", Option([this](const Option&) { tt.clear(searchOptions.threads); }));
    options.add("MultiPV", Option(1, 1, 256, [this](const Option& o) { searchOptions.multiPV = int(o); }));
    options.add("Move Overhead", Option(10, 0, 5000, [this](const Option& o) { searchOptions.moveOverhead = int(o); }));
    options.add("EvalFile", Option("<embedded>", [](const Option& o) {
                    // The embedded network is loaded by Eval::init(); only react to a real path
                    if (std::string(o) != "<embedded>")
                        sync_cout << Eval::set_network(std::string(o)) << sync_endl;
                }));

    pos.set(StartFEN);

    sync_cout << Eval::info() << sync_endl;
}

UCIEngine::~UCIEngine() {
    threads.stop = true;
    threads.wait_for_search_finished();
}

// Reads commands from the command line first, then from standard input until
// `quit` or end of file.
void UCIEngine::loop() {
    bool quit = false;

    if (!cli.empty()) {
        std::string cmd;
        for (const auto& arg : cli)
            cmd += arg + " ";
        run_command(cmd, quit);
        return;
    }

    std::string cmd;
    while (!quit && std::getline(std::cin, cmd))
        run_command(cmd, quit);
}

void UCIEngine::run_command(const std::string& cmd, bool& quit) {
    std::istringstream is(cmd);
    std::string        token;

    is >> std::skipws >> token;

    if (token == "quit" || token == "stop")
        threads.stop = true;

    else if (token == "uci")
        sync_cout << engine_info(true) << "\n" << options << "\nuciok" << sync_endl;

    else if (token == "isready")
        sync_cout << "readyok" << sync_endl;

    else if (token == "setoption")
        setoption(is);

    else if (token == "ucinewgame") {
        threads.wait_for_search_finished();
        tt.clear(searchOptions.threads);
        threads.clear();
    }

    else if (token == "position")
        position(is);

    else if (token == "go")
        go(is);

    else if (token == "ponderhit")
        ;  // pondering is not implemented; the search simply continues

    else if (token == "bench")
        bench(is);

    else if (token == "perft") {
        Depth depth = 1;
        is >> depth;
        wait_and_print_perft(depth);
    }

    else if (token == "d")
        sync_cout << pos.pretty() << sync_endl;

    else if (token == "eval") {
        threads.wait_for_search_finished();
        sync_cout << Eval::trace(pos) << sync_endl;
    }

    else if (token == "compiler")
        sync_cout << compiler_info() << sync_endl;

    else if (token == "datagen")
        datagen_command(is);

    else if (token == "convert") {
        // convert <bullet quantised.bin> <output.nnue>
        std::string in, out, error;
        is >> in >> out;
        if (in.empty() || out.empty())
            sync_cout << "usage: convert <bullet quantised.bin> <output.nnue>" << sync_endl;
        else if (auto net = nnue::Network::from_bullet(in, error); !net)
            sync_cout << "convert failed: " << error << sync_endl;
        else if (!net->save(out))
            sync_cout << "convert failed: cannot write " << out << sync_endl;
        else
            sync_cout << "wrote " << out << " (" << nnue::FILE_SIZE << " bytes)" << sync_endl;
    }

    else if (token == "--help" || token == "help" || token == "--license" || token == "license")
        sync_cout << "\nSuperpawn is a UCI chess engine. Run it from a UCI-compatible GUI, or type UCI\n"
                     "commands at the prompt. Extra commands: bench, perft <depth>, d, eval, compiler,\n"
                     "datagen <file> [options], convert <bullet quantised.bin> <output.nnue>.\n\n"
                     "Superpawn is free software licensed under the GNU Affero General Public\n"
                     "License v3.0 or later; see the LICENSE file for details."
                  << sync_endl;

    else if (!token.empty() && token[0] != '#')
        sync_cout << "Unknown command: '" << cmd << "'. Type help for more information." << sync_endl;

    if (token == "quit")
        quit = true;
}

// datagen <output> [--threads N] [--nodes N] [--positions N] [--games N] [--seed N]
//         [--random-plies N] [--max-ply N] [--hash MB]
void UCIEngine::datagen_command(std::istringstream& is) {
    threads.wait_for_search_finished();

    datagen::Options opt;
    std::string      token;
    is >> opt.output;

    while (is >> token) {
        if (token == "--threads")
            is >> opt.threads;
        else if (token == "--nodes")
            is >> opt.nodes;
        else if (token == "--positions")
            is >> opt.positions;
        else if (token == "--games")
            is >> opt.games;
        else if (token == "--seed")
            is >> opt.seed;
        else if (token == "--random-plies")
            is >> opt.randomPlies;
        else if (token == "--max-ply")
            is >> opt.maxPly;
        else if (token == "--hash")
            is >> opt.hashMb;
        else {
            sync_cout << "datagen: unknown option " << token << sync_endl;
            return;
        }
    }

    if (opt.output.empty() || opt.output[0] == '-') {
        sync_cout << "usage: datagen <output.bin> [--threads N] [--nodes N] [--positions N] [--games N] [--seed N]"
                     " [--random-plies N] [--max-ply N] [--hash MB]"
                  << sync_endl;
        return;
    }

    datagen::run(opt);
}

// setoption name <name> [value <value>]
void UCIEngine::setoption(std::istringstream& is) {
    threads.wait_for_search_finished();

    std::string token, name, value;

    is >> token;  // "name"

    while (is >> token && token != "value")
        name += (name.empty() ? "" : " ") + token;

    while (is >> token)
        value += (value.empty() ? "" : " ") + token;

    if (options.contains(name))
        options.set(name, value);
    else
        sync_cout << "No such option: " << name << sync_endl;
}

// position [startpos | fen <fen>] [moves <m1> <m2> ...]
void UCIEngine::position(std::istringstream& is) {
    threads.wait_for_search_finished();

    std::string token, fen;
    is >> token;

    if (token == "startpos") {
        fen = StartFEN;
        is >> token;  // consume "moves" if present
    } else if (token == "fen") {
        while (is >> token && token != "moves")
            fen += token + " ";
    } else
        return;

    pos.set(fen);

    Move m;
    while (is >> token && (m = pos.uci_to_move(token)) != Move::none())
        pos.do_move(m);
}

Limits UCIEngine::parse_limits(std::istream& is) const {
    Limits      limits;
    std::string token;

    limits.startTime = now();  // as early as possible

    while (is >> token)
        if (token == "searchmoves")
            while (is >> token)
                limits.searchmoves.push_back(pos.uci_to_move(token));

        else if (token == "wtime")
            is >> limits.time[WHITE];
        else if (token == "btime")
            is >> limits.time[BLACK];
        else if (token == "winc")
            is >> limits.inc[WHITE];
        else if (token == "binc")
            is >> limits.inc[BLACK];
        else if (token == "movestogo")
            is >> limits.movestogo;
        else if (token == "depth")
            is >> limits.depth;
        else if (token == "nodes")
            is >> limits.nodes;
        else if (token == "movetime")
            is >> limits.movetime;
        else if (token == "mate")
            is >> limits.mate;
        else if (token == "perft")
            is >> limits.perft;
        else if (token == "infinite")
            limits.infinite = true;

    return limits;
}

void UCIEngine::go(std::istringstream& is) {
    const Limits limits = parse_limits(is);

    if (limits.perft) {
        wait_and_print_perft(limits.perft);
        return;
    }

    threads.start_thinking(pos, limits);
}

void UCIEngine::wait_and_print_perft(Depth depth) {
    threads.wait_for_search_finished();

    const TimePoint     start   = now();
    const std::uint64_t nodes   = perft<true>(pos, depth);
    const TimePoint     elapsed = now() - start + 1;

    sync_cout << "\nNodes searched: " << nodes << "\nTime (ms): " << elapsed - 1
              << "\nNodes/second: " << 1000 * nodes / std::uint64_t(elapsed) << sync_endl;
}

// Runs a fixed set of searches and reports the total node count, which should
// be identical for identical builds and serves as a regression signature.
void UCIEngine::bench(std::istringstream& args) {
    std::uint64_t nodes = 0, cnt = 1;
    bool          quit = false;

    const std::vector<std::string> list = setup_bench(pos, args);
    const auto                     num  = std::count_if(list.begin(), list.end(),
                                                        [](const std::string& s) { return s.find("go ") == 0 || s.find("eval") == 0; });

    const TimePoint elapsedStart = now();

    for (const auto& cmd : list) {
        std::istringstream is(cmd);
        std::string        token;
        is >> std::skipws >> token;

        if (token == "go" || token == "eval") {
            std::cerr << "\nPosition: " << cnt++ << '/' << num << " (" << pos.fen() << ")" << std::endl;
            if (token == "go") {
                go(is);
                threads.wait_for_search_finished();
                nodes += threads.nodes_searched();
            } else
                run_command(cmd, quit);
        } else
            run_command(cmd, quit);
    }

    const TimePoint elapsed = now() - elapsedStart + 1;

    std::cerr << "\n===========================" << "\nTotal time (ms) : " << elapsed - 1
              << "\nNodes searched  : " << nodes << "\nNodes/second    : " << 1000 * nodes / std::uint64_t(elapsed)
              << std::endl;
}

}  // namespace sp
