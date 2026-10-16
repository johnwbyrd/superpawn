#!/usr/bin/env bash
# Superpawn, a UCI chess engine.
# Copyright (C) 2026 John Byrd and the Superpawn contributors.
# Licensed under the GNU Affero General Public License v3.0 or later.
#
# Estimates the engine's absolute strength by playing a gauntlet against
# Stockfish at several UCI_Elo levels. Stockfish's strength limiter is
# calibrated to roughly CCRL-like ratings, so the level the engine breaks even
# against is a usable estimate; SPRT runs only ever measure relative gains.
#
#   tools/sprt/gauntlet.sh                       # working tree, 40 games per level
#   tools/sprt/gauntlet.sh --levels 2000,2400,2800 --games 100 --tc 10+0.1
#   tools/sprt/gauntlet.sh --engine path/to/superpawn
#
# Stockfish is downloaded into tools/sprt/bin on first use (Linux x86-64 only;
# point --stockfish at a binary elsewhere).

set -euo pipefail

ROOT=$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)
TOOLS="$ROOT/tools/sprt"
BIN="$TOOLS/bin"
RESULTS="$TOOLS/results"

SF_URL="https://github.com/official-stockfish/Stockfish/releases/download/sf_17.1/stockfish-ubuntu-x86-64-avx2.tar"

ENGINE=""
STOCKFISH="${STOCKFISH:-}"
LEVELS="1800,2200,2600,3000"
GAMES=40
TC="10+0.1"
HASH=16
CONCURRENCY=""
NAME="gauntlet"

usage() { sed -n '5,17p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

while [[ $# -gt 0 ]]; do
    case "$1" in
    --engine) ENGINE="$2"; shift 2 ;;
    --stockfish) STOCKFISH="$2"; shift 2 ;;
    --levels) LEVELS="$2"; shift 2 ;;
    --games) GAMES="$2"; shift 2 ;;
    --tc) TC="$2"; shift 2 ;;
    --hash) HASH="$2"; shift 2 ;;
    -c | --concurrency) CONCURRENCY="$2"; shift 2 ;;
    --name) NAME="$2"; shift 2 ;;
    -h | --help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

log() { printf '\033[1m[gauntlet]\033[0m %s\n' "$*" >&2; }
cores() { command -v nproc >/dev/null 2>&1 && nproc || sysctl -n hw.ncpu 2>/dev/null || echo 2; }
[[ -n "$CONCURRENCY" ]] || CONCURRENCY=$(( $(cores) - 1 )); [[ $CONCURRENCY -lt 1 ]] && CONCURRENCY=1

# Reuse the SPRT script's helpers for fastchess, the book and the engine build
# shellcheck source=/dev/null
source <(sed -n '/^ensure_fastchess()/,/^}/p;/^ensure_book()/,/^}/p;/^cmake_build()/,/^}/p;/^build_worktree()/,/^}/p;/^bench_of()/,/^}/p' "$TOOLS/sprt.sh")
BOOKS="$TOOLS/books"; BUILDS="$TOOLS/builds"; FASTCHESS="${FASTCHESS:-}"; BOOK=""; REBUILD=0
BOOK_NAME="UHO_Lichess_4852_v1.epd"
BOOK_URL="https://raw.githubusercontent.com/official-stockfish/books/master/${BOOK_NAME}.zip"

ensure_fastchess
ensure_book

if [[ -z "$STOCKFISH" ]]; then
    STOCKFISH="$BIN/stockfish"
    if [[ ! -x "$STOCKFISH" ]]; then
        log "downloading Stockfish"
        mkdir -p "$BIN"
        curl -sSL --fail -o "$BIN/stockfish.tar" "$SF_URL"
        tar -xf "$BIN/stockfish.tar" -C "$BIN" --wildcards 'stockfish/stockfish-*'
        mv "$BIN"/stockfish/stockfish-* "$STOCKFISH"
        rm -rf "$BIN/stockfish.tar" "$BIN/stockfish"
        chmod +x "$STOCKFISH"
    fi
fi
[[ -x "$STOCKFISH" ]] || { echo "Stockfish not executable: $STOCKFISH" >&2; exit 1; }

if [[ -z "$ENGINE" ]]; then
    build_worktree
    ENGINE="$BUILT_BIN"
    ENGINE_NAME="$BUILT_NAME"
else
    ENGINE_NAME="superpawn"
fi

RUN="$RESULTS/$(date +%Y%m%d-%H%M%S)-$NAME"
mkdir -p "$RUN"

CMD=("$FASTCHESS" -engine "cmd=$ENGINE" "name=$ENGINE_NAME")
IFS=',' read -ra LEVEL_LIST <<<"$LEVELS"
for lvl in "${LEVEL_LIST[@]}"; do
    CMD+=(-engine "cmd=$STOCKFISH" "name=SF-$lvl" option.UCI_LimitStrength=true "option.UCI_Elo=$lvl")
done
CMD+=(
    -each "tc=$TC" "option.Hash=$HASH" option.Threads=1 proto=uci
    -tournament gauntlet -seeds 1
    -openings "file=$BOOK" format=epd order=random
    -repeat -rounds $(( (GAMES + 1) / 2 )) -games 2
    -concurrency "$CONCURRENCY" -recover
    -draw movenumber=40 movecount=8 score=10
    -resign movecount=3 score=600 twosided=true
    -maxmoves 250
    -ratinginterval 10 -report penta=false
    -pgnout "file=$RUN/games.pgn"
    -config "outname=$RUN/config.json"
)

printf '%q ' "${CMD[@]}" >"$RUN/command.txt"; echo >>"$RUN/command.txt"
log "engine $ENGINE_NAME (bench $(bench_of "$ENGINE")) vs Stockfish at UCI_Elo $LEVELS, $GAMES games each, $TC"
log "results in $RUN"
"${CMD[@]}" 2>&1 | tee "$RUN/output.txt"
