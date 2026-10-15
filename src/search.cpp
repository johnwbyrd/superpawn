// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "search.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>

#include "eval.h"
#include "movegen.h"
#include "thread.h"

namespace sp {

namespace {

// Futility margin for reverse futility pruning.
Value futility_margin(Depth d, bool noTtCutNode, bool improving) {
    const Value futilityMult = 118 - 44 * noTtCutNode;
    return futilityMult * d - improving * futilityMult * 2;
}

// Moves after this many are pruned at shallow depth (late move pruning).
constexpr int futility_move_count(bool improving, Depth depth) {
    return improving ? (3 + depth * depth) : (3 + depth * depth) / 2;
}

// History bonuses and penalties grow with depth and saturate.
int stat_bonus(Depth d) {
    return std::min(190 * d - 108, 1596);
}
int stat_malus(Depth d) {
    return std::min(736 * d - 268, 2044);
}

// Draw scores carry a tiny node-dependent noise so that the search does not
// treat every drawing line as identical.
Value value_draw(std::uint64_t nodes) {
    return VALUE_DRAW - 1 + Value(nodes & 0x2);
}

// Mate scores are stored relative to the current node so they stay correct
// when the entry is reached at a different ply.
Value value_to_tt(Value v, int ply) {
    assert(v != VALUE_NONE);
    return v >= VALUE_TB_WIN_IN_MAX_PLY ? v + ply : v <= VALUE_TB_LOSS_IN_MAX_PLY ? v - ply : v;
}

Value value_from_tt(Value v, int ply, int r50c) {
    if (v == VALUE_NONE)
        return VALUE_NONE;

    // A mate that cannot be delivered before the fifty-move rule intervenes is
    // downgraded to a tablebase-like score.
    if (v >= VALUE_TB_WIN_IN_MAX_PLY) {
        if (v >= VALUE_MATE_IN_MAX_PLY && VALUE_MATE - v > 100 - r50c)
            return VALUE_TB_WIN_IN_MAX_PLY - 1;
        return v - ply;
    }

    if (v <= VALUE_TB_LOSS_IN_MAX_PLY) {
        if (v <= VALUE_MATED_IN_MAX_PLY && VALUE_MATE + v > 100 - r50c)
            return VALUE_TB_LOSS_IN_MAX_PLY + 1;
        return v + ply;
    }

    return v;
}

void update_pv(Move* pv, Move move, const Move* childPv) {
    for (*pv++ = move; childPv && *childPv != Move::none();)
        *pv++ = *childPv++;
    *pv = Move::none();
}

Value evaluate(const Position& pos) {
    return std::clamp(Eval::evaluate(pos), VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);
}

std::string score_to_uci(Value v) {
    std::ostringstream ss;
    if (std::abs(v) < VALUE_TB_WIN_IN_MAX_PLY)
        ss << "cp " << v;
    else if (std::abs(v) <= VALUE_TB)
        ss << "cp " << (v > 0 ? VALUE_TB_WIN_IN_MAX_PLY - 1 : VALUE_TB_LOSS_IN_MAX_PLY + 1);
    else
        ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;
    return ss.str();
}

}  // namespace

Worker::Worker(ThreadPool& th, TranspositionTable& t, const SearchOptions& o, std::size_t idx) :
    threads(th), tt(t), options(o), threadIdx(idx) {
    clear();
}

// Resets the history tables and the reduction table. Called on ucinewgame and
// when the thread count changes.
void Worker::clear() {
    mainHistory.fill(0);
    captureHistory.fill(0);

    for (bool inCheck : {false, true})
        for (bool capture : {false, true})
            for (auto& to : continuationHistory[inCheck][capture])
                for (auto& h : to)
                    h->fill(-71);

    // Reductions grow with the logarithm of depth and move count; more threads
    // reduce a little more, since the helpers add width of their own.
    for (std::size_t i = 1; i < MAX_MOVES; ++i)
        reductions[i] = int((20.37 + std::log(double(options.threads)) / 2) * std::log(double(i)));
}

Depth Worker::reduction(bool improving, Depth depth, int moveCount, int delta) const {
    const int reductionScale = reductions[depth] * reductions[moveCount];
    return (reductionScale + 1118 - delta * 832 / std::max(rootDelta, 1)) / 1024
         + (!improving && reductionScale > 1135);
}

// Entry point for each thread. The main thread also starts the helpers,
// chooses the final move and prints it.
void Worker::start_searching() {
    if (!is_main_thread()) {
        iterative_deepening();
        return;
    }

    tm.init(limits, rootPos.side_to_move(), rootPos.game_ply(), options.moveOverhead);
    tt.new_search();

    // No legal moves: checkmate or stalemate at the root. The helpers are not
    // started, so only this thread's result exists.
    if (rootMoves.empty()) {
        rootMoves.emplace_back(Move::none());
        sync_cout << "info depth 0 score " << score_to_uci(rootPos.checkers() ? -VALUE_MATE : VALUE_DRAW) << sync_endl;

        while (!threads.stop && limits.infinite)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        threads.stop = true;
        sync_cout << "bestmove (none)" << sync_endl;
        return;
    }

    threads.start_searching_helpers();
    iterative_deepening();

    // In infinite mode, wait for the GUI to send "stop" before replying.
    while (!threads.stop && limits.infinite)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    threads.stop = true;
    threads.wait_for_helpers_finished();

    // Pick the move from the thread that searched deepest or scored highest.
    Worker* best = threads.best_thread()->worker.get();

    bestPreviousScore        = best->rootMoves[0].score;
    bestPreviousAverageScore = best->rootMoves[0].averageScore;

    // Print the best thread's PV if it differs from ours.
    if (best != this)
        sync_cout << best->pv_info(best->completedDepth, -VALUE_INFINITE, VALUE_INFINITE) << sync_endl;

    std::string ponder;
    if (best->rootMoves[0].pv.size() > 1)
        ponder = " ponder " + move_to_uci(best->rootMoves[0].pv[1]);

    sync_cout << "bestmove " << move_to_uci(best->rootMoves[0].pv[0]) << ponder << sync_endl;
}

// Iterative deepening with aspiration windows. Each iteration searches the
// root to a greater depth, starting with a narrow window around the previous
// score and widening it on failure.
void Worker::iterative_deepening() {
    Stack  stack[MAX_PLY + 10] = {};
    Stack* ss                  = stack + 7;
    Move   pv[MAX_PLY + 1];
    Value  alpha, beta;
    Value  bestValue     = -VALUE_INFINITE;
    double timeReduction = 1, totBestMoveChanges = 0;
    int    delta, iterIdx                        = 0;

    // Sentinel continuation histories for the plies before the root
    for (int i = 7; i > 0; --i) {
        (ss - i)->continuationHistory = &continuationHistory[0][0][NO_PIECE][0];
        (ss - i)->staticEval          = VALUE_NONE;
    }

    for (int i = 0; i <= MAX_PLY + 2; ++i)
        (ss + i)->ply = i;

    ss->pv = pv;

    if (is_main_thread()) {
        if (bestPreviousScore == VALUE_NONE)
            std::fill_n(iterValue, 4, VALUE_ZERO);
        else
            std::fill_n(iterValue, 4, bestPreviousScore);
    }

    const std::size_t multiPV = std::min<std::size_t>(std::size_t(options.multiPV), rootMoves.size());

    nmpMinPly         = 0;
    completedDepth    = 0;
    rootDepth         = 0;
    selDepth          = 0;
    bestMoveChanges   = 0;
    lastBestMove      = Move::none();
    lastBestMoveDepth = 0;

    while (++rootDepth < MAX_PLY && !threads.stop && !(limits.depth && is_main_thread() && rootDepth > limits.depth)) {
        // Age out the best move changes of previous iterations
        if (is_main_thread())
            totBestMoveChanges /= 2;

        for (RootMove& rm : rootMoves)
            rm.previousScore = rm.score;

        // MultiPV: search each of the best `multiPV` lines in turn, excluding
        // the moves already settled this iteration.
        for (pvIdx = 0; pvIdx < multiPV && !threads.stop; ++pvIdx) {
            selDepth = 0;

            // Aspiration window around the previous score
            const Value avg = rootMoves[pvIdx].averageScore;
            delta           = 9 + avg * avg / 12800;
            alpha           = std::max(avg - delta, -VALUE_INFINITE);
            beta            = std::min(avg + delta, VALUE_INFINITE);

            if (rootDepth < 4) {
                alpha = -VALUE_INFINITE;
                beta  = VALUE_INFINITE;
            }

            int failedHighCnt = 0;
            while (true) {
                // Fail-highs are re-searched at a slightly lower depth, which
                // keeps the window climbing cheap.
                const Depth adjustedDepth = std::max(1, rootDepth - failedHighCnt);
                rootDelta                 = beta - alpha;
                bestValue                 = search<Root>(rootPos, ss, alpha, beta, adjustedDepth, false);

                // Keep the already-searched PV lines in order; the rest of the
                // moves sort by this iteration's score.
                std::stable_sort(rootMoves.begin() + pvIdx, rootMoves.end());

                if (threads.stop)
                    break;

                // Report a fail high/low after a few seconds so the GUI sees progress
                if (is_main_thread() && multiPV == 1 && (bestValue <= alpha || bestValue >= beta)
                    && tm.elapsed() > 3000)
                    sync_cout << pv_info(rootDepth, alpha, beta) << sync_endl;

                if (bestValue <= alpha) {
                    beta          = (alpha + beta) / 2;
                    alpha         = std::max(bestValue - delta, -VALUE_INFINITE);
                    failedHighCnt = 0;
                } else if (bestValue >= beta) {
                    beta = std::min(bestValue + delta, VALUE_INFINITE);
                    ++failedHighCnt;
                } else
                    break;

                delta += delta / 3;

                assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE);
            }

            std::stable_sort(rootMoves.begin(), rootMoves.begin() + pvIdx + 1);

            if (is_main_thread() && (threads.stop || pvIdx + 1 == multiPV || tm.elapsed() > 3000))
                sync_cout << pv_info(rootDepth, alpha, beta) << sync_endl;
        }

        if (!threads.stop)
            completedDepth = rootDepth;

        // A mate shorter than requested ends a "go mate" search.
        if (limits.mate && rootMoves[0].score == rootMoves[0].previousScore
            && rootMoves[0].score >= VALUE_MATE_IN_MAX_PLY && VALUE_MATE - rootMoves[0].score <= 2 * limits.mate)
            threads.stop = true;

        if (!is_main_thread())
            continue;

        // Time management: decide whether another iteration fits in the budget.
        if (limits.use_time_management() && !threads.stop) {
            // Spend less time when the score is stable or falling, more when the
            // best move keeps changing.
            const double fallingEval =
                std::clamp((11 + 2 * (bestPreviousAverageScore - bestValue) + (iterValue[iterIdx] - bestValue)) / 100.0,
                           0.58, 1.6);

            // Reduce time if the best move has been stable across iterations
            timeReduction          = completedDepth >= 8 && rootMoves[0].pv[0] == lastBestMove ? 1.5 : 0.8;
            const double reduction = (1.5 + previousTimeReduction) / (2.3 * timeReduction);

            for (const auto& th : threads.all())
                totBestMoveChanges += th->worker->bestMoveChanges, th->worker->bestMoveChanges = 0;

            const double bestMoveInstability = 1 + 1.8 * totBestMoveChanges / double(threads.size());

            const double totalTime = double(tm.optimum()) * fallingEval * reduction * bestMoveInstability;

            if (rootMoves.size() == 1)
                threads.stop = true;
            else if (double(tm.elapsed()) > totalTime)
                threads.stop = true;
        }

        if (rootMoves[0].pv[0] != lastBestMove) {
            lastBestMove      = rootMoves[0].pv[0];
            lastBestMoveDepth = rootDepth;
        }

        iterValue[iterIdx] = bestValue;
        iterIdx            = (iterIdx + 1) & 3;
    }

