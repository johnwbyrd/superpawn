// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include "misc.h"
#include "types.h"

namespace sp {

struct Limits;

// Decides how long to think on a clock. optimum() is the time the search aims
// to use, maximum() is the hard limit checked during the search.
class TimeManagement {
   public:
    void init(const Limits& limits, Color us, int ply, int moveOverhead);

    TimePoint optimum() const { return optimumTime; }
    TimePoint maximum() const { return maximumTime; }
    TimePoint elapsed() const { return now() - startTime; }

   private:
    TimePoint startTime   = 0;
    TimePoint optimumTime = 0;
    TimePoint maximumTime = 0;
};

}  // namespace sp
