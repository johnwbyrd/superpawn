// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "movepick.h"

#include <algorithm>
#include <limits>

#include "tt.h"

namespace sp {

namespace {

enum Stages {
    // Main search when not in check
    MAIN_TT,
    CAPTURE_INIT,
    GOOD_CAPTURE,
    QUIET_INIT,
    GOOD_QUIET,
    BAD_CAPTURE,
    BAD_QUIET,

    // In check, main and quiescence search
    EVASION_TT,
    EVASION_INIT,
    EVASION,

    // ProbCut
    PROBCUT_TT,
    PROBCUT_INIT,
    PROBCUT,

    // Quiescence search when not in check
    QSEARCH_TT,
    QCAPTURE_INIT,
    QCAPTURE
};

// Sorts the moves with value >= limit to the front in descending order and
// leaves the rest unsorted. Cheaper than a full sort when most moves are never
// reached.
void partial_insertion_sort(ExtMove* begin, ExtMove* end, int limit) {
    for (ExtMove *sortedEnd = begin, *p = begin + 1; p < end; ++p)
        if (p->value >= limit) {
            ExtMove tmp = *p, *q;
            *p          = *++sortedEnd;
            for (q = sortedEnd; q != begin && *(q - 1) < tmp; --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

}  // namespace

MovePicker::MovePicker(const Position&              p,
                       Move                         ttm,
                       Depth                        d,
                       const ButterflyHistory*      mh,
                       const CapturePieceToHistory* cph,
                       const PieceToHistory**       ch,
                       const Move*                  k) :
    pos(p),
    mainHistory(mh),
    captureHistory(cph),
    continuationHistory(ch),
    ttMove(ttm),
    killers{k ? k[0] : Move::none(), k ? k[1] : Move::none()},
    depth(d) {

    if (pos.checkers())
        stage = EVASION_TT + !(ttm && pos.pseudo_legal(ttm));
    else
        stage = (depth > 0 ? MAIN_TT : QSEARCH_TT) + !(ttm && pos.pseudo_legal(ttm));
}

MovePicker::MovePicker(const Position& p, Move ttm, int th, const CapturePieceToHistory* cph) :
    pos(p),
    mainHistory(nullptr),
    captureHistory(cph),
    continuationHistory(nullptr),
    ttMove(ttm),
    killers{Move::none(), Move::none()},
    threshold(th),
    depth(DEPTH_QS) {
    assert(!pos.checkers());

    stage = PROBCUT_TT + !(ttm && pos.capture_stage(ttm) && pos.pseudo_legal(ttm) && pos.see_ge(ttm, threshold));
}

// Assigns a numerical value to each move in the list. Captures are ordered by
// the value of the captured piece and the capture history, quiets by the
// history tables plus a bonus for killer moves.
template <GenType Type>
void MovePicker::score() {
    static_assert(Type == CAPTURES || Type == QUIETS || Type == EVASIONS, "Wrong type");

    for (auto& m : *this) {
        const Piece     pc       = pos.moved_piece(m);
        const Square    to       = m.to();
        const PieceType captured = m.type() == EN_PASSANT ? PAWN : type_of(pos.piece_on(to));

        if constexpr (Type == CAPTURES)
            m.value = 7 * PieceValue[captured] + (*captureHistory)[pc][to][captured]
                    + (m.type() == PROMOTION ? PieceValue[m.promotion_type()] : 0);

        else if constexpr (Type == QUIETS) {
            m.value = 2 * (*mainHistory)[pos.side_to_move()][m.from_to()];
            m.value += (*continuationHistory[0])[pc][to];
            m.value += (*continuationHistory[1])[pc][to];
            m.value += (*continuationHistory[3])[pc][to];

            if (m == killers[0])
                m.value += 1 << 15;
            else if (m == killers[1])
                m.value += 1 << 14;
        }

        else  // EVASIONS: captures first, by captured value, then by history
        {
            if (pos.capture_stage(m))
                m.value = PieceValue[captured] + (1 << 28);
            else
                m.value = (*mainHistory)[pos.side_to_move()][m.from_to()] + (*continuationHistory[0])[pc][to];
        }
    }
}

// Returns the next move satisfying the filter, or Move::none() when the current
// stage is exhausted. The hash move is never returned twice.
template <typename Pred>
Move MovePicker::select(Pred filter) {
    for (; cur < endMoves; ++cur)
        if (*cur != ttMove && filter())
            return *cur++;

    return Move::none();
}

Move MovePicker::next_move() {
    auto quiet_threshold = [](Depth d) { return -3560 * d; };

top:
    switch (stage) {

    case MAIN_TT :
    case EVASION_TT :
    case QSEARCH_TT :
    case PROBCUT_TT :
        ++stage;
        return ttMove;

    case CAPTURE_INIT :
    case PROBCUT_INIT :
    case QCAPTURE_INIT :
        cur = endBadCaptures = moves;
        endMoves             = generate<CAPTURES>(pos, cur);

        score<CAPTURES>();
        partial_insertion_sort(cur, endMoves, std::numeric_limits<int>::min());
        ++stage;
        goto top;

    case GOOD_CAPTURE :
        if (select([&]() {
                // Captures that lose material are tried after the quiets.
                return pos.see_ge(*cur, -cur->value / 18) ? true : (*endBadCaptures++ = *cur, false);
            }))
            return *(cur - 1);

        ++stage;
        [[fallthrough]];

    case QUIET_INIT :
        if (!skipQuiets) {
            cur      = endBadCaptures;
            endMoves = beginBadQuiets = endBadQuiets = generate<QUIETS>(pos, cur);

            score<QUIETS>();
            // Quiets above the threshold are sorted to the front; the rest
            // stay behind them in generation order.
            partial_insertion_sort(cur, endMoves, quiet_threshold(depth));
        }

        ++stage;
        [[fallthrough]];

    case GOOD_QUIET :
        if (!skipQuiets && select([]() { return true; })) {
            if ((cur - 1)->value > quiet_threshold(depth))
                return *(cur - 1);

            // The first quiet below the threshold marks the start of the bad
            // quiets, which are tried after the bad captures.
            beginBadQuiets = cur - 1;
        }

        // Now the bad captures, which were stored at the start of the list.
        cur      = moves;
        endMoves = endBadCaptures;

        ++stage;
        [[fallthrough]];

    case BAD_CAPTURE :
        if (select([]() { return true; }))
            return *(cur - 1);

        // Finally the quiets that fell below the history threshold.
        cur      = beginBadQuiets;
        endMoves = endBadQuiets;

        ++stage;
        [[fallthrough]];

    case BAD_QUIET :
        if (!skipQuiets)
            return select([]() { return true; });

        return Move::none();

    case EVASION_INIT :
        cur      = moves;
        endMoves = generate<EVASIONS>(pos, cur);

        score<EVASIONS>();
        partial_insertion_sort(cur, endMoves, std::numeric_limits<int>::min());
        ++stage;
        [[fallthrough]];

    case EVASION :
        return select([]() { return true; });

    case PROBCUT :
        return select([&]() { return pos.see_ge(*cur, threshold); });

    case QCAPTURE :
        return select([]() { return true; });
    }

    assert(false);
    return Move::none();
}

}  // namespace sp
