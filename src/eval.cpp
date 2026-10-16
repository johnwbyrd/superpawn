// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

// Evaluation: the NNUE network when one is loaded, otherwise a placeholder
// hand-crafted evaluation of tapered material and piece-square tables.

#include "eval.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>

#include "bitboard.h"
#include "position.h"

namespace sp {

namespace {

std::unique_ptr<nnue::Network> theNetwork;

Value evaluate_hce(const Position& pos);

struct Score {
    int              mg, eg;
    constexpr Score  operator+(Score o) const { return {mg + o.mg, eg + o.eg}; }
    constexpr Score  operator-(Score o) const { return {mg - o.mg, eg - o.eg}; }
    constexpr Score  operator-() const { return {-mg, -eg}; }
    constexpr Score& operator+=(Score o) {
        mg += o.mg, eg += o.eg;
        return *this;
    }
    constexpr Score& operator-=(Score o) {
        mg -= o.mg, eg -= o.eg;
        return *this;
    }
};

constexpr Score Material[PIECE_TYPE_NB] = {{0, 0},     {82, 94},    {337, 281}, {365, 297},
                                           {477, 512}, {1025, 936}, {0, 0},     {0, 0}};

constexpr Score BishopPair = {30, 50};
constexpr Score Tempo      = {12, 6};

// Game phase: 24 at the start, 0 with only pawns and kings left.
constexpr int PhaseWeight[PIECE_TYPE_NB] = {0, 0, 1, 1, 2, 4, 0, 0};
constexpr int MaxPhase                   = 24;

// Piece-square bonuses indexed by piece and square. Black's table is White's
// with the ranks flipped; the sign is applied when summing.
Score Psq[PIECE_NB][SQUARE_NB];

// Distance of a square from the centre, 0 for d4/d5/e4/e5 up to 3 on the rim.
int centre_distance(Square s) {
    const int f = file_of(s), r = rank_of(s);
    return std::max(std::abs(2 * f - 7), std::abs(2 * r - 7)) / 2;
}

Score psq_bonus(PieceType pt, Square s) {
    const int cd = centre_distance(s);
    const int r  = rank_of(s);
    const int f  = file_of(s);

    switch (pt) {
    case PAWN : {
        // Central pawns forward in the middlegame, every pawn forward in the endgame.
        const int advance = r - RANK_2;
        const int central = (f == FILE_D || f == FILE_E) ? 6 : (f == FILE_C || f == FILE_F) ? 2 : 0;
        return {advance * 3 + (advance > 0 ? central : 0), advance * advance * 4};
    }
    case KNIGHT :
        return {28 - 14 * cd, 24 - 10 * cd};
    case BISHOP :
        return {16 - 7 * cd + (r == RANK_1 ? -8 : 0), 14 - 5 * cd};
    case ROOK :
        return {(r == RANK_7 ? 14 : 0) + (f == FILE_D || f == FILE_E ? 4 : 0), r == RANK_7 ? 6 : 0};
    case QUEEN :
        return {-4 * cd + (r == RANK_1 ? 4 : 0), 12 - 5 * cd};
    case KING : {
        // Stay home and tucked away in the middlegame, centralise in the endgame.
        const int home = r == RANK_1 ? 12 : -18 * r;
        const int side = (f == FILE_G || f == FILE_B) ? 12 : (f == FILE_H || f == FILE_A || f == FILE_C) ? 4 : -8;
        return {home + side, 24 - 12 * cd};
    }
    default :
        return {0, 0};
    }
}

}  // namespace

const nnue::Network* Eval::network() {
    return theNetwork.get();
}

std::string Eval::set_network(const std::string& path) {
    std::string error;
    auto        net =
        path.empty() || path == "<embedded>" ? nnue::Network::load_embedded(error) : nnue::Network::load(path, error);
    if (!net)
        return "info string NNUE network not loaded: " + error;

    theNetwork = std::move(net);
    return "info string NNUE evaluation using " + theNetwork->name + " (" + std::to_string(nnue::INPUTS) + "x"
         + std::to_string(nnue::HIDDEN) + "x2 -> " + std::to_string(nnue::OUTPUT_BUCKETS) + ", " + nnue::simd_name()
         + ")";
}

std::string Eval::info() {
    if (theNetwork)
        return "info string NNUE evaluation using " + theNetwork->name + " (" + nnue::simd_name() + ")";
    return "info string No NNUE network available, using the hand-crafted evaluation";
}

void Eval::init() {
    std::string error;
    theNetwork = nnue::Network::load_embedded(error);

    for (PieceType pt = PAWN; pt <= KING; ++pt)
        for (Square s = SQ_A1; s <= SQ_H8; ++s) {
            const Score bonus                        = psq_bonus(pt, s);
            Psq[make_piece(WHITE, pt)][s]            = bonus;
            Psq[make_piece(BLACK, pt)][flip_rank(s)] = bonus;
        }
}

Value Eval::evaluate(const Position& pos) {
    if (theNetwork)
        return nnue::evaluate(*theNetwork, pos);
    return evaluate_hce(pos);
}

namespace {

Value evaluate_hce(const Position& pos) {
    Score score = {0, 0};
    int   phase = 0;

    for (Bitboard b = pos.pieces(); b;) {
        const Square    s  = pop_lsb(b);
        const Piece     pc = pos.piece_on(s);
        const PieceType pt = type_of(pc);
        const Score     v  = Material[pt] + Psq[pc][s];

        score += color_of(pc) == WHITE ? v : -v;
        phase += PhaseWeight[pt];
    }

    if (pos.count<BISHOP>(WHITE) >= 2)
        score += BishopPair;
    if (pos.count<BISHOP>(BLACK) >= 2)
        score -= BishopPair;

    score += pos.side_to_move() == WHITE ? Tempo : -Tempo;

    phase         = std::min(phase, MaxPhase);
    const Value v = (score.mg * phase + score.eg * (MaxPhase - phase)) / MaxPhase;

    return pos.side_to_move() == WHITE ? v : -v;
}

}  // namespace

std::string Eval::trace(const Position& pos) {
    std::ostringstream ss;
    const Value        hce = evaluate_hce(pos);
    ss << "Hand-crafted evaluation (side to move): " << hce << " cp\n";
    if (theNetwork)
        ss << "NNUE evaluation (side to move): " << nnue::evaluate(*theNetwork, pos) << " cp  [" << theNetwork->name
           << "]\n";
    const Value v = evaluate(pos);
    ss << "Final evaluation (side to move): " << v << " cp\n"
       << "Final evaluation (white): " << (pos.side_to_move() == WHITE ? v : -v) << " cp\n";
    return ss.str();
}

}  // namespace sp
