#!/usr/bin/env bash
# Run Superpawn against a set of opponent engines using cutechess-cli.
# Writes one PGN per opponent plus an aggregate PGN, and prints a
# per-opponent score table at the end.
#
# Usage:
#   tests/gauntlet/gauntlet.sh [path/to/superpawn]
#
# Environment knobs:
#   CUTECHESS   path to cutechess-cli; auto-detected on PATH and at
#               ~/git/cutechess/build/cutechess-cli if not set.
#   ROUNDS      pairs of games per opponent; default 5 (10 games/opp).
#   TC          time control in cutechess syntax; default 10+0.1
#               (10 seconds base, 0.1 second increment per move).
#   OUT_DIR     where to write PGN files; default /tmp/superpawn-gauntlet.
#   TSCP        path to the tscp binary; auto-detected at ~/git/tscp/tscp.
#   FAIRYMAX    path to fairymax; auto-detected via `command -v fairymax`.
#
# Missing opponents are silently skipped so the script does something
# useful on a stock Debian/Ubuntu box with only one of them installed.

set -u

ENGINE="${1:-./build/superpawn}"

CUTECHESS="${CUTECHESS:-}"
if [ -z "$CUTECHESS" ]; then
    if command -v cutechess-cli > /dev/null; then
        CUTECHESS=$(command -v cutechess-cli)
    elif [ -x "$HOME/git/cutechess/build/cutechess-cli" ]; then
        CUTECHESS="$HOME/git/cutechess/build/cutechess-cli"
    fi
fi

ROUNDS="${ROUNDS:-5}"
TC="${TC:-10+0.1}"
OUT_DIR="${OUT_DIR:-/tmp/superpawn-gauntlet}"

TSCP="${TSCP:-$HOME/git/tscp/tscp}"
FAIRYMAX="${FAIRYMAX:-$(command -v fairymax || true)}"

die() { echo "error: $*" >&2; exit 2; }

[ -x "$ENGINE" ]    || die "engine not executable: $ENGINE"
[ -n "$CUTECHESS" ] && [ -x "$CUTECHESS" ] \
    || die "cutechess-cli not found; set CUTECHESS=/path/to/cutechess-cli"

mkdir -p "$OUT_DIR"
AGG_PGN="$OUT_DIR/all-games.pgn"
: > "$AGG_PGN"

# Each opponent is: label|command|protocol|dir (dir may be empty).
OPPONENTS=()
[ -x "$FAIRYMAX" ] && OPPONENTS+=( "fairymax|$FAIRYMAX|xboard|" )
[ -x "$TSCP" ]     && OPPONENTS+=( "tscp|$TSCP|xboard|$(dirname "$TSCP")" )

if [ "${#OPPONENTS[@]}" -eq 0 ]; then
    die "no opponents found; install fairymax (apt) or build TSCP at ~/git/tscp/tscp"
fi

echo "Superpawn gauntlet"
echo "  engine:     $ENGINE"
echo "  cutechess:  $CUTECHESS"
echo "  rounds:     $ROUNDS (= $((ROUNDS * 2)) games per opponent)"
echo "  tc:         $TC"
echo "  out:        $OUT_DIR"
echo

TOTAL_W=0
TOTAL_D=0
TOTAL_L=0

printf '%-12s  %4s  %4s  %4s  %5s\n' "opponent" "W" "D" "L" "score"
printf '%-12s  %4s  %4s  %4s  %5s\n' "--------" "----" "----" "----" "-----"

for entry in "${OPPONENTS[@]}"; do
    IFS='|' read -r label cmd proto dir <<< "$entry"
    pgn="$OUT_DIR/vs-${label}.pgn"
    opp_args=( -engine "cmd=$cmd" "name=$label" "proto=$proto" )
    [ -n "$dir" ] && opp_args+=( "dir=$dir" )

    out=$( "$CUTECHESS" \
        -engine "cmd=$ENGINE" "name=Superpawn" "proto=uci" \
        "${opp_args[@]}" \
        -each "tc=$TC" \
        -rounds "$ROUNDS" -games 2 \
        -pgnout "$pgn" \
        -recover 2>&1 )

    # cutechess prints a final "Score of A vs B: W - L - D  [...]" line.
    score_line=$(printf '%s\n' "$out" | awk '/^Score of /' | tail -1)
    stats=$(sed -nE 's/.*: *([0-9]+) *- *([0-9]+) *- *([0-9]+).*/\1 \2 \3/p' \
            <<< "$score_line")
    read -r W L D <<< "${stats:-0 0 0}"
    games=$((W + L + D))
    if [ "$games" -gt 0 ]; then
        score=$(awk -v w="$W" -v d="$D" -v g="$games" \
                'BEGIN{printf "%.2f", (w + d*0.5) / g}')
    else
        score="?"
    fi

    printf '%-12s  %4d  %4d  %4d  %5s\n' "$label" "$W" "$D" "$L" "$score"
    cat "$pgn" >> "$AGG_PGN"

    TOTAL_W=$((TOTAL_W + W))
    TOTAL_D=$((TOTAL_D + D))
    TOTAL_L=$((TOTAL_L + L))
done

echo
TOTAL_G=$((TOTAL_W + TOTAL_D + TOTAL_L))
if [ "$TOTAL_G" -gt 0 ]; then
    TOTAL_SCORE=$(awk -v w="$TOTAL_W" -v d="$TOTAL_D" -v g="$TOTAL_G" \
                  'BEGIN{printf "%.2f", (w + d*0.5) / g}')
else
    TOTAL_SCORE="?"
fi
printf '%-12s  %4d  %4d  %4d  %5s\n' "TOTAL" "$TOTAL_W" "$TOTAL_D" "$TOTAL_L" "$TOTAL_SCORE"
echo
echo "Aggregate PGN: $AGG_PGN"

# Optional: if ordo is installed, pipe through it for an Elo table.
if command -v ordo > /dev/null; then
    echo
    echo "Elo (ordo):"
    ordo -q -a 0 -A Superpawn -p "$AGG_PGN" 2>/dev/null | head -20
fi
