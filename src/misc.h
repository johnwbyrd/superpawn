// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

namespace sp {

std::string engine_info(bool toUci = false);
std::string compiler_info();

// Milliseconds since an arbitrary epoch, monotonic.
using TimePoint = std::chrono::milliseconds::rep;
static_assert(sizeof(TimePoint) == sizeof(std::int64_t), "TimePoint should be 64 bits");

inline TimePoint now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Output from the search thread and the UCI thread can interleave; wrapping
// each message in sync_cout ... sync_endl serialises them.
enum SyncCout { IO_LOCK, IO_UNLOCK };
std::ostream& operator<<(std::ostream&, SyncCout);

#define sync_cout std::cout << sp::IO_LOCK
#define sync_endl std::endl << sp::IO_UNLOCK

}  // namespace sp
