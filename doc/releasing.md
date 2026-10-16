Releasing
=========

Versions are `2.0.x`. The patch number goes up by one per release and means
nothing else; the minor number changes when the engine changes character,
such as a new network architecture. A release is a git tag `v2.0.x` on
`main`; the Release workflow does the rest.

What gets released
------------------

The Release workflow (`.github/workflows/release.yml`) builds statically
linked binaries with the current network embedded:

| Archive | For |
|---|---|
| `superpawn-2.0.x-linux-x86-64-avx2.tar.gz` | Linux, any x86-64 CPU from about 2015 on |
| `superpawn-2.0.x-linux-x86-64.tar.gz` | Linux, older CPUs |
| `superpawn-2.0.x-windows-x86-64-avx2.zip` | Windows, any x86-64 CPU from about 2015 on |
| `superpawn-2.0.x-windows-x86-64.zip` | Windows, older CPUs |
| `superpawn-2.0.x-macos-arm64.tar.gz` | Apple Silicon Macs |
| `superpawn-2.0.x-macos-x86-64.tar.gz` | Intel Macs |

Each archive holds the binary, `README.md`, `LICENSE` and the logo. The
release notes start with the network name and the bench signature and
continue with the commits since the previous release.

Every push to `main` also rebuilds the same set and refreshes the rolling
`latest` prerelease, so testers can always get the newest code without a
version being cut.

Cutting a release
-----------------

When a network has been adopted or enough tested changes have accumulated:

    git checkout main && git pull
    tools/release.sh 2.0.1

The script refuses to proceed unless you are on a clean `main` identical to
`origin/main`, the Build workflow is green for that commit, and the version
is higher than the last release tag. It creates an annotated tag whose
message records the network and the bench, pushes it, and the workflow
publishes the release a few minutes later at
https://github.com/johnwbyrd/superpawn/releases.

The engine reports its version from the tag: `Superpawn 2.0.1` when built at
the tag, `Superpawn 2.0.1-14-g1a2b3c4` for a build fourteen commits later.

Rhythm
------

One release per adopted network is a good default, since each adopted
network is a measured gain. Search changes that pass SPRT ride along with the
next one. There is no need to batch further; releases are cheap.
