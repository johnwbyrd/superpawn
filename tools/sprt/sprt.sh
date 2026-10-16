#!/usr/bin/env bash
# Superpawn, a UCI chess engine.
# Copyright (C) 2026 John Byrd and the Superpawn contributors.
# Licensed under the GNU Affero General Public License v3.0 or later.
#
# Runs a Sequential Probability Ratio Test between two versions of the engine
# with fastchess. By default the working tree ("dev") plays the last commit
# ("base") at short time control with the usual gainer bounds [0, 5].
#
#   tools/sprt/sprt.sh                      # working tree vs HEAD, STC, [0, 5]
#   tools/sprt/sprt.sh --base origin/feature/next
#   tools/sprt/sprt.sh --ltc --simplify     # 60+0.6, bounds [-5, 0]
#   tools/sprt/sprt.sh --new HEAD --base HEAD~1 -c 8
#
# fastchess and the opening book are fetched and built on first use into
# tools/sprt/bin and tools/sprt/books. Engine builds are cached per commit in
# tools/sprt/builds, and each run writes its games and output to
# tools/sprt/results/<timestamp>-<name>/.

set -euo pipefail

ROOT=$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)
TOOLS="$ROOT/tools/sprt"
BIN="$TOOLS/bin"
BOOKS="$TOOLS/books"
BUILDS="$TOOLS/builds"
RESULTS="$TOOLS/results"

FASTCHESS_REPO="https://github.com/Disservin/fastchess.git"
BOOK_NAME="UHO_Lichess_4852_v1.epd"
BOOK_URL="https://raw.githubusercontent.com/official-stockfish/books/master/${BOOK_NAME}.zip"

# Defaults: short time control, gainer bounds, one thread per game.
BASE="HEAD"
NEW=""            # empty means the working tree
TC="10+0.1"
NODES=""        # when set, play at fixed nodes per move instead of on the clock
HASH=16
THREADS=1
ELO0=0
ELO1=5
ALPHA=0.05
BETA=0.05
MODEL="normalized"
ROUNDS=100000
BOOK=""
NAME=""
CONCURRENCY=""
REBUILD=0
DRY_RUN=0
FASTCHESS="${FASTCHESS:-}"

usage() {
    sed -n '5,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    cat <<USAGE

Options:
  --base REF          baseline git ref (default HEAD)
  --new REF           candidate git ref (default: the working tree)
  --tc TC             time control, e.g. 10+0.1 (default)
  --ltc               long time control preset: 60+0.6, Hash 64
  --nodes N           fixed nodes per move instead of a clock (removes speed from the comparison)
  --bounds LO,HI      SPRT Elo bounds (default 0,5)
  --gainer            bounds [0, 5]
  --simplify          bounds [-5, 0]
  --hash MB           Hash option for both engines (default 16)
  --threads N         Threads option for both engines (default 1)
  -c, --concurrency N games in parallel (default: cores - 1, divided by threads)
  --rounds N          maximum rounds of two games (default 100000, SPRT stops earlier)
  --book FILE         EPD opening book (default: UHO_Lichess_4852_v1, downloaded)
  --name NAME         label for the results directory
  --fastchess PATH    fastchess binary (default: tools/sprt/bin, PATH, or build it)
  --rebuild           rebuild cached engine binaries
  --dry-run           print the fastchess command without running it
  --                  pass the remaining arguments to fastchess

Environment: SP_ARCH (default native) and SP_CMAKE_ARGS are passed to CMake.
USAGE
}

EXTRA=()
while [[ $# -gt 0 ]]; do
    case "$1" in
    --base) BASE="$2"; shift 2 ;;
    --new) NEW="$2"; shift 2 ;;
    --tc) TC="$2"; shift 2 ;;
    --ltc) TC="60+0.6"; HASH=64; shift ;;
    --nodes) NODES="$2"; shift 2 ;;
    --bounds) ELO0="${2%,*}"; ELO1="${2#*,}"; shift 2 ;;
    --gainer) ELO0=0; ELO1=5; shift ;;
    --simplify) ELO0=-5; ELO1=0; shift ;;
    --hash) HASH="$2"; shift 2 ;;
    --threads) THREADS="$2"; shift 2 ;;
    -c | --concurrency) CONCURRENCY="$2"; shift 2 ;;
    --rounds) ROUNDS="$2"; shift 2 ;;
    --book) BOOK="$2"; shift 2 ;;
    --name) NAME="$2"; shift 2 ;;
    --fastchess) FASTCHESS="$2"; shift 2 ;;
    --rebuild) REBUILD=1; shift ;;
    --dry-run) DRY_RUN=1; shift ;;
    -h | --help) usage; exit 0 ;;
    --) shift; EXTRA=("$@"); break ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

