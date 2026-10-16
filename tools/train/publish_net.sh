#!/usr/bin/env bash
# Superpawn, a UCI chess engine.
# Copyright (C) 2026 John Byrd and the Superpawn contributors.
# Licensed under the GNU Affero General Public License v3.0 or later.
#
# Makes a network the project's current one:
#
#   tools/train/publish_net.sh nets/spn-002.nnue
#
# writes nets/NETWORK (file name and SHA-256) and uploads the file to the
# "nets" GitHub release with the gh CLI, creating the release if needed.
# Commit nets/NETWORK afterwards; builds without a local nets/default.nnue
# then download and embed this network.

set -euo pipefail

[[ $# -eq 1 ]] || { echo "usage: $0 <network.nnue>" >&2; exit 2; }
NET="$1"
[[ -f "$NET" ]] || { echo "not found: $NET" >&2; exit 1; }

ROOT=$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)
NAME=$(basename "$NET")
[[ "$NAME" != "default.nnue" ]] || { echo "give the network a real name, e.g. nets/spn-002.nnue" >&2; exit 1; }

if command -v sha256sum >/dev/null 2>&1; then
    SHA=$(sha256sum "$NET" | cut -d' ' -f1)
else
    SHA=$(shasum -a 256 "$NET" | cut -d' ' -f1)
fi

mkdir -p "$ROOT/nets"
[[ "$(cd "$(dirname "$NET")" && pwd)/$NAME" == "$ROOT/nets/$NAME" ]] || cp "$NET" "$ROOT/nets/$NAME"

cat >"$ROOT/nets/NETWORK" <<EOT
# The current network: file name and SHA-256. Downloaded from the "nets"
# GitHub release by the build when not present. Written by publish_net.sh.
$NAME $SHA
EOT
echo "nets/NETWORK now names $NAME ($SHA)"

if command -v gh >/dev/null 2>&1; then
    if ! gh release view nets --repo johnwbyrd/superpawn >/dev/null 2>&1; then
        # A prerelease, so that GitHub never picks it as the project's
        # "latest release" (the README's download links go through
        # releases/latest).
        gh release create nets --repo johnwbyrd/superpawn --title "Networks" \
            --notes "NNUE networks referenced by nets/NETWORK. Each file is immutable once uploaded." \
            --target main --prerelease
    fi
    gh release upload nets "$ROOT/nets/$NAME" --repo johnwbyrd/superpawn --clobber
    echo "uploaded $NAME to the nets release"
else
    echo "gh is not installed; upload $ROOT/nets/$NAME to the 'nets' release at"
    echo "https://github.com/johnwbyrd/superpawn/releases/tag/nets (create it if missing)"
fi

echo "now: git add nets/NETWORK && git commit -m 'Use network $NAME' && git push"
