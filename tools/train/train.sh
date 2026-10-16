#!/usr/bin/env bash
# Superpawn, a UCI chess engine.
# Copyright (C) 2026 John Byrd and the Superpawn contributors.
# Licensed under the GNU Affero General Public License v3.0 or later.
#
# Runs the bullet trainer. Finds the CUDA toolkit, which bullet needs as
# CUDA_PATH, builds the trainer in release mode and passes all arguments on:
#
#   tools/train/train.sh --data data/all.bin --id spn-001 --superbatches 40
#
# Works from any directory. Checkpoints go to tools/train/checkpoints/.

set -euo pipefail

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)

# bullet links libcuda, libcudart, libnvrtc and libcublas from
# $CUDA_PATH/lib64 and reads headers from $CUDA_PATH/include. That is the
# layout of NVIDIA's installer (https://developer.nvidia.com/cuda-downloads);
# distribution packages that scatter the libraries elsewhere will not do.
find_cuda() {
    local candidates=()
    [[ -n "${CUDA_PATH:-}" ]] && candidates+=("$CUDA_PATH")
    [[ -n "${CUDA_HOME:-}" ]] && candidates+=("$CUDA_HOME")
    if command -v nvcc >/dev/null 2>&1; then
        candidates+=("$(cd "$(dirname "$(readlink -f "$(command -v nvcc)")")/.." && pwd)")
    fi
    candidates+=(/usr/local/cuda /opt/cuda)
    for c in /usr/local/cuda-*; do
        [[ -d "$c" ]] && candidates+=("$c")
    done

    for c in "${candidates[@]}"; do
        if [[ -d "$c/include" && -d "$c/lib64" ]] && ls "$c"/lib64/libnvrtc.so* >/dev/null 2>&1; then
            echo "$c"
            return 0
        fi
    done
    return 1
}

FEATURES="${SP_TRAIN_FEATURES:-cuda}"

if [[ "$FEATURES" == "cuda" ]]; then
    if ! CUDA_PATH=$(find_cuda); then
        cat >&2 <<MSG
train.sh: no CUDA toolkit found.

bullet needs the full toolkit (including nvrtc) with CUDA_PATH pointing at a
directory that contains include/ and lib64/. Install it from
https://developer.nvidia.com/cuda-downloads, which puts it in /usr/local/cuda,
or set CUDA_PATH yourself:

    CUDA_PATH=/path/to/cuda tools/train/train.sh ...

For an AMD GPU use SP_TRAIN_FEATURES=rocm (needs HIP_PATH), for Apple Silicon
SP_TRAIN_FEATURES=metal.
MSG
        exit 1
    fi
    export CUDA_PATH
    echo "train.sh: using CUDA toolkit at $CUDA_PATH" >&2
fi

# The trainer runs inside tools/train so that its checkpoints land there;
# make path arguments absolute first so they still resolve.
ARGS=()
expect_path=0
for arg in "$@"; do
    if [[ $expect_path -eq 1 ]]; then
        ARGS+=("$(cd "$(dirname "$arg")" 2>/dev/null && pwd)/$(basename "$arg")")
        expect_path=0
    else
        ARGS+=("$arg")
        case "$arg" in
        --data | --resume | --out) expect_path=1 ;;
        esac
    fi
done

cd "$HERE"
cargo run --release --no-default-features --features "$FEATURES" -- "${ARGS[@]}"

cat >&2 <<MSG

train.sh: checkpoints are in $HERE/checkpoints/. To convert the final one:

    $ROOT/build/superpawn convert $HERE/checkpoints/<id>-<n>/quantised.bin $ROOT/nets/<id>.nnue
MSG
