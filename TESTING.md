Testing Superpawn
=================

This document describes how a change to Superpawn is judged. The short
version: a change is an improvement when it beats the previous version
in a self-play match that is large enough to be statistically
meaningful, and not before. Evaluations that "look better", searches
that reach a deeper ply, or a single won game against another engine
are not evidence of an improvement. Games are.

The method is the one used by every serious engine project since
Stockfish's fishtest made it standard: play the new build against the
build immediately before it, from a book of openings, with the colours
swapped on every opening, and let a sequential probability ratio test
(SPRT) decide when enough games have been played.

Tools
-----

Games are run with [fastchess](https://github.com/Disservin/fastchess).
It was written for exactly this workflow: two UCI engines, an opening
book, SPRT stopping, and pentanomial statistics that account for the
two games of each colour-swapped pair being correlated. It is a single
static binary with no Qt dependency.

    git clone https://github.com/Disservin/fastchess
    cd fastchess && make -j
    sudo cp fastchess /usr/local/bin/

cutechess-cli works too and the options are nearly identical, but its
statistics are weaker (trinomial rather than pentanomial) and it drags
in Qt. The gauntlet script in `tests/gauntlet/` still uses it.

Perft is run with `tests/perft/perft.sh` and must pass before anything
below is attempted. It runs in CI on every platform.

Two kinds of change
-------------------

Every change is one of two kinds, and the first thing to do is decide
which.

**Non-functional changes** do not alter any search decision: refactors,
speedups, UCI fixes, build changes. These are proved non-functional by
the `bench` command, which searches a fixed set of positions to a fixed
depth and prints the total node count. If the count is identical before
and after, the search made exactly the same decisions, and the change
needs no games. Measure the speed (nodes per second from the same
`bench` run) and you are done.

**Functional changes** alter the bench count: anything in evaluation,
move ordering, pruning, extensions, time management. These need games.
There is no way to know whether a functional change helps except by
playing it.

A change that was meant to be non-functional but changes the bench
count has a bug, or is functional after all. Find out which before
going further.

The standard test
-----------------

### Fixed conditions

These are fixed so that results from different days and different
changes can be compared. Changing any of them starts a new baseline.

| Condition        | Value                                               |
|------------------|-----------------------------------------------------|
| Time control     | 10+0.1 (10 seconds, 0.1 second increment)           |
| Long control     | 60+0.6, for confirmation runs only                  |
| Hash             | 64 MB per engine                                    |
| Concurrency      | one game per physical core, never more              |
| Openings         | `tests/openings/book.epd`, random order, 8 plies    |
| Colours          | each opening played twice, colours swapped          |
| Draw adjudication| after move 40, when both scores are within 10 cp for 8 moves |
| Resign adjudication | when both scores are beyond 800 cp for 3 moves   |
| SPRT bounds      | elo0 = 0, elo1 = 5, alpha = 0.05, beta = 0.05        |

The opening book is any EPD file of a few thousand positions, 6 to 10
plies into normal openings. It is checked into the repository so every
test uses the same positions. Playing every game from the start
position is not acceptable: the engines will repeat the same handful of
games.

### Running it

Build the candidate and the base into two separately named binaries.
Never test against a binary rebuilt from the same tree at another time;
copy the base binary aside before touching the source.

    cmake --build build && cp build/superpawn /tmp/sp-new
    git stash && cmake --build build && cp build/superpawn /tmp/sp-old && git stash pop

Then:

    fastchess \
      -engine cmd=/tmp/sp-new name=new \
      -engine cmd=/tmp/sp-old name=old \
      -each proto=uci tc=10+0.1 option.Hash=64 \
      -openings file=tests/openings/book.epd format=epd order=random plies=8 \
      -repeat -rounds 5000 -games 2 \
      -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
      -draw movenumber=40 movecount=8 score=10 \
      -resign movecount=3 score=800 \
      -concurrency 4 \
      -ratinginterval 10 \
      -pgnout file=sprt.pgn

`-rounds 5000` is a ceiling, not a target; the SPRT stops the match
when it has a verdict. Adjust `-concurrency` to the number of physical
cores on the machine, leaving none spare for anything else that is
running.

### Reading the result

fastchess prints, on every rating interval, the current Elo estimate
with its error bar, the W/D/L and pentanomial counts, and the
log-likelihood ratio (LLR) against its two bounds. The test ends in one
of two ways:

- **LLR reaches the upper bound: pass.** The new build is, with the
  stated confidence, at least as much better than the old as `elo1`.
  The change is an improvement and may be committed.
- **LLR reaches the lower bound: fail.** The new build is no better
  than the old. The change is not an improvement, however good the idea
  was.

How long this takes depends on the real size of the gain:

| True gain      | Games to a verdict, roughly |
|----------------|-----------------------------|
| 20 Elo or more | 200 to 500                  |
| 5 to 10 Elo    | 500 to 1500                 |
| about 0 Elo    | 1000 to 3000, then fails    |

Proving that something does nothing takes the longest, which is correct.
On one four-core machine at 10+0.1 a test takes between twenty minutes
and a few hours.

Do not stop the test by hand because it looks good. The SPRT already
decides when to stop; stopping at a favourable moment is precisely how
a change that does nothing gets accepted.

### Confirmation at the long control

Search changes (pruning, reductions, extensions, time management) can
help at short time controls and hurt at long ones. After such a change
passes at 10+0.1, run the same test at 60+0.6 with bounds
`elo0=-3 elo1=1`, which asks only "is it not worse". A change that
passes short and fails long is a time-control-dependent heuristic:
usually keep it, but say so in the commit message.

Evaluation-only changes do not need the long run.

### Fixed nodes for evaluation changes

For a change that touches only the evaluation, replace the time control
with a fixed node count:

    -each proto=uci tc=inf nodes=200000 option.Hash=64

The result is then immune to machine load and reproducible on any
computer, and the evaluation is the only thing that differs between the
two sides. Time management is not being tested, so nothing is lost.

Recording the verdict
---------------------

The commit message is the audit trail. Every commit that touches
`Chess.cpp` states one of:

- for a non-functional change, the bench count, which must match the
  previous commit's, and the speed before and after:

        bench: 1234567 nodes (unchanged)
        speed: 571 knps -> 603 knps

- for a functional change, the final line of the fastchess output:

        SPRT [0, 5] at 10+0.1: LLR 2.95 (-2.94, 2.94) PASS
        Elo +11.3 +/- 6.2, 812 games: 231W 428D 153L

A functional change whose message has no SPRT result has not been
tested, whatever its author believes.

Keep a short list of rejected ideas somewhere in the repository, with
their SPRT results, so the same thing is not re-tried untested. A
failure is information.

Rules that keep the results honest
----------------------------------

- **One change per test.** Two changes tested together and passed tell
  you nothing about either; one may be carrying the other. A commit
  that is hard to split into independently testable pieces is usually
  too large.
- **Test against the immediate predecessor.** Testing against a version
  from several changes ago confounds the result with everything in
  between.
- **Same book, same conditions, every time.** The table above exists so
  that no one has to decide these per test.
- **Do not pick the best of several.** Trying ten values of a constant
  and committing the one with the best score is not a test; the best of
  ten noisy results looks like a gain by construction. Tune with SPSA
  (fastchess has it built in), then SPRT the tuned result as one change
  against the untuned base.
- **Do not oversubscribe the machine.** With more games than cores both
  engines lose on time, and not evenly. Do not run anything else heavy
  during a test.
- **Self-play has one blind spot.** It can overvalue knowledge that the
  opponent lacks, because the opponent is the only other player it ever
  meets. The gauntlet below exists to catch this.

The gauntlet: absolute strength
-------------------------------

`tests/gauntlet/gauntlet.sh` plays Superpawn against fixed opponents
(fairymax, TSCP, Stockfish at a fixed depth) and prints a score table.
It answers a different question from the SPRT: not "did this change
help" but "how strong is the engine now, and against what does it
struggle". It is a poor instrument for judging a single change, because
each opponent adds its own variance and a change that helps against
Superpawn's own play may do nothing against TSCP's.

Run it after every few accepted changes, and record the table in the
commit message or a release note. If the self-play Elo keeps rising but
the gauntlet does not move, the engine is learning to beat itself, and
the recent changes should be looked at again.

What an improvement cycle looks like
------------------------------------

1. Make one change on a branch. Run perft.
2. Run `bench` against the base. Unchanged count: measure speed, commit
   with the bench line, done.
3. Changed count: copy the base binary aside, run the SPRT at 10+0.1.
4. Pass: for a search change, run the long-control confirmation.
   Commit with the SPRT line(s). Fail: note the idea and the result in
   the rejected list, discard the change.
5. Every five or so accepted changes, run the gauntlet and record it.

Prerequisites still to be added to the engine
---------------------------------------------

- A `bench` UCI command that searches a fixed list of positions to a
  fixed depth and prints total nodes and nodes per second. Without it,
  non-functional changes cannot be proved non-functional and must be
  played out like any other.
- `tests/openings/book.epd`, the shared opening book.