    if (is_main_thread())
        previousTimeReduction = timeReduction;
}

// The main alpha-beta search. Returns a score for the side to move within
// (alpha, beta), or a bound outside it.
template <NodeType nodeType>
Value Worker::search(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode) {
    constexpr bool PvNode   = nodeType != NonPV;
    constexpr bool rootNode = nodeType == Root;

    // Dive into quiescence search at the horizon
    if (depth <= 0)
        return qsearch < PvNode ? PV : NonPV > (pos, ss, alpha, beta);

    depth = std::min(depth, MAX_PLY - 1);

    assert(-VALUE_INFINITE <= alpha && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));
    assert(0 < depth && depth < MAX_PLY);
    assert(!(PvNode && cutNode));

    Move     pv[MAX_PLY + 1];
    Move     capturesSearched[32];
    Move     quietsSearched[32];
    TTEntry* tte;
    Key      posKey;
    Move     ttMove, move, excludedMove, bestMove;
    Depth    extension, newDepth;
    Value    bestValue, value, ttValue, eval, maxValue;
    bool     givesCheck, improving, priorCapture;
    bool     capture, ttCapture;
    int      moveCount, captureCount, quietCount;

    // Step 1. Initialise the node
    ss->inCheck    = bool(pos.checkers());
    priorCapture   = bool(pos.captured_piece());
    const Color us = pos.side_to_move();
    moveCount = captureCount = quietCount = ss->moveCount = 0;
    bestValue                                             = -VALUE_INFINITE;
    maxValue                                              = VALUE_INFINITE;

    if (is_main_thread())
        check_time();

    if (PvNode && selDepth < ss->ply + 1)
        selDepth = ss->ply + 1;

    if (!rootNode) {
        // Step 2. Check for an aborted search or an immediate draw
        if (threads.stop.load(std::memory_order_relaxed) || pos.is_draw(ss->ply) || ss->ply >= MAX_PLY)
            return (ss->ply >= MAX_PLY && !ss->inCheck) ? evaluate(pos) : value_draw(nodes);

        // Step 3. Mate distance pruning: a shorter mate has already been found,
        // so this node cannot improve on it.
        alpha = std::max(mated_in(ss->ply), alpha);
        beta  = std::min(mate_in(ss->ply + 1), beta);
        if (alpha >= beta)
            return alpha;
    }

    assert(0 <= ss->ply && ss->ply < MAX_PLY);

    bestMove               = Move::none();
    (ss + 1)->excludedMove = Move::none();
    (ss + 2)->killers[0] = (ss + 2)->killers[1] = Move::none();
    (ss + 2)->cutoffCnt                         = 0;
    const Square prevSq                         = (ss - 1)->currentMove.is_ok() ? (ss - 1)->currentMove.to() : SQ_NONE;
    ss->statScore                               = 0;

    // Step 4. Transposition table lookup
    excludedMove        = ss->excludedMove;
    posKey              = pos.key();
    tte                 = tt.probe(posKey, ss->ttHit);
    ttValue             = ss->ttHit ? value_from_tt(tte->value(), ss->ply, pos.rule50_count()) : VALUE_NONE;
    ttMove              = rootNode ? rootMoves[pvIdx].pv[0] : ss->ttHit ? tte->move() : Move::none();
    ttCapture           = ttMove && pos.capture_stage(ttMove);
    const Depth ttDepth = ss->ttHit ? tte->depth() : DEPTH_ENTRY_OFFSET;

    // An excluded move (singular search) keeps the ttPv flag of the real node
    if (!excludedMove)
        ss->ttPv = PvNode || (ss->ttHit && tte->is_pv());

    // At non-PV nodes a sufficiently deep entry with the right bound ends the search
    if (!PvNode && !excludedMove && ttDepth > depth - (ttValue <= beta) && ttValue != VALUE_NONE
        && (tte->bound() & (ttValue >= beta ? BOUND_LOWER : BOUND_UPPER)) && pos.rule50_count() < 90) {
        // Reward the hash move that produced the cutoff
        if (ttMove && ttValue >= beta) {
            if (!ttCapture)
                update_quiet_histories(pos, ss, ttMove, stat_bonus(depth));

            // Penalise the quiet parent move that allowed this refutation
            if (prevSq != SQ_NONE && (ss - 1)->moveCount <= 2 && !priorCapture)
                update_continuation_histories(ss - 1, pos.piece_on(prevSq), prevSq, -stat_malus(depth + 1));
        }

        return ttValue;
    }

    // Step 5. Static evaluation
    if (ss->inCheck) {
        ss->staticEval = eval = VALUE_NONE;
        improving             = false;
        goto moves_loop;
    } else if (excludedMove) {
        eval = ss->staticEval;  // computed before the singular search
    } else if (ss->ttHit) {
        ss->staticEval = eval = tte->eval() != VALUE_NONE ? tte->eval() : evaluate(pos);

        // The table value is a better guess than the static eval when its bound allows
        if (ttValue != VALUE_NONE && (tte->bound() & (ttValue > eval ? BOUND_LOWER : BOUND_UPPER)))
            eval = ttValue;
    } else {
        ss->staticEval = eval = evaluate(pos);
        tte->save(posKey, VALUE_NONE, ss->ttPv, BOUND_NONE, DEPTH_UNSEARCHED, Move::none(), eval, tt.generation());
    }

    // Use the change in static eval to grade the parent's quiet move
    if ((ss - 1)->currentMove.is_ok() && !(ss - 1)->inCheck && !priorCapture) {
        const int bonus = std::clamp(-10 * ((ss - 1)->staticEval + ss->staticEval), -1831, 1428) + 623;
        mainHistory[~us][(ss - 1)->currentMove.from_to()] << bonus;
    }

    // "Improving" means the static eval is better than two plies ago, which
    // makes pruning and reductions more cautious.
    improving = (ss - 2)->staticEval != VALUE_NONE ? ss->staticEval > (ss - 2)->staticEval
              : (ss - 4)->staticEval != VALUE_NONE ? ss->staticEval > (ss - 4)->staticEval
                                                   : true;

    // Step 6. Razoring: a hopeless position is verified with a quiescence search
    if (eval < alpha - 462 - 297 * depth * depth) {
        value = qsearch<NonPV>(pos, ss, alpha - 1, alpha);
        if (value < alpha && std::abs(value) < VALUE_TB_WIN_IN_MAX_PLY)
            return value;
    }

    // Step 7. Reverse futility pruning: the static eval is so far above beta
    // that a quiet reply is unlikely to bring it back.
    if (!ss->ttPv && depth < 9 && eval - futility_margin(depth, cutNode && !ss->ttHit, improving) >= beta
        && eval >= beta && eval < VALUE_TB_WIN_IN_MAX_PLY && (!ttMove || ttCapture))
        return beta > VALUE_TB_LOSS_IN_MAX_PLY ? (eval + beta) / 2 : eval;

    // Step 8. Null move pruning: if passing the turn still beats beta, the
    // position is good enough to cut.
    if (cutNode && (ss - 1)->currentMove != Move::null() && eval >= beta && ss->staticEval >= beta - 20 * depth + 198
        && !excludedMove && pos.non_pawn_material(us) && ss->ply >= nmpMinPly && beta > VALUE_TB_LOSS_IN_MAX_PLY) {
        assert(eval - beta >= 0);

        // Reduce more the further above beta we are
        const Depth R = std::min(int(eval - beta) / 148, 6) + depth / 3 + 4;

        ss->currentMove         = Move::null();
        ss->continuationHistory = &continuationHistory[0][0][NO_PIECE][0];

        pos.do_null_move();
        Value nullValue = -search<NonPV>(pos, ss + 1, -beta, -beta + 1, depth - R, false);
        pos.undo_null_move();

        if (nullValue >= beta && nullValue < VALUE_TB_WIN_IN_MAX_PLY) {
            if (nmpMinPly || depth < 16)
                return nullValue;

            assert(!nmpMinPly);

            // Verification search at high depth, with null move disabled for
            // the next few plies, guards against zugzwang.
            nmpMinPly = ss->ply + 3 * (depth - R) / 4;
            value     = search<NonPV>(pos, ss, beta - 1, beta, depth - R, false);
            nmpMinPly = 0;

            if (value >= beta)
                return nullValue;
        }
    }

    // Step 9. Internal iterative reductions: without a hash move the node is
    // probably not important, so search it a little shallower.
    if (PvNode && !ttMove)
        depth -= 3;

    if (depth <= 0)
        return qsearch<PV>(pos, ss, alpha, beta);

    if (cutNode && depth >= 8 && !ttMove)
        depth -= 2;

    // Step 10. ProbCut: a capture that beats beta by a wide margin at reduced
    // depth is almost certainly good enough at full depth.
    {
        const Value probCutBeta = beta + 170 - 64 * improving;
        if (!PvNode && depth > 3 && std::abs(beta) < VALUE_TB_WIN_IN_MAX_PLY
            && !(ttDepth >= depth - 3 && ttValue != VALUE_NONE && ttValue < probCutBeta)) {
            assert(probCutBeta < VALUE_INFINITE && probCutBeta > beta);

            MovePicker mp(pos, ttMove, probCutBeta - ss->staticEval, &captureHistory);

            while ((move = mp.next_move()) != Move::none()) {
                assert(move.is_ok());

                if (move == excludedMove || !pos.legal(move))
                    continue;

                assert(pos.capture_stage(move));

                ss->currentMove         = move;
                ss->continuationHistory = &continuationHistory[ss->inCheck][true][pos.moved_piece(move)][move.to()];

                nodes.fetch_add(1, std::memory_order_relaxed);
                pos.do_move(move);

                // Confirm with a quiescence search, then a reduced-depth search
                value = -qsearch<NonPV>(pos, ss + 1, -probCutBeta, -probCutBeta + 1);

                if (value >= probCutBeta)
                    value = -search<NonPV>(pos, ss + 1, -probCutBeta, -probCutBeta + 1, depth - 4, !cutNode);

                pos.undo_move(move);

                if (value >= probCutBeta) {
                    tte->save(posKey, value_to_tt(value, ss->ply), ss->ttPv, BOUND_LOWER, depth - 3, move,
                              ss->staticEval, tt.generation());
                    return std::abs(value) < VALUE_TB_WIN_IN_MAX_PLY ? value - (probCutBeta - beta) : value;
                }
            }
        }
    }

