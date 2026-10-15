Superpawn design
================

This document describes the architecture of the rewritten engine and the
roadmap towards its goals. It is kept short on purpose; the code is the
reference, and this is the map.

Goals
-----

- Be a serious engine. The long-term target is the top tier of open-source
  engines, and eventually a credible challenge to Stockfish.
- Performance first. Data layout, search structure and evaluation are chosen
  for speed on modern x86-64. Other platforms run the same code, correctly
  and more slowly.
- Readable. Every heuristic has a comment that says what it does and why;
  constants are named; nothing is clever for its own sake.
- Everything measurable. Changes to search and evaluation are accepted or
  rejected by SPRT testing, and the `bench` node count is the regression
  signature.

Layout
------

    src/        engine sources
    tests/      doctest unit tests
    tests/doctest/  vendored doctest header
    nets/       the embedded NNUE network (to come)
    tools/      training, testing and tuning scripts (to come)

Board representation (`types.h`, `bitboard.*`, `position.*`)
-------------------------------------------------------------

Squares are a1 = 0 to h8 = 63, rank-major. Pieces are `(colour << 3) | type`.
Moves are 16 bits: from, to, promotion piece and a two-bit type. Castling is
encoded king-from to king-to.

The board is a set of bitboards by piece type and by colour, plus a
piece-per-square array. Sliding attacks use fancy magic bitboards, generated
at startup; with `SP_USE_PEXT` the index is a hardware PEXT instead.

`Position` holds the board, the side to move and a vector of `StateInfo`,
one per move made. Everything that cannot be cheaply recomputed after an undo
(hash keys, castling rights, en passant square, fifty-move counter, captured
piece, checkers, pins) lives in `StateInfo`, so undo is a pop. The position is
a plain value: each search thread gets its own copy.

Zobrist hashing is incremental. The en passant square is only recorded when a
capture is actually possible, so equivalent positions hash equal. Repetitions
are detected by walking back through the state vector.

`see_ge()` is a static exchange evaluator with pin awareness, used for move
ordering and pruning.

Move generation (`movegen.*`)
-----------------------------

Pseudo-legal, staged: captures (including queen promotions), quiets
(including underpromotions), evasions when in check, or everything. Only
moves that can be illegal (king moves, pinned pieces, en passant, castling)
are checked with `Position::legal()`. Bulk-counted perft runs at well over
100 million nodes per second on a modern core.

Transposition table (`tt.*`)
----------------------------

Ten-byte entries in 32-byte clusters of three, with a 16-bit key check,
generation-based ageing and depth-preferred replacement. Access is lockless:
threads may race on an entry; the key check catches most corruption and the
search validates every hash move with `pseudo_legal()` before using it.

Search (`search.*`, `movepick.*`, `history.h`, `thread.*`, `timeman.*`)
-----------------------------------------------------------------------

Iterative deepening with aspiration windows and MultiPV, over a principal
variation search. The node loop has, in order: transposition table cutoff,
static evaluation with history-based adjustment, razoring, reverse futility
pruning, null move pruning with verification, internal iterative reductions,
ProbCut, then the move loop with late move pruning, futility pruning,
continuation-history pruning, SEE pruning, singular extensions with
multi-cut, late move reductions scaled by history, and a full re-search on a
fail high. Quiescence search resolves captures and check evasions with
futility and SEE pruning.

Move ordering is the hash move, then captures by victim value and capture
history with losing captures deferred, then quiets by butterfly history,
continuation history (one, two and four plies back) and killer moves.
History tables use the gravity update: bonuses are damped as the counter
approaches its bound.

Threads are Lazy SMP. Each `Worker` owns a root position, root move list,
history tables and node counter; threads share only the transposition table
and the stop flag. The main thread handles time management, output, and
chooses the final move by a depth-and-score weighted vote.

Time management budgets an optimum and a maximum per move from the clock,
scaled by how much the best move has been changing and how the score has
been trending.

Evaluation (`eval.*`)
---------------------

A placeholder: tapered material and piece-square tables derived from simple
formulas. It exists so the engine plays sensible chess until the NNUE
evaluator replaces it.

Roadmap
-------

Each step below is a separate change, tested in isolation.

1. **Done.** Board, move generation (perft-exact), UCI, threaded search,
   placeholder evaluation, unit tests, CI.
2. **Testing infrastructure.** `tools/` scripts for fastchess SPRT runs
   against the previous commit, an opening book, and a `bench` check in CI.
3. **NNUE inference.** A perspective network, 768 inputs per side (king
   bucket to follow), two accumulators updated incrementally in make and
   unmake, SCReLU activation, output buckets by material. AVX2 with AVX-512
   and scalar paths. Network embedded in the binary.
4. **NNUE training.** A `bullet`-based trainer in `tools/train/`, first on
   public Leela data, then on self-play data generated by the engine itself
   with a data generation command. Iterate: new data, new net, SPRT.
5. **Search refinement.** Correction history, capture history in SEE
   thresholds, improving-rate reductions, and the long tail of SPRT-tested
   patches. Syzygy tablebase probing via Fathom.
6. **Tuning.** SPSA over UCI-exposed search parameters.
7. **Chess960**, pondering, and other protocol niceties as time allows.
