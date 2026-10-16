// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include "history.h"
#include "movegen.h"
#include "position.h"
#include "types.h"

namespace sp {

// Produces moves one at a time in the order the search should try them, with
// generation deferred stage by stage so that a cutoff on the hash move or a
// good capture never pays for generating quiet moves.
class MovePicker {
   public:
    MovePicker(const MovePicker&)            = delete;
    MovePicker& operator=(const MovePicker&) = delete;

    // Main search and quiescence search (depth <= 0).
    MovePicker(const Position&              pos,
               Move                         ttMove,
               Depth                        depth,
               const ButterflyHistory*      mainHistory,
               const CapturePieceToHistory* captureHistory,
               const PieceToHistory**       continuationHistory,
               const Move*                  killers);

    // ProbCut: captures that win at least `threshold` by static exchange.
    MovePicker(const Position& pos, Move ttMove, int threshold, const CapturePieceToHistory* captureHistory);

    Move next_move();
    void skip_quiet_moves() { skipQuiets = true; }

   private:
    template <typename Pred>
    Move select(Pred filter);
    template <GenType T>
    void score();

    ExtMove* begin() { return cur; }
    ExtMove* end() { return endMoves; }

    const Position&              pos;
    const ButterflyHistory*      mainHistory;
    const CapturePieceToHistory* captureHistory;
    const PieceToHistory**       continuationHistory;
    Move                         ttMove;
    Move                         killers[2];
    ExtMove *                    cur, *endBadCaptures, *endMoves, *beginBadQuiets, *endBadQuiets;
    int                          stage;
    int                          threshold = 0;
    Depth                        depth;
    bool                         skipQuiets = false;
    ExtMove                      moves[MAX_MOVES];
};

}  // namespace sp
