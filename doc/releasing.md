Releasing
=========

Versions are `major.minor.patch`, starting at 1.9.0 for the rewrite. The
patch number goes up by one per release and means nothing else; the minor
number changes when the engine changes character, such as a new network
architecture; the major is for a change people must know about. A release is
a git tag `vX.Y.Z` on `main`; the Release workflow does the rest. The old
engine's `v0.0.1` tag is ignored, so the major must be at least 1.

What gets released
------------------

The Release workflow (`.github/workflows/release.yml`) builds statically
linked binaries with the current network embedded:

| Archive | For |
|---|---|
| `superpawn-linux-x86-64-avx2.tar.gz` | Linux, any x86-64 CPU from about 2015 on |
| `superpawn-linux-x86-64.tar.gz` | Linux, older CPUs |
| `superpawn-windows-x86-64-avx2.zip` | Windows, any x86-64 CPU from about 2015 on |
| `superpawn-windows-x86-64.zip` | Windows, older CPUs |
| `superpawn-macos-arm64.tar.gz` | Apple Silicon Macs |
| `superpawn-macos-x86-64.tar.gz` | Intel Macs |

The archive names carry no version, so the README can link to
`releases/latest/download/<archive>`, which GitHub resolves to the newest
non-prerelease release; those links never go stale. The folder inside each
archive is `superpawn-X.Y.Z-<platform>`, so unpacked builds of different
versions do not collide. Each archive holds the binary, `README.md`,
`LICENSE` and the logo. The release notes start with the network name and
the bench signature and continue with the commits since the previous
release.

Every push to `main` also rebuilds the same set and refreshes the rolling
`latest` prerelease, so testers can always get the newest code without a
version being cut.

Cutting a release
-----------------

When a network has been adopted or enough tested changes have accumulated:

    git checkout main && git pull
    tools/release.sh 1.9.0

The script refuses to proceed unless you are on a clean `main` identical to
`origin/main`, the Build workflow is green for that commit, and the version
is higher than the last release tag. It creates an annotated tag whose
message records the network and the bench, pushes it, and the workflow
publishes the release a few minutes later at
https://github.com/johnwbyrd/superpawn/releases.

The engine reports its version from the tag: `Superpawn 1.9.0` when built at
the tag, `Superpawn 1.9.0-14-g1a2b3c4` for a build fourteen commits later.

Rhythm
------

One release per adopted network is a good default, since each adopted
network is a measured gain. Search changes that pass SPRT ride along with the
next one. There is no need to batch further; releases are cheap.
