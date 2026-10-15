// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <string>

#include "types.h"

namespace sp {

class Position;

namespace Eval {

void init();

// Static evaluation from the side to move's point of view, in centipawns.
Value evaluate(const Position& pos);

// Human-readable breakdown, for the `eval` command.
std::string trace(const Position& pos);

}  // namespace Eval

}  // namespace sp
