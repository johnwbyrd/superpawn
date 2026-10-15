// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "types.h"

namespace sp {

// A saturating history counter. Updates pull the value towards +D or -D with a
// strength that falls off as the value approaches the bound, so recent results
// weigh more than old ones.
template <typename T, int D>
class StatsEntry {
    T entry;

   public:
    StatsEntry& operator=(const T& v) {
        entry = v;
        return *this;
    }
    operator const T&() const { return entry; }

    // Nested statistics (ContinuationHistory) are addressed through the entry.
    T* operator&() { return &entry; }
    T* operator->() { return &entry; }

    void operator<<(int bonus) {
        static_assert(D <= std::numeric_limits<T>::max(), "D overflows T");
        // Clamp to [-D, D] so that the gravity term cannot overshoot.
        bonus = std::clamp(bonus, -D, D);
        entry += T(bonus - entry * std::abs(bonus) / D);
        assert(std::abs(entry) <= D);
    }
};

// Multi-dimensional array of StatsEntry.
template <typename T, int D, int Size, int... Sizes>
struct Stats : public std::array<Stats<T, D, Sizes...>, Size> {
    using stats = Stats<T, D, Size, Sizes...>;

    void fill(const T& v) {
        // All levels are contiguous arrays of StatsEntry, so a flat fill works.
        using entry = StatsEntry<T, D>;
        entry* p    = reinterpret_cast<entry*>(this);
        std::fill(p, p + sizeof(*this) / sizeof(entry), v);
    }
};

template <typename T, int D, int Size>
struct Stats<T, D, Size> : public std::array<StatsEntry<T, D>, Size> {};

// Indexed by [colour][from_to]: how often a quiet move caused a beta cutoff.
using ButterflyHistory = Stats<std::int16_t, 7183, COLOR_NB, int(SQUARE_NB) * int(SQUARE_NB)>;

// Indexed by [piece][to]: used both as the continuation history leaf and for
// counter moves.
using PieceToHistory = Stats<std::int16_t, 29952, PIECE_NB, SQUARE_NB>;

// Indexed by [previous piece][previous to][piece][to]: history of a move given
// the move that preceded it one, two, four or six plies earlier.
using ContinuationHistory = Stats<PieceToHistory, 0, PIECE_NB, SQUARE_NB>;

// Indexed by [piece][to][captured type].
using CapturePieceToHistory = Stats<std::int16_t, 10692, PIECE_NB, SQUARE_NB, PIECE_TYPE_NB>;

}  // namespace sp
