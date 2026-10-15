// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <algorithm>

#include "types.h"

namespace sp {

class Position;

// Which subset of moves to generate. The search asks for captures and quiets
// separately so that it can order and prune them differently; EVASIONS is used
// when in check; LEGAL is for perft, move validation and the root.
enum GenType {
    CAPTURES,      // captures and queen promotions
    QUIETS,        // non-captures, including underpromotions
    EVASIONS,      // all pseudo-legal moves when in check
    NON_EVASIONS,  // all pseudo-legal moves when not in check
    LEGAL          // strictly legal moves
};

// A move together with the score the move picker assigns to it.
struct ExtMove : public Move {
    int value;

    void operator=(Move m) { data = m.raw(); }

    // Keep ExtMove comparable only by value, never as a Move.
    operator float() const = delete;
};

inline bool operator<(const ExtMove& f, const ExtMove& s) {
    return f.value < s.value;
}

template <GenType>
ExtMove* generate(const Position& pos, ExtMove* moveList);

// A fixed-size move list filled at construction.
template <GenType T>
struct MoveList {
    explicit MoveList(const Position& pos) : last(generate<T>(pos, moveList)) {}

    const ExtMove* begin() const { return moveList; }
    const ExtMove* end() const { return last; }
    std::size_t    size() const { return std::size_t(last - moveList); }
    bool           contains(Move move) const { return std::find(begin(), end(), move) != end(); }

   private:
    ExtMove  moveList[MAX_MOVES];
    ExtMove* last;
};

}  // namespace sp
