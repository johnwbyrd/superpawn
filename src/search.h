// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "history.h"
#include "misc.h"
#include "movepick.h"
#include "position.h"
#include "timeman.h"
#include "tt.h"
#include "types.h"

namespace sp {

class ThreadPool;

// Search limits from the `go` command.
struct Limits {
    bool use_time_management() const { return time[WHITE] || time[BLACK]; }

    std::vector<Move> searchmoves;
    TimePoint         time[COLOR_NB] = {0, 0}, inc[COLOR_NB] = {0, 0};
    TimePoint         movetime = 0, startTime = 0;
    std::int64_t      nodes     = 0;
    int               movestogo = 0, depth = 0, mate = 0, perft = 0;
    bool              infinite = false;
};

// UCI options the search reads. Set by the UCI layer before each search.
struct SearchOptions {
    int  threads      = 1;
    int  multiPV      = 1;
    int  moveOverhead = 10;
    bool quiet        = false;  // suppress all UCI output (data generation)
};

// A root move with the principal variation found below it.
struct RootMove {
    explicit RootMove(Move m) : pv(1, m) {}
    bool operator==(const Move& m) const { return pv[0] == m; }
    // Sort in descending order by score, then by the previous iteration's score.
    bool operator<(const RootMove& m) const {
        return m.score != score ? m.score < score : m.previousScore < previousScore;
    }

    Value             score         = -VALUE_INFINITE;
    Value             previousScore = -VALUE_INFINITE;
    Value             averageScore  = -VALUE_INFINITE;
    int               selDepth      = 0;
    std::uint64_t     effort        = 0;
    std::vector<Move> pv;
};

using RootMoves = std::vector<RootMove>;

// Per-ply search state. The stack is indexed relative to the current ply so
// that (ss - 1) is the parent node.
struct Stack {
    Move*           pv;
    PieceToHistory* continuationHistory;
    int             ply;
    Move            currentMove;
    Move            excludedMove;
    Move            killers[2];
    Value           staticEval;
    int             statScore;
    int             moveCount;
    bool            inCheck;
    bool            ttPv;
    bool            ttHit;
    int             cutoffCnt;
    int             doubleExtensions;
};

enum NodeType { NonPV, PV, Root };

// Everything a single search thread owns: its copy of the root position, its
// history tables and its node counter. Threads share only the transposition
// table and the stop flag, which is what makes this Lazy SMP.
class Worker {
   public:
    Worker(ThreadPool& threads, TranspositionTable& tt, const SearchOptions& options, std::size_t threadIdx);

    void clear();
    void start_searching();

    bool is_main_thread() const { return threadIdx == 0; }

    // Set up by ThreadPool::start_thinking()
    Limits    limits;
    Position  rootPos;
    RootMoves rootMoves;

    std::atomic<std::uint64_t> nodes;
    Depth                      rootDepth, completedDepth;
    int                        selDepth, nmpMinPly;
    Value                      rootDelta;
    std::size_t                pvIdx;

    ButterflyHistory      mainHistory;
    CapturePieceToHistory captureHistory;
    ContinuationHistory   continuationHistory[2][2];  // [in check][capture]

    // Main thread only
    TimeManagement tm;
    int            callsCnt              = 0;
    double         previousTimeReduction = 1.0;
    Value          bestPreviousScore = VALUE_NONE, bestPreviousAverageScore = VALUE_NONE;
    Value          iterValue[4] = {};
    Move           lastBestMove;
    Depth          lastBestMoveDepth = 0;
    int            bestMoveChanges   = 0;

   private:
    void iterative_deepening();

    template <NodeType nodeType>
    Value search(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode);

    template <NodeType nodeType>
    Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta);

    Depth reduction(bool improving, Depth depth, int moveCount, int delta) const;
    void  check_time();

    void update_all_stats(const Position& pos,
                          Stack*          ss,
                          Move            bestMove,
                          Square          prevSq,
                          Move*           quietsSearched,
                          int             quietCount,
                          Move*           capturesSearched,
                          int             captureCount,
                          Depth           depth);
    void update_quiet_histories(const Position& pos, Stack* ss, Move move, int bonus);
    void update_continuation_histories(Stack* ss, Piece pc, Square to, int bonus);

    std::string pv_info(Depth depth, Value alpha, Value beta) const;

    ThreadPool&          threads;
    TranspositionTable&  tt;
    const SearchOptions& options;
    const std::size_t    threadIdx;
    int                  reductions[MAX_MOVES];
};

}  // namespace sp
