// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
    #include <intrin.h>
#endif

#include "types.h"

namespace sp {

// Depths below zero are quiescence search depths. The entry offset keeps the
// stored 8-bit depth positive.
constexpr Depth DEPTH_QS           = 0;
constexpr Depth DEPTH_UNSEARCHED   = -2;
constexpr Depth DEPTH_ENTRY_OFFSET = -3;

// A transposition table entry packed into 10 bytes:
//   key        16 bit  low bits of the position key (the cluster index uses
//                      the high bits, so these must be independent of it)
//   depth       8 bit  search depth plus DEPTH_ENTRY_OFFSET
//   generation  5 bit  age, pv 1 bit, bound 2 bit
//   move       16 bit
//   value      16 bit
//   eval       16 bit  static evaluation
struct TTEntry {
    Move  move() const { return move16; }
    Value value() const { return value16; }
    Value eval() const { return eval16; }
    Depth depth() const { return Depth(depth8) + DEPTH_ENTRY_OFFSET; }
    bool  is_pv() const { return genBound8 & 0x4; }
    Bound bound() const { return Bound(genBound8 & 0x3); }
    bool  is_occupied() const { return bool(depth8); }

    void save(Key k, Value v, bool pv, Bound b, Depth d, Move m, Value ev, std::uint8_t generation8);

    std::uint8_t relative_age(std::uint8_t generation8) const;

   private:
    friend class TranspositionTable;

    std::uint16_t key16;
    std::uint8_t  depth8;
    std::uint8_t  genBound8;
    Move          move16;
    std::int16_t  value16;
    std::int16_t  eval16;
};

static_assert(sizeof(TTEntry) == 10, "TTEntry must be 10 bytes");

// The table is a flat array of 32-byte clusters holding three entries each.
// Access is lockless: concurrent threads may race on an entry, which is caught
// by the key check in probe() and by validating the hash move before use.
class TranspositionTable {
   public:
    ~TranspositionTable() { aligned_free(table); }

    void     resize(std::size_t mbSize, int threadCount);
    void     clear(int threadCount);
    int      hashfull(int maxAge = 0) const;
    void     new_search() { generation8 += GENERATION_DELTA; }
    uint8_t  generation() const { return generation8; }
    TTEntry* probe(Key key, bool& found) const;
    void     prefetch(Key key) const;

    // Entries with the same bound/pv bits but older generation are preferred
    // for replacement.
    static constexpr unsigned GENERATION_BITS  = 3;
    static constexpr int      GENERATION_DELTA = 1 << GENERATION_BITS;
    static constexpr int      GENERATION_CYCLE = 255 + GENERATION_DELTA;
    static constexpr int      GENERATION_MASK  = (0xFF << GENERATION_BITS) & 0xFF;

   private:
    static constexpr int ClusterSize = 3;

    struct Cluster {
        TTEntry entry[ClusterSize];
        char    padding[2];
    };

    static_assert(sizeof(Cluster) == 32, "Cluster must be 32 bytes");

    static void* aligned_alloc(std::size_t bytes);
    static void  aligned_free(void* p);

    // Multiply-high maps the key uniformly onto [0, clusterCount) without a
    // division and without requiring a power-of-two size.
    Cluster* first_entry(Key key) const {
#if defined(_MSC_VER)
        return &table[__umulh(key, std::uint64_t(clusterCount))];
#else
        __extension__ using uint128 = unsigned __int128;
        return &table[std::size_t(uint128(key) * uint128(clusterCount) >> 64)];
#endif
    }

    std::size_t  clusterCount = 0;
    Cluster*     table        = nullptr;
    std::uint8_t generation8  = 0;
};

}  // namespace sp
