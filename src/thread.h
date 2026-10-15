// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "position.h"
#include "search.h"
#include "tt.h"

namespace sp {

// An OS thread that sleeps until told to search. The search state itself lives
// in the Worker so that it can be inspected after the thread goes idle.
class Thread {
   public:
    Thread(ThreadPool& threads, TranspositionTable& tt, const SearchOptions& options, std::size_t idx);
    ~Thread();

    void start_searching();
    void wait_for_search_finished();

    std::unique_ptr<Worker> worker;

   private:
    void idle_loop();

    std::mutex              mutex;
    std::condition_variable cv;
    bool                    exit = false, searching = true;
    std::thread             stdThread;
};

class ThreadPool {
   public:
    ThreadPool(TranspositionTable& t, SearchOptions& o) : tt(t), options(o) {}
    ~ThreadPool();

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void set(int requested);
    void clear();
    void start_thinking(const Position& pos, Limits limits);
    void start_searching_helpers();
    void wait_for_search_finished();
    void wait_for_helpers_finished();

    Thread*                                     main_thread() const { return threads.front().get(); }
    Worker*                                     main_worker() const { return main_thread()->worker.get(); }
    Thread*                                     best_thread() const;
    std::uint64_t                               nodes_searched() const;
    std::size_t                                 size() const { return threads.size(); }
    const std::vector<std::unique_ptr<Thread>>& all() const { return threads; }

    std::atomic<bool> stop{false}, abortedSearch{false};

   private:
    std::vector<std::unique_ptr<Thread>> threads;
    TranspositionTable&                  tt;
    SearchOptions&                       options;
};

}  // namespace sp
