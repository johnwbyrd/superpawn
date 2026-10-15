// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#define DOCTEST_CONFIG_IMPLEMENT
#include "bitboard.h"
#include "doctest/doctest.h"
#include "eval.h"
#include "position.h"

int main(int argc, char** argv) {
    sp::Bitboards::init();
    sp::Position::init();
    sp::Eval::init();

    return doctest::Context(argc, argv).run();
}
