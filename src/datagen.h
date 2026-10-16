// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <cstdint>
#include <string>

#include "types.h"

namespace sp {

class Position;

// Self-play data generation for NNUE training.
//
// Games start with a few random moves, then both sides play fixed-node
// searches. Every quiet, non-check position is written together with the
// search score and, once the game is over, its result, in the bulletformat
// ChessBoard layout that the trainer in tools/train reads directly.
namespace datagen {

struct Options {
    std::string   output;  // file to append to
    int           threads       = 1;
    std::int64_t  nodes         = 5000;  // search nodes per move
    std::uint64_t positions     = 100'000'000;
    std::uint64_t games         = 0;    // 0 means until `positions` is reached
    std::uint64_t seed          = 0;    // 0 means from the clock
    int           randomPlies   = 8;    // random opening moves (plus one on odd games)
    int           maxPly        = 400;  // longer games are drawn
    int           hashMb        = 16;
    Value         openingLimit  = 1000;  // discard openings scored beyond this
    Value         adjudicateWin = 2000;  // score for 4 plies that decides a game
};

void run(const Options& options);

// One position in the bulletformat ChessBoard layout: 32 bytes.
struct BulletBoard {
    std::uint64_t occ;
    std::uint8_t  pcs[16];
    std::int16_t  score;
    std::uint8_t  result;
    std::uint8_t  ksq;
    std::uint8_t  oppKsq;
    std::uint8_t  extra[3];
};

static_assert(sizeof(BulletBoard) == 32, "BulletBoard must be 32 bytes");

// Packs a position with the search score from the side to move's view. The
// result is filled in later from White's view (0 loss, 1 draw, 2 win).
BulletBoard pack(const Position& pos, Value scoreStm);
void        set_result(BulletBoard& b, Color stm, int whiteResult2);

}  // namespace datagen
}  // namespace sp
