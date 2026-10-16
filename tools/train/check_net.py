#!/usr/bin/env python3
# Superpawn, a UCI chess engine.
# Copyright (C) 2026 John Byrd and the Superpawn contributors.
# Licensed under the GNU Affero General Public License v3.0 or later.
#
# Independent reference evaluation of a Superpawn .nnue network, written
# from bullet's definition of the architecture rather than from the engine's
# code. Use it to check that the engine's `eval` agrees with the network:
#
#   python3 tools/train/check_net.py nets/default.nnue
#   python3 tools/train/check_net.py nets/default.nnue "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#
# Without a FEN it prints weight statistics and evaluates a few positions.

import struct, sys
from array import array

INPUTS, HIDDEN, BUCKETS = 768, 1024, 8
QA, QB, SCALE = 255, 64, 400

def load(path):
    data = open(path, "rb").read()
    magic, version, inputs, hidden, buckets, qa, qb, scale, _ = struct.unpack_from("<4s8I", data, 0)
    assert magic == b"SPNN", "not a Superpawn network"
    assert (inputs, hidden, buckets, qa, qb, scale) == (INPUTS, HIDDEN, BUCKETS, QA, QB, SCALE), \
        f"architecture {inputs}x{hidden}x2 -> {buckets}, q {qa}/{qb}/{scale} does not match this script"
    off = 36
    def take(n):
        nonlocal off
        a = array("h"); a.frombytes(data[off:off + 2 * n]); off += 2 * n
        if sys.byteorder == "big": a.byteswap()
        return a
    l0w = take(INPUTS * HIDDEN)
    l0b = take(HIDDEN)
    l1w = take(BUCKETS * 2 * HIDDEN)
    l1b = take(BUCKETS)
    return l0w, l0b, l1w, l1b

def parse_fen(fen):
    parts = fen.split()
    board = {}
    rank, file = 7, 0
    for ch in parts[0]:
        if ch == "/": rank -= 1; file = 0
        elif ch.isdigit(): file += int(ch)
        else:
            board[rank * 8 + file] = ch
            file += 1
    return board, 0 if parts[1] == "w" else 1

PIECE_TYPES = "PNBRQK"

def features(board, perspective):
    # bullet Chess768: [0, 384][piece belongs to the other side] + 64 * type + square,
    # square flipped vertically for the black perspective
    out = []
    for sq, ch in board.items():
        colour = 0 if ch.isupper() else 1
        ptype = PIECE_TYPES.index(ch.upper())
        rel_colour = colour != perspective
        rel_sq = sq if perspective == 0 else sq ^ 56
        out.append(384 * rel_colour + 64 * ptype + rel_sq)
    return out

def evaluate(net, fen):
    l0w, l0b, l1w, l1b = net
    board, stm = parse_fen(fen)
    acc = {}
    for persp in (0, 1):
        a = list(l0b)
        for f in features(board, persp):
            base = f * HIDDEN
            for i in range(HIDDEN):
                a[i] += l0w[base + i]
        acc[persp] = a
    bucket = (len(board) - 2) // ((32 + BUCKETS - 1) // BUCKETS)
    w = l1w[bucket * 2 * HIDDEN:(bucket + 1) * 2 * HIDDEN]
    total = 0
    for i in range(HIDDEN):
        us = min(max(acc[stm][i], 0), QA)
        them = min(max(acc[1 - stm][i], 0), QA)
        total += us * us * w[i] + them * them * w[HIDDEN + i]
    # bullet: output /= QA; output += bias; output *= SCALE; output /= QA * QB  (integer steps)
    out = total // QA if total >= 0 else -((-total) // QA)
    out += l1b[bucket]
    out = out * SCALE
    out = out // (QA * QB) if out >= 0 else -((-out) // (QA * QB))
    return out, bucket, acc

def stats(net):
    l0w, l0b, l1w, l1b = net
    def describe(name, a):
        mn, mx = min(a), max(a)
        zeros = sum(1 for v in a if v == 0)
        print(f"  {name:4s} n={len(a):8d} min={mn:7d} max={mx:7d} zero={100.0 * zeros / len(a):5.1f}%")
    print("weights:")
    describe("l0w", l0w); describe("l0b", l0b); describe("l1w", l1w); describe("l1b", l1b)

def main():
    if len(sys.argv) < 2:
        print(__doc__ or "usage: check_net.py <net.nnue> [fen]"); sys.exit(2)
    net = load(sys.argv[1])
    fens = sys.argv[2:] or [
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
        "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1",
        "4k3/8/8/8/8/8/8/R3K3 b Q - 0 1",
        "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
        "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1",
    ]
    if len(sys.argv) < 3:
        stats(net)
    for fen in fens:
        out, bucket, acc = evaluate(net, fen)
        sat = sum(1 for p in (0, 1) for v in acc[p] if v >= QA) / (2 * HIDDEN)
        neg = sum(1 for p in (0, 1) for v in acc[p] if v <= 0) / (2 * HIDDEN)
        print(f"{out:6d} cp  bucket {bucket}  saturated {100 * sat:4.1f}%  inactive {100 * neg:4.1f}%  {fen}")

if __name__ == "__main__":
    main()
