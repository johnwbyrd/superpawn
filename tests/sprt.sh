#!/usr/bin/env bash
# Decide whether a change to Superpawn is an improvement: play the new
# build against the old one under the fixed conditions in TESTING.md
# until a sequential probability ratio test (SPRT) reaches a verdict.
#
# Usage:
#   tests/sprt.sh NEW_BINARY OLD_BINARY [extra fastchess args...]
#
# Environment knobs (defaults are the TESTING.md standard; change them
# only for the confirmation runs TESTING.md describes):
#   FASTCHESS   path to fastchess; default: found on PATH.
#   TC          time control; default 10+0.1. For evaluation-only changes
#               TESTING.md suggests fixed nodes instead: TC=inf NODES=200000.
#   NODES       if set, fixed nodes per move (used with TC=inf).
#   HASH        hash size in MB per engine; default 64.
#   CONCURRENCY games in flight; default: number of cores - 1, min 1.
#   ELO0 ELO1   SPRT bounds; default 0 and 5.
#   BOOK        opening book; default tests/openings/book.epd.
#   OUT_DIR     where the PGN and log go; default /tmp/superpawn-sprt.
#
# Anything after the two binaries is passed to fastchess unchanged, so
# e.g. "-rounds 20" caps a shakedown run.

set -u

if [ $# -lt 2 ]; then
    sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
fi

NEW="$1"; OLD="$2"; shift 2

HERE="$(cd "$(dirname "$0")/.." && pwd)"
FASTCHESS="${FASTCHESS:-$(command -v fastchess || true)}"
TC="${TC:-10+0.1}"
NODES="${NODES:-}"
HASH="${HASH:-64}"
ELO0="${ELO0:-0}"
ELO1="${ELO1:-5}"
BOOK="${BOOK:-$HERE/tests/openings/book.epd}"
OUT_DIR="${OUT_DIR:-/tmp/superpawn-sprt}"
if [ -z "${CONCURRENCY:-}" ]; then
    CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    CONCURRENCY=$(( CORES > 1 ? CORES - 1 : 1 ))
fi

die() { echo "error: $*" >&2; exit 2; }
[ -n "$FASTCHESS" ] && [ -x "$FASTCHESS" ] || die "fastchess not found; set FASTCHESS=/path/to/fastchess"
[ -x "$NEW" ] || die "new binary not executable: $NEW"
[ -x "$OLD" ] || die "old binary not executable: $OLD"
[ -r "$BOOK" ] || die "opening book not found: $BOOK"
[ "$NEW" -ef "$OLD" ] && die "new and old are the same file; copy the old binary aside before rebuilding"

mkdir -p "$OUT_DIR"
STAMP=$(date +%Y%m%d-%H%M%S)

LIMIT="tc=$TC"
[ -n "$NODES" ] && LIMIT="$LIMIT nodes=$NODES"

echo "SPRT [$ELO0, $ELO1] at $LIMIT, hash $HASH MB, concurrency $CONCURRENCY"
echo "new: $NEW"
echo "old: $OLD"
echo "pgn: $OUT_DIR/sprt-$STAMP.pgn"
echo

exec "$FASTCHESS" \
    -engine cmd="$NEW" name=new \
    -engine cmd="$OLD" name=old \
    -each proto=uci $LIMIT option.Hash="$HASH" \
    -openings file="$BOOK" format=epd order=random \
    -repeat -games 2 -rounds 5000 \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha=0.05 beta=0.05 \
    -draw movenumber=40 movecount=8 score=10 \
    -resign movecount=3 score=800 twosided=true \
    -concurrency "$CONCURRENCY" \
    -ratinginterval 10 \
    -pgnout file="$OUT_DIR/sprt-$STAMP.pgn" \
    "$@"
