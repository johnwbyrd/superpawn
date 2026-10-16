// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "ucioption.h"

namespace sp {

// The UCI front end: owns the engine state and translates text commands into
// calls on the thread pool. Command-line arguments are run as commands first,
// so `superpawn bench` works without a GUI.
class UCIEngine {
   public:
    UCIEngine(int argc, char** argv);
    ~UCIEngine();

    void loop();

   private:
    void   run_command(const std::string& cmd, bool& quit);
    void   go(std::istringstream& is);
    void   position(std::istringstream& is);
    void   setoption(std::istringstream& is);
    void   bench(std::istringstream& is);
    void   datagen_command(std::istringstream& is);
    Limits parse_limits(std::istream& is) const;
    void   wait_and_print_perft(Depth depth);

    Position           pos;
    TranspositionTable tt;
    SearchOptions      searchOptions;
    ThreadPool         threads;
    OptionsMap         options;

    std::vector<std::string> cli;
};

}  // namespace sp
