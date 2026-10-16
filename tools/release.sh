#!/usr/bin/env bash
# Superpawn, a UCI chess engine.
# Copyright (C) 2026 John Byrd and the Superpawn contributors.
# Licensed under the GNU Affero General Public License v3.0 or later.
#
# Cuts a release: checks the tree, tags the current commit and pushes the tag.
# The Release workflow then builds the binaries and publishes the release.
#
#   tools/release.sh 1.9.0
#
# Requirements: on main, clean, identical to origin/main, CI green for this
# commit, version higher than the last release tag.

set -euo pipefail

VERSION="${1:-}"
# major.minor.patch; the major must be at least 1 so the old engine's v0.0.1
# tag never collides with the new series.
[[ "$VERSION" =~ ^[1-9][0-9]*\.[0-9]+\.[0-9]+$ ]] || { echo "usage: $0 major.minor.patch (e.g. 1.9.0)" >&2; exit 2; }
TAG="v$VERSION"

ROOT=$(git rev-parse --show-toplevel)
cd "$ROOT"

fail() { echo "release: $*" >&2; exit 1; }

[[ "$(git branch --show-current)" == "main" ]] || fail "not on main"
[[ -z "$(git status --porcelain)" ]] || fail "working tree is not clean"
git fetch -q origin main --tags
[[ "$(git rev-parse HEAD)" == "$(git rev-parse origin/main)" ]] || fail "HEAD differs from origin/main; push or pull first"
git rev-parse -q --verify "refs/tags/$TAG" >/dev/null && fail "tag $TAG already exists"

# The version must be greater than the latest release tag
LAST=$(git tag -l 'v[1-9]*' --sort=-v:refname | head -1)
if [[ -n "$LAST" ]]; then
    HIGHEST=$(printf '%s\n%s\n' "$LAST" "$TAG" | sort -V | tail -1)
    [[ "$HIGHEST" == "$TAG" ]] || fail "$TAG is not higher than the last release $LAST"
fi

# CI must be green for this commit
if command -v gh >/dev/null 2>&1; then
    STATUS=$(gh run list --commit "$(git rev-parse HEAD)" --workflow Build --json conclusion --jq '.[0].conclusion' 2>/dev/null || true)
    [[ "$STATUS" == "success" ]] || fail "Build workflow for this commit is '${STATUS:-not found}', not success"
else
    echo "release: gh not installed, skipping the CI check" >&2
fi

NET=$(grep -v '^#' nets/NETWORK | awk 'NF {print $1; exit}')
BENCH=""
if [[ -x build/superpawn ]]; then
    BENCH=$(build/superpawn bench 2>&1 | awk '/Nodes searched/ {print $NF}')
fi

git tag -a "$TAG" -m "Superpawn $VERSION

Network: $NET${BENCH:+
Bench: $BENCH}"
git push origin "$TAG"

echo "release: tagged and pushed $TAG; the Release workflow is building the binaries:"
echo "         https://github.com/johnwbyrd/superpawn/actions"