moves_loop:  // When in check, search starts here

    const PieceToHistory* contHist[] = {(ss - 1)->continuationHistory,
                                        (ss - 2)->continuationHistory,
                                        (ss - 3)->continuationHistory,
                                        (ss - 4)->continuationHistory,
                                        nullptr,
                                        (ss - 6)->continuationHistory};

    MovePicker mp(pos, ttMove, depth, &mainHistory, &captureHistory, contHist, ss->killers);

    value = bestValue;

    // Step 11. Loop through all pseudo-legal moves until no moves remain or a
    // beta cutoff occurs.
    while ((move = mp.next_move()) != Move::none()) {
        assert(move.is_ok());

        if (move == excludedMove)
            continue;

        // At the root, obey searchmoves and skip the lines already settled by MultiPV
        if (rootNode && !std::count(rootMoves.begin() + pvIdx, rootMoves.end(), move))
            continue;

        if (!rootNode && !pos.legal(move))
            continue;

        ss->moveCount = ++moveCount;

        if (rootNode && is_main_thread() && tm.elapsed() > 3000)
            sync_cout << "info depth " << depth << " currmove " << move_to_uci(move) << " currmovenumber "
                      << moveCount + pvIdx << sync_endl;

        if (PvNode)
            (ss + 1)->pv = nullptr;

        extension              = 0;
        capture                = pos.capture_stage(move);
        givesCheck             = pos.gives_check(move);
        const Piece movedPiece = pos.moved_piece(move);

        newDepth = depth - 1;

        const int   delta = beta - alpha;
        const Depth r     = reduction(improving, depth, moveCount, delta);

        // Step 12. Pruning at shallow depth
        if (!rootNode && pos.non_pawn_material(us) && bestValue > VALUE_TB_LOSS_IN_MAX_PLY) {
            // Late move pruning: skip quiet moves once enough have been tried
            if (moveCount >= futility_move_count(improving, depth))
                mp.skip_quiet_moves();

            // Depth the move would be searched at after reduction
            int lmrDepth = newDepth - r;

            if (capture || givesCheck) {
                const Piece capturedPiece = pos.piece_on(move.to());
                const int   captHist      = captureHistory[movedPiece][move.to()][type_of(capturedPiece)];

                // Futility pruning for captures
                if (!givesCheck && lmrDepth < 7 && !ss->inCheck) {
                    const Value futilityValue =
                        ss->staticEval + 285 + 277 * lmrDepth + PieceValue[capturedPiece] + captHist / 7;
                    if (futilityValue <= alpha)
                        continue;
                }

                // SEE pruning for losing captures
                const int seeHist = std::clamp(captHist / 33, -161 * depth, 156 * depth);
                if (!pos.see_ge(move, -168 * depth - seeHist))
                    continue;
            } else {
                int history = (*contHist[0])[movedPiece][move.to()] + (*contHist[1])[movedPiece][move.to()]
                            + (*contHist[3])[movedPiece][move.to()];

                // Continuation history pruning
                if (lmrDepth < 6 && history < -3884 * depth)
                    continue;

                history += 2 * mainHistory[us][move.from_to()];

                lmrDepth += history / 3609;

                // Futility pruning for quiet moves
                const Value futilityValue =
                    ss->staticEval + (bestValue < ss->staticEval - 54 ? 128 : 57) + 118 * lmrDepth;
                if (!ss->inCheck && lmrDepth < 15 && futilityValue <= alpha) {
                    if (bestValue <= futilityValue && std::abs(bestValue) < VALUE_TB_WIN_IN_MAX_PLY
                        && futilityValue < VALUE_TB_WIN_IN_MAX_PLY)
                        bestValue = futilityValue;
                    continue;
                }

                lmrDepth = std::max(lmrDepth, 0);

                // SEE pruning for quiet moves that hang material
                if (!pos.see_ge(move, -26 * lmrDepth * lmrDepth))
                    continue;
            }
        }

        // Step 13. Extensions
        // Singular extension: if the hash move is much better than every
        // alternative, extend it. If the alternatives also beat beta, cut.
        if (!rootNode && move == ttMove && !excludedMove && depth >= 4 - (completedDepth > 30) + ss->ttPv
            && std::abs(ttValue) < VALUE_TB_WIN_IN_MAX_PLY && (tte->bound() & BOUND_LOWER) && ttDepth >= depth - 3) {
            const Value singularBeta  = ttValue - (56 + 79 * (ss->ttPv && !PvNode)) * depth / 64;
            const Depth singularDepth = newDepth / 2;

            ss->excludedMove = move;
            value            = search<NonPV>(pos, ss, singularBeta - 1, singularBeta, singularDepth, cutNode);
            ss->excludedMove = Move::none();

            if (value < singularBeta) {
                extension = 1;

                // Extend twice when the alternatives are far worse
                if (!PvNode && value < singularBeta - 14 && ss->doubleExtensions < 15)
                    extension = 2 + (value < singularBeta - 150 && !ttCapture);
            }

            // Multi-cut: another move also fails high, so this node almost
            // certainly fails high too.
            else if (singularBeta >= beta)
                return singularBeta;

            // The hash move fails high but is not singular: reduce it.
            else if (ttValue >= beta)
                extension = -3;

            else if (cutNode)
                extension = -2;

            else if (ttValue <= value)
                extension = -1;
        }

        // Extend checks at shallow depth that do not lose material
        else if (givesCheck && depth > 9)
            extension = 1;

        newDepth += extension;
        ss->doubleExtensions = (ss - 1)->doubleExtensions + (extension >= 2);

        // Step 14. Make the move
        ss->currentMove         = move;
        ss->continuationHistory = &continuationHistory[ss->inCheck][capture][movedPiece][move.to()];

        const std::uint64_t nodeCount = rootNode ? nodes.load(std::memory_order_relaxed) : 0;

        nodes.fetch_add(1, std::memory_order_relaxed);
        pos.do_move(move, givesCheck);

        // Step 15. Late move reductions: later moves are searched shallower;
        // a reduced search that beats alpha is repeated at full depth.
        if (depth >= 2 && moveCount > 1 + rootNode) {
            Depth rr = r;

            if (ss->ttPv)
                rr -= 1 + (ttValue > alpha) + (ttDepth >= depth);

            if (cutNode)
                rr += 2 - (ttDepth >= depth && ss->ttPv);

            if (ttCapture)
                rr += 1;

            if ((ss + 1)->cutoffCnt > 3)
                rr += 1 + !(PvNode || cutNode);

            if (move == ttMove)
                rr -= 2;

            ss->statScore = 2 * mainHistory[us][move.from_to()] + (*contHist[0])[movedPiece][move.to()]
                          + (*contHist[1])[movedPiece][move.to()] + (*contHist[3])[movedPiece][move.to()] - 4723;

            rr -= ss->statScore / 13659;

            const Depth d = std::clamp(newDepth - rr, 1, newDepth + 1);

            value = -search<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, d, true);

            if (value > alpha && d < newDepth) {
                // Decide whether to go deeper or shallower in the re-search
                const bool doDeeper    = value > (bestValue + 47 + 2 * newDepth);
                const bool doShallower = value < bestValue + newDepth;

                newDepth += doDeeper - doShallower;

                if (newDepth > d)
                    value = -search<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, newDepth, !cutNode);

                // Update the continuation history with the outcome of the re-search
                const int bonus = value <= alpha ? -stat_malus(newDepth) : value >= beta ? stat_bonus(newDepth) : 0;
                update_continuation_histories(ss, movedPiece, move.to(), bonus);
            }
        }

        // Step 16. Full-depth search when LMR is skipped
        else if (!PvNode || moveCount > 1)
            value = -search<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, newDepth, !cutNode);

        // Step 17. The first move at a PV node and any move that beats alpha
        // get a full-window search.
        if (PvNode && (moveCount == 1 || value > alpha)) {
            (ss + 1)->pv    = pv;
            (ss + 1)->pv[0] = Move::none();

            value = -search<PV>(pos, ss + 1, -beta, -alpha, newDepth, false);
        }

        // Step 18. Undo the move
        pos.undo_move(move);

        assert(value > -VALUE_INFINITE && value < VALUE_INFINITE);

        // Step 19. Check for a new best move. Once the search is stopped the
        // values are unreliable and must not be used.
        if (threads.stop.load(std::memory_order_relaxed))
            return VALUE_ZERO;

        if (rootNode) {
            RootMove& rm = *std::find(rootMoves.begin(), rootMoves.end(), move);

            rm.effort += nodes.load(std::memory_order_relaxed) - nodeCount;

            rm.averageScore = rm.averageScore != -VALUE_INFINITE ? (2 * value + rm.averageScore) / 3 : value;

            // The PV move or a new best move: store its score and PV
            if (moveCount == 1 || value > alpha) {
                rm.score    = value;
                rm.selDepth = selDepth;
                rm.pv.resize(1);

                assert((ss + 1)->pv);

                for (Move* m = (ss + 1)->pv; *m != Move::none(); ++m)
                    rm.pv.push_back(*m);

                // Count best move changes for time management
                if (moveCount > 1 && !pvIdx)
                    ++bestMoveChanges;
            } else
                // Any other move keeps a sentinel score so that sorting leaves it behind
                rm.score = -VALUE_INFINITE;
        }

        if (value > bestValue) {
            bestValue = value;

            if (value > alpha) {
                bestMove = move;

                if (PvNode && !rootNode)
                    update_pv(ss->pv, move, (ss + 1)->pv);

                if (value >= beta) {
                    ss->cutoffCnt += 1 + !ttMove - (extension >= 2);
                    assert(value >= beta);
                    break;
                }

                // Reduce the remaining depth a little when alpha rises
                if (depth > 2 && depth < 14 && std::abs(value) < VALUE_TB_WIN_IN_MAX_PLY)
                    depth -= 2;

                assert(depth > 0);
                alpha = value;
            }
        }

        // Remember the searched moves for the history update
        if (move != bestMove && moveCount <= 32) {
            if (capture)
                capturesSearched[captureCount++] = move;
            else
                quietsSearched[quietCount++] = move;
        }
    }

    // Step 20. Check for mate and stalemate. With no legal moves the side to
    // move is mated if in check, otherwise it is a draw. In a singular search
    // the excluded move might have been the only one.
    assert(moveCount || !ss->inCheck || excludedMove || !MoveList<LEGAL>(pos).size());

    if (!moveCount)
        bestValue = excludedMove ? alpha : ss->inCheck ? mated_in(ss->ply) : VALUE_DRAW;

    // A best move was found: reward it and penalise the moves tried before it
    else if (bestMove)
        update_all_stats(pos, ss, bestMove, prevSq, quietsSearched, quietCount, capturesSearched, captureCount, depth);

    // No move beat alpha: the parent's move was a good refutation, reward it
    else if (!priorCapture && prevSq != SQ_NONE) {
        const int bonus = (depth > 5) + (PvNode || cutNode) + ((ss - 1)->statScore < -14963)
                        + ((ss - 1)->moveCount > 11) + (!ss->inCheck && bestValue <= ss->staticEval - 150);
        update_continuation_histories(ss - 1, pos.piece_on(prevSq), prevSq, stat_bonus(depth) * bonus);
        mainHistory[~us][(ss - 1)->currentMove.from_to()] << stat_bonus(depth) * bonus / 2;
    }

    if (PvNode)
        bestValue = std::min(bestValue, maxValue);

    // A fail low at a node whose parent was on the PV keeps the PV flag
    if (bestValue <= alpha)
        ss->ttPv = ss->ttPv || ((ss - 1)->ttPv && depth > 3);

    // Store the result. MultiPV lines after the first are not stored at the
    // root, since their scores are relative to the excluded moves.
    if (!excludedMove && !(rootNode && pvIdx))
        tte->save(posKey, value_to_tt(bestValue, ss->ply), ss->ttPv,
                  bestValue >= beta    ? BOUND_LOWER
                  : PvNode && bestMove ? BOUND_EXACT
                                       : BOUND_UPPER,
                  depth, bestMove, ss->staticEval, tt.generation());

    assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);

    return bestValue;
}

