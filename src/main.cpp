// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include <iostream>

#include "bitboard.h"
#include "eval.h"
#include "misc.h"
#include "position.h"
#include "uci.h"

int main(int argc, char** argv) {
    std::cout << sp::engine_info() << std::endl;

    sp::Bitboards::init();
    sp::Position::init();
    sp::Eval::init();

    sp::UCIEngine uci(argc, argv);
    uci.loop();

    return 0;
}