log() { printf '\033[1m[sprt]\033[0m %s\n' "$*" >&2; }

cores() {
    if command -v nproc >/dev/null 2>&1; then nproc
    elif command -v sysctl >/dev/null 2>&1; then sysctl -n hw.ncpu
    else echo 2
    fi
}

if [[ -z "$CONCURRENCY" ]]; then
    CONCURRENCY=$(( ($(cores) - 1) / THREADS ))
    [[ $CONCURRENCY -lt 1 ]] && CONCURRENCY=1
fi

# ---------------------------------------------------------------------------
# fastchess
# ---------------------------------------------------------------------------
ensure_fastchess() {
    if [[ -n "$FASTCHESS" ]]; then
        [[ -x "$FASTCHESS" ]] || { echo "fastchess not executable: $FASTCHESS" >&2; exit 1; }
        return
    fi
    if [[ -x "$BIN/fastchess" ]]; then
        FASTCHESS="$BIN/fastchess"
        return
    fi
    if command -v fastchess >/dev/null 2>&1; then
        FASTCHESS=$(command -v fastchess)
        return
    fi

    log "fastchess not found, building it from $FASTCHESS_REPO"
    mkdir -p "$BIN"
    rm -rf "$BIN/fastchess-src"
    git clone -q --depth 1 "$FASTCHESS_REPO" "$BIN/fastchess-src"
    make -C "$BIN/fastchess-src" -j"$(cores)" >"$BIN/fastchess-build.log" 2>&1 \
        || { echo "fastchess build failed, see $BIN/fastchess-build.log" >&2; exit 1; }
    cp "$BIN/fastchess-src/fastchess" "$BIN/fastchess"
    rm -rf "$BIN/fastchess-src"
    FASTCHESS="$BIN/fastchess"
    log "built $("$FASTCHESS" -version 2>&1 | head -1)"
}

# ---------------------------------------------------------------------------
# Opening book
# ---------------------------------------------------------------------------
ensure_book() {
    if [[ -n "$BOOK" ]]; then
        [[ -f "$BOOK" ]] || { echo "book not found: $BOOK" >&2; exit 1; }
        return
    fi
    BOOK="$BOOKS/$BOOK_NAME"
    [[ -f "$BOOK" ]] && return

    log "downloading opening book $BOOK_NAME (about 40 MB)"
    mkdir -p "$BOOKS"
    curl -sSL --fail -o "$BOOKS/book.zip" "$BOOK_URL"
    if command -v unzip >/dev/null 2>&1; then
        unzip -o -q "$BOOKS/book.zip" -d "$BOOKS"
    else
        python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$BOOKS/book.zip" "$BOOKS"
    fi
    rm -f "$BOOKS/book.zip"
    [[ -f "$BOOK" ]] || { echo "book download did not produce $BOOK" >&2; exit 1; }
}

# ---------------------------------------------------------------------------
# Engine builds
# ---------------------------------------------------------------------------
cmake_build() {  # source dir, build dir, output binary
    local src="$1" bld="$2" out="$3"
    cmake -S "$src" -B "$bld" -DCMAKE_BUILD_TYPE=Release -DSP_BUILD_TESTS=OFF \
        -DSP_ARCH="${SP_ARCH:-native}" ${SP_CMAKE_ARGS:-} >"$bld.log" 2>&1 \
        && cmake --build "$bld" --parallel >>"$bld.log" 2>&1 \
        || { echo "build failed, see $bld.log" >&2; exit 1; }

    # A build that names a network but could not get it would silently test
    # the hand-crafted evaluation. Refuse rather than cache such a binary.
    if grep -qE "Could not download|does not match the SHA-256" "$bld.log"; then
        grep -E "Could not download|does not match the SHA-256" "$bld.log" >&2
        echo "the network named in nets/NETWORK is not available; fix the release or the checksum and rerun" >&2
        rm -f "$out"
        exit 1
    fi
    cp "$bld/superpawn" "$out"
}

# Reports whether a binary carries a network, from its startup line.
net_of() {
    printf 'uci\nquit\n' | "$1" 2>/dev/null | sed -n 's/^info string //p' | head -1
}

