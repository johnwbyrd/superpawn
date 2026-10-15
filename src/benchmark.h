// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace sp {

class Position;

// Builds the list of UCI commands run by `bench [hash] [threads] [limit]
// [fenfile|default|current] [depth|nodes|movetime]`. Default is a fixed set
// of positions searched to depth 13 on one thread, so the node count is
// reproducible and serves as a signature of the search.
std::vector<std::string> setup_bench(const Position& current, std::istream& is);

}  // namespace sp
