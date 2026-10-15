// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "perft.h"

#include "misc.h"
#include "movegen.h"
#include "position.h"

namespace sp {

template <bool Root>
std::uint64_t perft(Position& pos, Depth depth) {
    std::uint64_t cnt, nodes = 0;
    const bool    leaf = (depth == 2);

    for (const auto& m : MoveList<LEGAL>(pos)) {
        if (Root && depth <= 1)
            cnt = 1, nodes++;
        else {
            pos.do_move(m);
            // At the last ply, count the legal moves instead of making them
            cnt = leaf ? MoveList<LEGAL>(pos).size() : perft<false>(pos, depth - 1);
            nodes += cnt;
            pos.undo_move(m);
        }
        if (Root)
            sync_cout << move_to_uci(m) << ": " << cnt << sync_endl;
    }
    return nodes;
}

template std::uint64_t perft<true>(Position&, Depth);
template std::uint64_t perft<false>(Position&, Depth);

}  // namespace sp