# Builds a committed revision in a temporary worktree. Cached by commit hash.
build_ref() {  # ref -> sets BUILT_BIN and BUILT_NAME
    local ref="$1" sha dir
    sha=$(git -C "$ROOT" rev-parse --short=12 "${ref}^{commit}")
    dir="$BUILDS/$sha"
    BUILT_BIN="$dir/superpawn"
    BUILT_NAME="$sha"

    if [[ -x "$BUILT_BIN" && $REBUILD -eq 0 ]]; then
        log "using cached build of $ref ($sha)"
        return
    fi

    log "building $ref ($sha)"
    rm -rf "$dir"
    mkdir -p "$dir"
    git -C "$ROOT" worktree add -q --detach "$dir/src" "$sha"
    cmake_build "$dir/src" "$dir/build" "$BUILT_BIN"
    git -C "$ROOT" worktree remove --force "$dir/src"
    rm -rf "$dir/build"
}

# Builds the working tree as it is, including uncommitted changes.
build_worktree() {
    local dir="$BUILDS/dev"
    mkdir -p "$dir"
    log "building the working tree ($(git -C "$ROOT" describe --always --dirty))"
    cmake_build "$ROOT" "$dir/build" "$dir/superpawn"
    BUILT_BIN="$dir/superpawn"
    BUILT_NAME="dev-$(git -C "$ROOT" describe --always --dirty | tr -c 'A-Za-z0-9._\n-' '_')"
}

bench_of() {  # binary -> node count of a short bench, the search signature
    "$1" bench 16 1 10 2>&1 | awk '/Nodes searched/ {print $NF}'
}

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
ensure_fastchess
ensure_book

build_ref "$BASE"
BASE_BIN="$BUILT_BIN"; BASE_NAME="base-$BUILT_NAME"

if [[ -n "$NEW" ]]; then
    build_ref "$NEW"
    NEW_BIN="$BUILT_BIN"; NEW_NAME="new-$BUILT_NAME"
else
    build_worktree
    NEW_BIN="$BUILT_BIN"; NEW_NAME="$BUILT_NAME"
fi

BASE_BENCH=$(bench_of "$BASE_BIN")
NEW_BENCH=$(bench_of "$NEW_BIN")
log "bench: $NEW_NAME $NEW_BENCH vs $BASE_NAME $BASE_BENCH"
log "eval:  new: $(net_of "$NEW_BIN")"
log "       base: $(net_of "$BASE_BIN")"
if [[ "$BASE_BENCH" == "$NEW_BENCH" && -z "$NEW" ]]; then
    log "warning: identical bench signatures; the search may be unchanged"
fi

# Fixed nodes per move, or a clock
if [[ -n "$NODES" ]]; then LIMIT="nodes=$NODES"; else LIMIT="tc=$TC"; fi

RUN="$RESULTS/$(date +%Y%m%d-%H%M%S)${NAME:+-$NAME}"
mkdir -p "$RUN"

CMD=(
    "$FASTCHESS"
    -engine "cmd=$NEW_BIN" "name=$NEW_NAME"
    -engine "cmd=$BASE_BIN" "name=$BASE_NAME"
    -each "$LIMIT" "option.Hash=$HASH" "option.Threads=$THREADS" proto=uci
    -openings "file=$BOOK" format=epd order=random
    -repeat -rounds "$ROUNDS" -games 2
    -concurrency "$CONCURRENCY" -recover
    -sprt "elo0=$ELO0" "elo1=$ELO1" "alpha=$ALPHA" "beta=$BETA" "model=$MODEL"
    -draw movenumber=40 movecount=8 score=10
    -resign movecount=3 score=600 twosided=true
    -maxmoves 250
    -ratinginterval 10 -report penta=true
    -pgnout "file=$RUN/games.pgn"
    -config "outname=$RUN/config.json"
    "${EXTRA[@]}"
)

printf '%q ' "${CMD[@]}" >"$RUN/command.txt"; echo >>"$RUN/command.txt"
{
    echo "base:  $BASE ($BASE_NAME) bench $BASE_BENCH  [$(net_of "$BASE_BIN")]"
    echo "new:   ${NEW:-working tree} ($NEW_NAME) bench $NEW_BENCH  [$(net_of "$NEW_BIN")]"
    echo "limit: $LIMIT  hash $HASH  threads $THREADS  concurrency $CONCURRENCY"
    echo "sprt:  [$ELO0, $ELO1]  alpha $ALPHA  beta $BETA  $MODEL"
} >"$RUN/summary.txt"
cat "$RUN/summary.txt" >&2

if [[ $DRY_RUN -eq 1 ]]; then
    cat "$RUN/command.txt"
    exit 0
fi

log "results in $RUN"
"${CMD[@]}" 2>&1 | tee "$RUN/output.txt"