// Quiescence search: resolves captures (and check evasions) until the position
// is quiet enough for the static evaluation to be trusted.
template <NodeType nodeType>
Value Worker::qsearch(Position& pos, Stack* ss, Value alpha, Value beta) {
    static_assert(nodeType != Root);
    constexpr bool PvNode = nodeType == PV;

    assert(alpha >= -VALUE_INFINITE && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));

    Move     pv[MAX_PLY + 1];
    TTEntry* tte;
    Key      posKey;
    Move     ttMove, move, bestMove;
    Value    bestValue, value, ttValue, futilityValue, futilityBase;
    bool     pvHit, givesCheck, capture;
    int      moveCount;

    if (PvNode) {
        (ss + 1)->pv = pv;
        ss->pv[0]    = Move::none();
    }

    bestMove    = Move::none();
    ss->inCheck = bool(pos.checkers());
    moveCount   = 0;

    if (PvNode && selDepth < ss->ply + 1)
        selDepth = ss->ply + 1;

    // Immediate draw or maximum ply reached
    if (pos.is_draw(ss->ply) || ss->ply >= MAX_PLY)
        return (ss->ply >= MAX_PLY && !ss->inCheck) ? evaluate(pos) : VALUE_DRAW;

    assert(0 <= ss->ply && ss->ply < MAX_PLY);

    // Transposition table lookup
    posKey  = pos.key();
    tte     = tt.probe(posKey, ss->ttHit);
    ttValue = ss->ttHit ? value_from_tt(tte->value(), ss->ply, pos.rule50_count()) : VALUE_NONE;
    ttMove  = ss->ttHit ? tte->move() : Move::none();
    pvHit   = ss->ttHit && tte->is_pv();

    if (!PvNode && ss->ttHit && tte->depth() >= DEPTH_QS && ttValue != VALUE_NONE
        && (tte->bound() & (ttValue >= beta ? BOUND_LOWER : BOUND_UPPER)))
        return ttValue;

    // Static evaluation and stand pat
    if (ss->inCheck)
        bestValue = futilityBase = -VALUE_INFINITE;
    else {
        if (ss->ttHit) {
            if ((ss->staticEval = bestValue = tte->eval()) == VALUE_NONE)
                ss->staticEval = bestValue = evaluate(pos);

            if (ttValue != VALUE_NONE && (tte->bound() & (ttValue > bestValue ? BOUND_LOWER : BOUND_UPPER)))
                bestValue = ttValue;
        } else
            ss->staticEval = bestValue = evaluate(pos);

        // Stand pat: the side to move need not capture
        if (bestValue >= beta) {
            if (std::abs(bestValue) < VALUE_TB_WIN_IN_MAX_PLY)
                bestValue = (3 * bestValue + beta) / 4;
            if (!ss->ttHit)
                tte->save(posKey, value_to_tt(bestValue, ss->ply), false, BOUND_LOWER, DEPTH_UNSEARCHED, Move::none(),
                          ss->staticEval, tt.generation());
            return bestValue;
        }

        if (bestValue > alpha)
            alpha = bestValue;

        futilityBase = ss->staticEval + 226;
    }

    const PieceToHistory* contHist[] = {
        (ss - 1)->continuationHistory, (ss - 2)->continuationHistory, nullptr, nullptr, nullptr, nullptr};

    const Square prevSq = (ss - 1)->currentMove.is_ok() ? (ss - 1)->currentMove.to() : SQ_NONE;

    // Captures only, or all evasions when in check
    MovePicker mp(pos, ttMove, DEPTH_QS, &mainHistory, &captureHistory, contHist, nullptr);

    while ((move = mp.next_move()) != Move::none()) {
        assert(move.is_ok());

        if (!pos.legal(move))
            continue;

        givesCheck = pos.gives_check(move);
        capture    = pos.capture_stage(move);

        moveCount++;

        // Pruning
        if (bestValue > VALUE_TB_LOSS_IN_MAX_PLY && pos.non_pawn_material(pos.side_to_move())) {
            // Futility pruning and move count pruning
            if (!givesCheck && move.to() != prevSq && futilityBase > VALUE_TB_LOSS_IN_MAX_PLY
                && move.type() != PROMOTION) {
                if (moveCount > 2)
                    continue;

                futilityValue = futilityBase + PieceValue[pos.piece_on(move.to())];

                // The capture cannot raise alpha even if it wins the piece for free
                if (futilityValue <= alpha) {
                    bestValue = std::max(bestValue, futilityValue);
                    continue;
                }

                // The capture is not winning and cannot raise alpha either
                if (futilityBase <= alpha && !pos.see_ge(move, 1)) {
                    bestValue = std::max(bestValue, futilityBase);
                    continue;
                }

                // Even a free capture does not raise alpha
                if (futilityBase > alpha && !pos.see_ge(move, (alpha - futilityBase) * 4)) {
                    bestValue = alpha;
                    continue;
                }
            }

            // Prune quiet check evasions with bad continuation history
            if (!capture
                && (*contHist[0])[pos.moved_piece(move)][move.to()] + (*contHist[1])[pos.moved_piece(move)][move.to()]
                       <= 5095)
                continue;

            // Losing captures
            if (!pos.see_ge(move, -83))
                continue;
        }

        ss->currentMove         = move;
        ss->continuationHistory = &continuationHistory[ss->inCheck][capture][pos.moved_piece(move)][move.to()];

        nodes.fetch_add(1, std::memory_order_relaxed);
        pos.do_move(move, givesCheck);
        value = -qsearch<nodeType>(pos, ss + 1, -beta, -alpha);
        pos.undo_move(move);

        assert(value > -VALUE_INFINITE && value < VALUE_INFINITE);

        if (value > bestValue) {
            bestValue = value;

            if (value > alpha) {
                bestMove = move;

                if (PvNode)
                    update_pv(ss->pv, move, (ss + 1)->pv);

                if (value < beta)
                    alpha = value;
                else
                    break;  // fail high
            }
        }
    }

    // No legal evasions: checkmate
    if (ss->inCheck && bestValue == -VALUE_INFINITE) {
        assert(!MoveList<LEGAL>(pos).size());
        return mated_in(ss->ply);
    }

    if (std::abs(bestValue) < VALUE_TB_WIN_IN_MAX_PLY && bestValue >= beta)
        bestValue = (3 * bestValue + beta) / 4;

    tte->save(posKey, value_to_tt(bestValue, ss->ply), pvHit, bestValue >= beta ? BOUND_LOWER : BOUND_UPPER, DEPTH_QS,
              bestMove, ss->staticEval, tt.generation());

    assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);

    return bestValue;
}

