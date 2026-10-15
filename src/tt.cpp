// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "tt.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
    #include <malloc.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
    #include <xmmintrin.h>
#endif

namespace sp {

// Writes to an entry. The entry is only overwritten by a worse-depth search of
// the same position when the bound is exact or the entry is stale.
void TTEntry::save(Key k, Value v, bool pv, Bound b, Depth d, Move m, Value ev, std::uint8_t generation8) {
    const std::uint16_t k16 = std::uint16_t(k >> 48);

    // Keep the old move if the new search found none.
    if (m || k16 != key16)
        move16 = m;

    // Overwrite less valuable entries. Lower-depth entries for the same
    // position are kept unless the new bound is exact.
    if (b == BOUND_EXACT || k16 != key16 || d - DEPTH_ENTRY_OFFSET + 2 * pv > depth8 - 4 || relative_age(generation8)) {
        assert(d > DEPTH_ENTRY_OFFSET);
        assert(d < 256 + DEPTH_ENTRY_OFFSET);

        key16     = k16;
        depth8    = std::uint8_t(d - DEPTH_ENTRY_OFFSET);
        genBound8 = std::uint8_t(generation8 | std::uint8_t(pv) << 2 | b);
        value16   = std::int16_t(v);
        eval16    = std::int16_t(ev);
    }
}

std::uint8_t TTEntry::relative_age(std::uint8_t generation8) const {
    // The cycle keeps the result correct when the generation counter wraps.
    return (TranspositionTable::GENERATION_CYCLE + generation8 - genBound8) & TranspositionTable::GENERATION_MASK;
}

void* TranspositionTable::aligned_alloc(std::size_t bytes) {
#if defined(_MSC_VER)
    return _aligned_malloc(bytes, 64);
#else
    void* p = nullptr;
    if (posix_memalign(&p, 64, bytes) != 0)
        return nullptr;
    return p;
#endif
}

void TranspositionTable::aligned_free(void* p) {
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}

void TranspositionTable::resize(std::size_t mbSize, int threadCount) {
    aligned_free(table);

    clusterCount = mbSize * 1024 * 1024 / sizeof(Cluster);
    table        = static_cast<Cluster*>(aligned_alloc(clusterCount * sizeof(Cluster)));

    if (!table) {
        std::fprintf(stderr, "Failed to allocate %zu MB for the transposition table\n", mbSize);
        std::exit(EXIT_FAILURE);
    }

    clear(threadCount);
}

// Zeroes the table using several threads, which matters for large tables.
void TranspositionTable::clear(int threadCount) {
    generation8 = 0;

    std::vector<std::thread> threads;
    threadCount = std::max(1, threadCount);

    for (int idx = 0; idx < threadCount; ++idx)
        threads.emplace_back([this, idx, threadCount]() {
            const std::size_t stride = clusterCount / std::size_t(threadCount);
            const std::size_t start  = stride * std::size_t(idx);
            const std::size_t len    = idx != threadCount - 1 ? stride : clusterCount - start;
            std::memset(static_cast<void*>(&table[start]), 0, len * sizeof(Cluster));
        });

    for (auto& th : threads)
        th.join();
}

// Approximate fill rate in permille, sampled from the first thousand clusters.
int TranspositionTable::hashfull(int maxAge) const {
    const int maxAgeInternal = maxAge << GENERATION_BITS;
    int       cnt            = 0;

    for (int i = 0; i < 1000; ++i)
        for (int j = 0; j < ClusterSize; ++j)
            cnt += table[i].entry[j].is_occupied() && table[i].entry[j].relative_age(generation8) <= maxAgeInternal;

    return cnt / ClusterSize;
}

// Looks up a position. Returns the matching entry if found, otherwise the entry
// in the cluster that is the best candidate for replacement.
TTEntry* TranspositionTable::probe(Key key, bool& found) const {
    TTEntry* const      tte = first_entry(key)->entry;
    const std::uint16_t k16 = std::uint16_t(key >> 48);

    for (int i = 0; i < ClusterSize; ++i)
        if (tte[i].key16 == k16)
            return found = bool(tte[i].is_occupied()), &tte[i];

    // Replace the shallowest entry, preferring old generations.
    TTEntry* replace = tte;
    for (int i = 1; i < ClusterSize; ++i)
        if (replace->depth8 - replace->relative_age(generation8) * 2
            > tte[i].depth8 - tte[i].relative_age(generation8) * 2)
            replace = &tte[i];

    return found = false, replace;
}

void TranspositionTable::prefetch(Key key) const {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(first_entry(key));
#elif defined(_MSC_VER)
    _mm_prefetch(reinterpret_cast<const char*>(first_entry(key)), _MM_HINT_T0);
#endif
}

}  // namespace sp
