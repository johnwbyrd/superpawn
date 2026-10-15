// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "thread.h"

#include <algorithm>
#include <cstdlib>
#include <unordered_map>

#include "movegen.h"

namespace sp {

// The OS thread starts in idle_loop() and waits until the first search. The
// Worker is created once the thread is known to be idle.
Thread::Thread(ThreadPool& threads, TranspositionTable& tt, const SearchOptions& options, std::size_t idx) :
    stdThread(&Thread::idle_loop, this) {
    wait_for_search_finished();
    worker = std::make_unique<Worker>(threads, tt, options, idx);
}

// Wakes the thread so that it can see `exit` and return from idle_loop().
Thread::~Thread() {
    assert(!searching);
    exit = true;
    start_searching();
    stdThread.join();
}

void Thread::start_searching() {
    std::lock_guard<std::mutex> lk(mutex);
    searching = true;
    cv.notify_one();
}

void Thread::wait_for_search_finished() {
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait(lk, [&] { return !searching; });
}

void Thread::idle_loop() {
    while (true) {
        std::unique_lock<std::mutex> lk(mutex);
        searching = false;
        cv.notify_one();  // wake anyone waiting for the search to finish
        cv.wait(lk, [&] { return searching; });

        if (exit)
            return;

        lk.unlock();

        worker->start_searching();
    }
}

ThreadPool::~ThreadPool() {
    if (!threads.empty()) {
        main_thread()->wait_for_search_finished();
        threads.clear();
    }
}

// Creates or destroys threads to match the requested count. Called when the
// Threads option changes, and once at startup.
void ThreadPool::set(int requested) {
    if (!threads.empty()) {
        main_thread()->wait_for_search_finished();
        threads.clear();
    }

    requested       = std::max(1, requested);
    options.threads = requested;

    for (int i = 0; i < requested; ++i)
        threads.emplace_back(std::make_unique<Thread>(*this, tt, options, std::size_t(i)));

    clear();
}

// Resets the search state that persists between moves (histories, time
// management data). Called on ucinewgame.
void ThreadPool::clear() {
    for (auto& th : threads)
        th->worker->clear();

    main_worker()->bestPreviousScore        = VALUE_NONE;
    main_worker()->bestPreviousAverageScore = VALUE_NONE;
    main_worker()->previousTimeReduction    = 1.0;
    main_worker()->callsCnt                 = 0;
}

// Copies the position to every thread and wakes the main thread, which in turn
// wakes the helpers. Returns at once; the search runs in the background.
void ThreadPool::start_thinking(const Position& pos, Limits limits) {
    main_thread()->wait_for_search_finished();

    stop = abortedSearch = false;

    RootMoves rootMoves;
    for (const auto& m : MoveList<LEGAL>(pos))
        if (limits.searchmoves.empty() || std::count(limits.searchmoves.begin(), limits.searchmoves.end(), m))
            rootMoves.emplace_back(m);

    for (auto& th : threads) {
        Worker& w   = *th->worker;
        w.limits    = limits;
        w.nodes     = 0;
        w.rootDepth = w.completedDepth = 0;
        w.selDepth                     = 0;
        w.rootMoves                    = rootMoves;
        w.rootPos                      = pos;
    }

    main_thread()->start_searching();
}

void ThreadPool::start_searching_helpers() {
    for (auto& th : threads)
        if (th != threads.front())
            th->start_searching();
}

void ThreadPool::wait_for_search_finished() {
    for (auto& th : threads)
        th->wait_for_search_finished();
}

void ThreadPool::wait_for_helpers_finished() {
    for (auto& th : threads)
        if (th != threads.front())
            th->wait_for_search_finished();
}

std::uint64_t ThreadPool::nodes_searched() const {
    std::uint64_t sum = 0;
    for (auto& th : threads)
        sum += th->worker->nodes.load(std::memory_order_relaxed);
    return sum;
}

// Picks the thread whose result to play. Threads vote for their best move,
// weighted by score and completed depth; a proven mate always wins.
Thread* ThreadPool::best_thread() const {
    Thread* best     = threads.front().get();
    Value   minScore = VALUE_NONE;

    for (auto& th : threads)
        minScore = std::min(minScore, th->worker->rootMoves[0].score);

    // Votes are positive by construction
    std::unordered_map<std::uint16_t, std::int64_t> votes;
    for (auto& th : threads)
        votes[th->worker->rootMoves[0].pv[0].raw()] +=
            (th->worker->rootMoves[0].score - minScore + 14) * int(th->worker->completedDepth);

    for (auto& th : threads) {
        const Value bestScore = best->worker->rootMoves[0].score;
        const Value newScore  = th->worker->rootMoves[0].score;

        const auto bestVote = votes[best->worker->rootMoves[0].pv[0].raw()];
        const auto newVote  = votes[th->worker->rootMoves[0].pv[0].raw()];

        if (std::abs(bestScore) >= VALUE_TB_WIN_IN_MAX_PLY) {
            // Keep the shortest mate / longest mated line
            if (newScore > bestScore)
                best = th.get();
        } else if (newScore >= VALUE_TB_WIN_IN_MAX_PLY
                   || (newScore > VALUE_TB_LOSS_IN_MAX_PLY
                       && (newVote > bestVote
                           || (newVote == bestVote && th->worker->completedDepth > best->worker->completedDepth))))
            best = th.get();
    }

    return best;
}

}  // namespace sp