// Called from the main thread every so many nodes to enforce the time, node
// and move-time limits.
void Worker::check_time() {
    if (--callsCnt > 0)
        return;

    // Check more often when a node limit is set so that it is not overshot much
    callsCnt = limits.nodes ? std::min(512, int(limits.nodes / 1024)) : 512;

    if (threads.stop)
        return;

    const TimePoint elapsed = tm.elapsed();

    // The first iteration always completes so that a best move exists
    if (completedDepth < 1 && !(limits.nodes && threads.nodes_searched() >= std::uint64_t(limits.nodes)))
        return;

    if ((limits.use_time_management() && elapsed > tm.maximum()) || (limits.movetime && elapsed >= limits.movetime)
        || (limits.nodes && threads.nodes_searched() >= std::uint64_t(limits.nodes)))
        threads.stop = threads.abortedSearch = true;
}

// Updates the history tables after a beta cutoff or a new best move: the best
// move is rewarded, the moves tried before it are penalised.
void Worker::update_all_stats(const Position& pos,
                              Stack*          ss,
                              Move            bestMove,
                              Square          prevSq,
                              Move*           quietsSearched,
                              int             quietCount,
                              Move*           capturesSearched,
                              int             captureCount,
                              Depth           depth) {
    const Piece movedPiece = pos.moved_piece(bestMove);
    const int   bonus      = stat_bonus(depth);
    const int   malus      = stat_malus(depth);

    if (!pos.capture_stage(bestMove)) {
        update_quiet_histories(pos, ss, bestMove, bonus);

        for (int i = 0; i < quietCount; ++i)
            update_quiet_histories(pos, ss, quietsSearched[i], -malus);
    } else {
        const PieceType captured = type_of(pos.piece_on(bestMove.to()));
        captureHistory[movedPiece][bestMove.to()][captured] << bonus;
    }

    // Extra penalty for a quiet hash move at the parent that got refuted early
    if (prevSq != SQ_NONE && ((ss - 1)->moveCount == 1 + (ss - 1)->ttHit) && !pos.captured_piece())
        update_continuation_histories(ss - 1, pos.piece_on(prevSq), prevSq, -malus);

    for (int i = 0; i < captureCount; ++i) {
        const Piece     pc       = pos.moved_piece(capturesSearched[i]);
        const PieceType captured = type_of(pos.piece_on(capturesSearched[i].to()));
        captureHistory[pc][capturesSearched[i].to()][captured] << -malus;
    }
}

