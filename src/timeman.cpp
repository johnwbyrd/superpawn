// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "timeman.h"

#include <algorithm>
#include <cmath>

#include "search.h"

namespace sp {

// Allocates thinking time for the current move. The budget is a fraction of
// the time left, where the fraction grows slowly with the game length and is
// capped so that a single move cannot use most of the clock.
void TimeManagement::init(const Limits& limits, Color us, int ply, int moveOverhead) {
    startTime   = limits.startTime;
    optimumTime = maximumTime = 0;

    if (!limits.use_time_management())
        return;

    // Assume there are at most 50 moves left when the GUI gives no count.
    const int mtg = limits.movestogo ? std::min(limits.movestogo, 50) : 50;

    // Time left for the remaining moves, less the communication overhead.
    const TimePoint timeLeft =
        std::max<TimePoint>(1, limits.time[us] + limits.inc[us] * (mtg - 1) - moveOverhead * (2 + mtg));

    double optScale, maxScale;

    if (limits.movestogo == 0) {
        // Sudden death or increment only: spend more per move as the game goes on.
        optScale =
            std::min(0.0120 + std::pow(ply + 3.0, 0.45) * 0.0039, 0.2 * double(limits.time[us]) / double(timeLeft));
        maxScale = std::min(6.5, 1.75 + 0.03 * ply);
    } else {
        // Fixed number of moves to the next time control.
        optScale = std::min((0.88 + ply / 116.4) / mtg, 0.88 * double(limits.time[us]) / double(timeLeft));
        maxScale = std::min(6.3, 1.5 + 0.11 * mtg);
    }

    optimumTime = TimePoint(optScale * double(timeLeft));
    maximumTime =
        TimePoint(std::min(0.825 * double(limits.time[us]) - moveOverhead, maxScale * double(optimumTime))) - 10;

    optimumTime = std::max<TimePoint>(optimumTime, 1);
    maximumTime = std::max<TimePoint>(maximumTime, optimumTime);
}

}  // namespace sp
