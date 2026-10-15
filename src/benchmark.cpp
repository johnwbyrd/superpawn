// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "benchmark.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "position.h"

namespace sp {

namespace {

// A mix of openings, middlegames, endgames and tactical positions.
const std::vector<std::string> Defaults = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
    "r1bqk2r/ppp2ppp/2n2n2/2bpp3/4P3/2PP1N2/PP3PPP/RNBQKB1R w KQkq - 0 6",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NP1/PPP1NBBP/R2Q1RK1 b - - 0 11",
    "2rq1rk1/pb3pp1/1p2pb1p/8/3p4/1N1P2P1/PP3PBP/R2QR1K1 w - - 0 20",
    "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2PP1N2/PP3PPP/RN1QRBK1 b - - 0 13",
    "3r1rk1/pp4pp/2n1pn2/8/3P4/P2BBP2/1P3PPP/R2R2K1 w - - 0 18",
    "4rrk1/pp1n3p/3q2pQ/2p1pb1N/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
    "r3r1k1/ppqbbppp/2p2n2/3p4/3PP3/2N1BN2/PP3PPP/R2Q1RK1 w - - 0 14",
    "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 0 1",
    "8/8/8/8/5kp1/P7/8/1K1N4 w - - 0 1",
    "8/8/1p1r1k2/p1pPN1p1/P3P1p1/1P1R1P2/2KP4/8 w - - 0 1",
    "6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - - 0 1",
    "8/2p4P/8/kr6/6R1/8/8/1K6 w - - 0 1",
    "8/8/3P3k/8/1p6/8/1P6/1K3n2 b - - 0 1",
    "8/3p4/p1bk3p/Pp6/1Kp1PpPp/2BbP1pP/8/8 b - - 0 1",
    "5k2/8/8/8/8/8/8/4K2R w K - 0 1",
    "8/8/2K5/4B3/3N4/8/8/4k3 w - - 0 1",
    "4k3/8/8/8/8/8/8/4K2R w K - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "1r2k1r1/pbppnp1p/1b3P2/8/Q7/B1PB1q2/P4PPP/3R2K1 w - - 0 1",
    "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - 0 1",
    "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P1N3/8 w - - 0 1",
    "q4rk1/1n1Qbppp/2p5/1p2p3/1P2P3/2P4P/6P1/2B4K b - - 0 1",
    "6kr/1q2r1p1/1pRN1p1p/5p2/1P1P4/6P1/1P2Q2K/8 w - - 0 1",
    "1Q6/5pk1/2p3p1/1p2N2p/1b5P/1bn2R2/8/7K b - - 0 1",
    "8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
};

}  // namespace

std::vector<std::string> setup_bench(const Position& current, std::istream& is) {
    std::vector<std::string> fens, list;
    std::string              go, token;

    // Defaults
    std::string ttSize    = (is >> token) ? token : "16";
    std::string threads   = (is >> token) ? token : "1";
    std::string limit     = (is >> token) ? token : "13";
    std::string fenFile   = (is >> token) ? token : "default";
    std::string limitType = (is >> token) ? token : "depth";

    go = limitType == "eval" ? "eval" : "go " + limitType + " " + limit;

    if (fenFile == "default")
        fens = Defaults;
    else if (fenFile == "current")
        fens.push_back(current.fen());
    else {
        std::ifstream file(fenFile);
        if (!file.is_open()) {
            std::cerr << "Unable to open file " << fenFile << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::string fen;
        while (std::getline(file, fen))
            if (!fen.empty())
                fens.push_back(fen);
    }

    list.emplace_back("setoption name Threads value " + threads);
    list.emplace_back("setoption name Hash value " + ttSize);
    list.emplace_back("ucinewgame");

    for (const std::string& fen : fens) {
        list.emplace_back("position fen " + fen);
        list.emplace_back(go);
    }

    return list;
}

}  // namespace sp
