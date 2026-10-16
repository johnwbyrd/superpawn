// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <string>

#include "nnue.h"
#include "types.h"

namespace sp {

class Position;

namespace Eval {

void init();

// The network in use, or nullptr when evaluating with the hand-crafted terms.
const nnue::Network* network();

// Loads a network from a file, or the embedded one when `path` is empty or
// "<embedded>". Returns a message describing the outcome for the UCI layer;
// on failure the previous network stays in use.
std::string set_network(const std::string& path);

// One line naming the evaluation in use, printed at startup.
std::string info();

// Static evaluation from the side to move's point of view, in centipawns.
Value evaluate(const Position& pos);

// The hand-crafted evaluation on its own, for diagnostics.
Value evaluate_hce(const Position& pos);

// Human-readable breakdown, for the `eval` command.
std::string trace(const Position& pos);

}  // namespace Eval

}  // namespace sp
