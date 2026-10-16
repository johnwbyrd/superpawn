SPRT testing
============

Search and evaluation changes are accepted on the result of a Sequential
Probability Ratio Test, not on intuition. `sprt.sh` runs one with
[fastchess](https://github.com/Disservin/fastchess) between the working tree
and a baseline commit.

    tools/sprt/sprt.sh                          # working tree vs HEAD, 10+0.1, bounds [0, 5]
    tools/sprt/sprt.sh --base origin/feature/next --name lmr-tweak
    tools/sprt/sprt.sh --ltc                    # 60+0.6, Hash 64
    tools/sprt/sprt.sh --simplify               # bounds [-5, 0]
    tools/sprt/sprt.sh --new HEAD --base HEAD~1 # two commits
    tools/sprt/sprt.sh --dry-run                # show the fastchess command only

The first run clones and builds fastchess into `tools/sprt/bin` and downloads
the UHO_Lichess_4852_v1 opening book into `tools/sprt/books`. Both are
ignored by git. Engines are built with `-DSP_ARCH=native` and cached per
commit under `tools/sprt/builds`. Each run writes `summary.txt`,
`command.txt`, `output.txt`, `games.pgn` and the fastchess `config.json` to
`tools/sprt/results/<timestamp>-<name>/`.

The script needs bash, git, cmake, a C++ compiler, curl and unzip (or
python3). On Windows, run it from WSL or Git Bash.

Conventions
-----------

| Test            | Time control | Hash | Bounds   |
|-----------------|--------------|------|----------|
| Gainer, STC     | 10+0.1       | 16   | [0, 5]   |
| Gainer, LTC     | 60+0.6       | 64   | [0, 5]   |
| Simplification  | 10+0.1       | 16   | [-5, 0]  |

A gainer must pass STC and then LTC. A simplification or a non-functional
change that alters the search needs one passing simplification test. These
bounds suit an engine whose patches still gain several Elo at a time; they
will tighten as the engine matures.

Bounds are in normalized Elo (fastchess `model=normalized`), with alpha and
beta of 0.05. Games use the UHO book, which starts from unbalanced openings
to reduce draws. Draws are adjudicated after move 40 when both engines
report within 10 centipawns for 8 moves; losses when both agree on more
than 600 centipawns for 3 moves.

Before each run the script prints the `bench` signature of both binaries.
A functional change must change the signature; a change meant to be
non-functional must not.

Absolute strength
-----------------

SPRT only measures gains between two versions. For a rating estimate,
`gauntlet.sh` plays the working tree against Stockfish at several `UCI_Elo`
levels, whose strength limiter is calibrated to roughly CCRL-like ratings:

    tools/sprt/gauntlet.sh --levels 1800,2200,2600,3000 --games 100

Stockfish is downloaded into `tools/sprt/bin` on first use (Linux x86-64;
`--stockfish` points at another binary). The level the engine scores about
50 percent against is the estimate. fastchess prints warnings about
Stockfish's best move not matching its PV at limited strength; they are
harmless.

Reporting
---------

Put the fastchess result block in the commit message, for example:

    STC: https://... or the summary line
    LLR: 2.95 (-2.94, 2.94) [0.00, 5.00]
    Total: 1984 W: 612 L: 497 D: 875
    Ptnml(0-2): 31, 207, 419, 283, 52