void Worker::update_quiet_histories(const Position& pos, Stack* ss, Move move, int bonus) {
    const Color us = pos.side_to_move();

    // Killer moves: the two most recent cutoff moves at this ply
    if (bonus > 0 && ss->killers[0] != move) {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = move;
    }

    mainHistory[us][move.from_to()] << bonus;
    update_continuation_histories(ss, pos.moved_piece(move), move.to(), bonus);
}

// Updates the continuation histories of the moves one, two, three, four and
// six plies back.
void Worker::update_continuation_histories(Stack* ss, Piece pc, Square to, int bonus) {
    for (int i : {1, 2, 3, 4, 6}) {
        // Only the two most recent plies matter when in check
        if (ss->inCheck && i > 2)
            break;
        if ((ss - i)->currentMove.is_ok())
            (*(ss - i)->continuationHistory)[pc][to] << bonus / (1 + (i == 3));
    }
}

// Formats the UCI "info" line(s) for the current iteration.
std::string Worker::pv_info(Depth depth, Value alpha, Value beta) const {
    std::ostringstream  ss;
    const TimePoint     elapsed  = tm.elapsed() + 1;
    const std::uint64_t nodesAll = threads.nodes_searched();
    const std::size_t   multiPV  = std::min<std::size_t>(std::size_t(options.multiPV), rootMoves.size());
    bool                first    = true;

    for (std::size_t i = 0; i < multiPV; ++i) {
        const bool updated = rootMoves[i].score != -VALUE_INFINITE;

        if (depth == 1 && !updated && i > 0)
            continue;

        const Depth d = updated ? depth : std::max(1, depth - 1);
        Value       v = updated ? rootMoves[i].score : rootMoves[i].previousScore;

        if (v == -VALUE_INFINITE)
            v = VALUE_ZERO;

        if (!first)
            ss << "\n";
        first = false;

        ss << "info depth " << d << " seldepth " << rootMoves[i].selDepth << " multipv " << i + 1 << " score "
           << score_to_uci(v);

        if (i == pvIdx && updated)
            ss << (v >= beta ? " lowerbound" : v <= alpha ? " upperbound" : "");

        ss << " nodes " << nodesAll << " nps " << nodesAll * 1000 / std::uint64_t(elapsed) << " hashfull "
           << tt.hashfull() << " time " << elapsed << " pv";

        for (Move m : rootMoves[i].pv)
            ss << " " << move_to_uci(m);
    }

    return ss.str();
}

}  // namespace sp
