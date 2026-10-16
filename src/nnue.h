// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "types.h"

namespace sp {

class Position;

// NNUE: an efficiently updatable neural network evaluation.
//
// The network is a perspective network. Each side sees the board from its own
// point of view: 768 binary inputs, one per (piece colour relative to the
// viewer, piece type, square flipped for Black), feed a layer of HIDDEN
// neurons. Both perspectives' hidden vectors, side to move first, go through
// a squared clipped ReLU into a single output neuron chosen by material.
//
// The first layer dominates the cost and is kept incrementally: a move changes
// at most a handful of inputs, so its accumulator is the parent's plus and
// minus a few weight rows.
namespace nnue {

constexpr int INPUTS         = 768;
constexpr int HIDDEN         = 1024;
constexpr int OUTPUT_BUCKETS = 8;

// Quantisation: first layer weights and biases are scaled by QA, output
// weights by QB. Output weights must fit in |w| < 128 so that the clipped
// activation times a weight fits in 16 bits.
constexpr int QA    = 255;
constexpr int QB    = 64;
constexpr int SCALE = 400;

// The hidden layer sums for both perspectives at one position.
struct alignas(64) Accumulator {
    std::int16_t v[COLOR_NB][HIDDEN];
    bool         computed;
};

// The inputs a move changed. A plain move is one entry with both squares,
// a capture adds an entry with only `from`, a promotion replaces the pawn by
// an entry with only `to`, castling moves two pieces.
struct DirtyPiece {
    int    count = 0;
    Piece  piece[3];
    Square from[3];
    Square to[3];

    void move(Piece pc, Square f, Square t) {
        piece[count] = pc, from[count] = f, to[count] = t;
        ++count;
    }
    void remove(Piece pc, Square f) { move(pc, f, SQ_NONE); }
    void add(Piece pc, Square t) { move(pc, SQ_NONE, t); }
};

// Weights as stored in the file, aligned for SIMD loads.
struct alignas(64) Network {
    std::int16_t featureWeights[INPUTS * HIDDEN];
    std::int16_t featureBias[HIDDEN];
    std::int16_t outputWeights[OUTPUT_BUCKETS][2 * HIDDEN];
    std::int16_t outputBias[OUTPUT_BUCKETS];

    std::string name;  // file name or "<embedded>"

    // Readers. Both return nullptr and set `error` on failure.
    static std::unique_ptr<Network> load(const std::string& path, std::string& error);
    static std::unique_ptr<Network> load_embedded(std::string& error);
    static std::unique_ptr<Network> from_bytes(const std::uint8_t* data, std::size_t size, std::string& error);

    // Writes the network in the file format; used by the tests and tools.
    bool save(const std::string& path) const;
};

// The file format, little-endian:
//
//   char     magic[4]   "SPNN"
//   uint32   version    1
//   uint32   inputs     768
//   uint32   hidden     HIDDEN
//   uint32   buckets    OUTPUT_BUCKETS
//   uint32   qa, qb, scale
//   uint32   reserved   0
//   int16    featureWeights[INPUTS][HIDDEN]
//   int16    featureBias[HIDDEN]
//   int16    outputWeights[OUTPUT_BUCKETS][2 * HIDDEN]   side to move half first
//   int16    outputBias[OUTPUT_BUCKETS]
//   uint64   checksum   FNV-1a of every weight byte above
constexpr std::uint32_t FILE_VERSION = 1;
constexpr std::size_t   HEADER_SIZE  = 4 + 8 * 4;
constexpr std::size_t   WEIGHTS_SIZE =
    sizeof(std::int16_t)
    * (std::size_t(INPUTS) * HIDDEN + HIDDEN + std::size_t(OUTPUT_BUCKETS) * 2 * HIDDEN + OUTPUT_BUCKETS);
constexpr std::size_t FILE_SIZE = HEADER_SIZE + WEIGHTS_SIZE + 8;

// Index of the input for piece `pc` on `sq` as seen by `perspective`.
inline int feature_index(Color perspective, Piece pc, Square sq) {
    const int relativeColour = color_of(pc) != perspective;
    const int relativeSquare = perspective == WHITE ? sq : flip_rank(sq);
    return (relativeColour * 6 + (type_of(pc) - PAWN)) * SQUARE_NB + relativeSquare;
}

// Which output neuron a position uses: more pieces, higher bucket.
inline int output_bucket(int pieceCount) {
    constexpr int divisor = (32 + OUTPUT_BUCKETS - 1) / OUTPUT_BUCKETS;
    return (pieceCount - 2) / divisor;
}

// Computes the accumulator for a position from scratch.
void refresh(const Network& net, const Position& pos, Accumulator& acc);

// Computes `acc` from `prev` by applying the changes in `dp`.
void update(const Network& net, const Accumulator& prev, Accumulator& acc, const DirtyPiece& dp);

// Makes sure the position's current accumulator is computed, updating from the
// nearest computed ancestor or refreshing, and returns it.
const Accumulator& accumulator(const Network& net, const Position& pos);

// Evaluation in centipawns from the side to move's point of view.
Value evaluate(const Network& net, const Position& pos);

// Reference implementation of the output layer without SIMD, for the tests.
Value evaluate_scalar(const Network& net, const Accumulator& acc, Color stm, int bucket);

// Name of the SIMD path compiled in: "AVX-512", "AVX2" or "scalar".
const char* simd_name();

}  // namespace nnue
}  // namespace sp
