// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <cstdint>

#include "types.h"

namespace sp {

class Position;

// Counts the leaf nodes of the legal move tree to the given depth. With Root
// set, prints the count under each root move as well.
template <bool Root>
std::uint64_t perft(Position& pos, Depth depth);

}  // namespace sp
