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
    tools/sprt/ SPRT testing with fastchess
    tools/train/  the bullet trainer and the training pipeline
    nets/       the embedded NNUE network (not committed)

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

Evaluation (`eval.*`, `nnue.*`)
-------------------------------

The NNUE evaluator is a perspective network: 768 binary inputs per side (piece
colour relative to the viewer, piece type, square flipped for Black) feed 1024
hidden neurons per perspective; both hidden vectors, side to move first, pass
through a squared clipped ReLU into one of 8 output neurons chosen by piece
count. Weights are 16-bit integers quantised by 255 (first layer) and 64
(output layer); the output is scaled to centipawns by 400.

The first layer is incremental. Every `StateInfo` records the inputs its move
changed (at most three piece placements), and the position keeps a parallel
stack of accumulators. Accumulators are computed lazily when a node is
evaluated, by walking back to the nearest computed ancestor and applying the
recorded changes, or from scratch when there is none. Nodes that are never
evaluated cost nothing.

Three code paths are selected at compile time: AVX-512, AVX2 and scalar. The
scalar output layer is also the reference the unit tests compare the SIMD
path against, alongside a test that incremental accumulators equal a fresh
refresh over thousands of random moves.

Training data comes from the engine itself (`datagen.*`): each thread plays
games that open with random moves and continue with fixed-node searches,
and writes quiet positions with the search score and the game result in the
bulletformat `ChessBoard` layout, which the trainer reads directly. The
pipeline is described in `tools/train/README.md`.

The network file format is documented in `src/nnue.h`. A network is embedded
at build time from `nets/default.nnue` (or `SP_EMBED_NET`) and can be
replaced at run time with the `EvalFile` option. Without a network the engine
falls back to a placeholder hand-crafted evaluation of tapered material and
piece-square tables, so it plays sensible chess until a trained network
exists.

Roadmap
-------

Each step below is a separate change, tested in isolation.

1. **Done.** Board, move generation (perft-exact), UCI, threaded search,
   placeholder evaluation, unit tests, CI.
2. **Done.** SPRT testing with fastchess: `tools/sprt/sprt.sh` builds the
   working tree and a baseline commit, fetches fastchess and the UHO book,
   and runs the test with the bounds in `tools/sprt/README.md`.
3. **Done.** NNUE inference: perspective network, 768 inputs per side,
   1024 hidden neurons, SCReLU, 8 output buckets, lazily updated
   accumulators, AVX-512, AVX2 and scalar paths, embedding and `EvalFile`.
   King buckets and horizontal mirroring are candidates for later.
4. **Done.** NNUE training: the `datagen` command plays fixed-node
   self-play games on many threads and writes bulletformat records; the
   bullet trainer in `tools/train/` trains the engine's architecture and
   exports it; `convert` turns the export into the engine's file format.
   The loop from here is: generate data with the current network, train,
   SPRT, repeat. The first network is still to be trained.
5. **Search refinement.** Correction history, capture history in SEE
   thresholds, improving-rate reductions, and the long tail of SPRT-tested
   patches. Syzygy tablebase probing via Fathom.
6. **Tuning.** SPSA over UCI-exposed search parameters.
7. **Chess960**, pondering, and other protocol niceties as time allows.
