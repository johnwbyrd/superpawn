#!/usr/bin/env bash
# Perft tests: run the engine's perft command on standard positions
# and check leaf-node counts against known-correct values.
#
# Usage: perft.sh [path/to/superpawn]
#
# Each case is "LABEL|FEN|DEPTH|EXPECTED". Expected values are cross-
# checked against stockfish; the FENs are the six standard positions
# from chessprogramming.org/Perft_Results.

set -u

BINARY="${1:-./superpawn}"

if [ ! -x "$BINARY" ]; then
    echo "error: $BINARY is not executable" >&2
    exit 2
fi

CASES=(
    # startpos
    "startpos|startpos|1|20"
    "startpos|startpos|2|400"
    "startpos|startpos|3|8902"
    "startpos|startpos|4|197281"

    # Kiwipete -- tests all castling + promotions + checks
    "kiwipete|r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|1|48"
    "kiwipete|r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|2|2039"
    "kiwipete|r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|3|97862"

    # Position 3 -- endgame with en-passant edge cases
    "position3|8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|1|14"
    "position3|8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|2|191"
    "position3|8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|3|2812"
    "position3|8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|4|43238"

    # Position 4 -- king in check, blocks, promotions, under-promotions
    "position4|r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2pP/R2Q1RK1 w kq - 0 1|1|6"
    "position4|r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2pP/R2Q1RK1 w kq - 0 1|2|280"
    "position4|r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2pP/R2Q1RK1 w kq - 0 1|3|9346"

    # Position 5 -- castling + promotion-captures
    "position5|rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|1|44"
    "position5|rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|2|1486"
    "position5|rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|3|62379"

    # Position 6 -- quiet middlegame
    "position6|r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|1|46"
    "position6|r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|2|2079"
    "position6|r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|3|89890"
)

run_perft() {
    local fen="$1" depth="$2"
    local pos_cmd
    if [ "$fen" = "startpos" ]; then
        pos_cmd="position startpos"
    else
        pos_cmd="position fen $fen"
    fi
    printf 'uci\n%s\nperft %s\nquit\n' "$pos_cmd" "$depth" \
        | "$BINARY" 2>/dev/null \
        | awk -v d="$depth" '$0 ~ "^perft " d ":" { print $3; exit }'
}

fails=0
passes=0

for case in "${CASES[@]}"; do
    IFS='|' read -r label fen depth expected <<< "$case"
    actual=$(run_perft "$fen" "$depth")

    if [ -z "$actual" ]; then
        printf '  ERROR  %-10s d=%s  (no output)\n' "$label" "$depth"
        fails=$((fails + 1))
        continue
    fi

    if [ "$actual" = "$expected" ]; then
        printf '  PASS   %-10s d=%s  %s\n' "$label" "$depth" "$actual"
        passes=$((passes + 1))
    else
        printf '  FAIL   %-10s d=%s  expected %s, got %s\n' \
            "$label" "$depth" "$expected" "$actual"
        fails=$((fails + 1))
    fi
done

echo
echo "Summary: $passes passed, $fails failed"
[ "$fails" -gt 0 ] && exit 1
exit 0
