![Superpawn logo designed by Angela M. Eads](logo/logo-medium.jpg "Superpawn logo designed by Angela M. Eads")

Superpawn
=========

Superpawn is a [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface)
chess engine written in C++23. This branch is a complete rewrite, built from
the ground up for strength: bitboard move generation, a lockless shared
transposition table, Lazy SMP parallel search, a modern alpha-beta search with
the full set of pruning, reduction and extension heuristics, and an NNUE
evaluator trained on our own hardware.

Superpawn is free software under the GNU Affero General Public License,
version 3 or later. See [LICENSE](LICENSE).

Status
------

The rewrite is in its first milestone: the engine generates moves correctly
(verified against the standard perft suite), speaks UCI, plays legal games on
the clock with any number of threads, and searches with a placeholder
hand-crafted evaluation. The NNUE evaluator and the testing and tuning
pipeline are next. See [DESIGN.md](DESIGN.md) for the architecture and the
roadmap.

Building
--------

You need CMake 3.21 or newer and a C++23 compiler: GCC 13, Clang 17, or
Visual Studio 2022 17.8 and later.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ./build/superpawn

Useful CMake options:

| Option          | Default      | Meaning                                                      |
|-----------------|--------------|--------------------------------------------------------------|
| `SP_ARCH`       | `x86-64-v3`  | Target CPU: `x86-64-v3` (AVX2), `x86-64-v4` (AVX-512), `x86-64` (any x86-64), `native`, or `none` |
| `SP_USE_PEXT`   | `OFF`        | Use the BMI2 PEXT instruction for sliding attacks. Fast on Intel and AMD Zen 3 and later, slow on Zen 1 and 2 |
| `SP_LTO`        | `ON`         | Link-time optimisation                                       |
| `SP_BUILD_TESTS`| `ON`         | Build the unit tests                                         |
| `SP_WERROR`     | `OFF`        | Treat compiler warnings as errors                            |

On non-x86 hosts such as Apple Silicon, `SP_ARCH` defaults to `none` and the
engine uses its portable code paths.

Running
-------

Superpawn is meant to be driven by a UCI-compatible GUI such as Cute Chess,
Arena, Banksia, or En Croissant. It also accepts commands on the console:

    uci                       print the engine name and options
    position startpos moves e2e4 e7e5
    go depth 20               search to a fixed depth
    go wtime 60000 btime 60000 winc 1000 binc 1000
    bench [hash] [threads] [depth]
    perft 6                   count leaf nodes of the legal move tree
    d                         display the board
    eval                      print the static evaluation

Running `superpawn bench` from the command line searches a fixed set of
positions and prints the total node count. Identical builds give identical
counts, so the number acts as a signature of the search when reviewing
changes.

UCI options: `Threads`, `Hash` (MB), `Clear Hash`, `MultiPV`, `Move Overhead`.

Testing
-------

    ctest --test-dir build --output-on-failure

The unit tests cover bitboard primitives, FEN parsing, make and unmake,
hash key consistency, the perft suite including the standard edge cases for
en passant, castling and promotion, static exchange evaluation, and a few
searches that must find short mates.

Contributing
------------

Search and evaluation changes are accepted on the basis of SPRT test results
against the previous version, in the usual way for chess engines. The tooling
for this is part of the roadmap in [DESIGN.md](DESIGN.md). Format code with
`clang-format` using the repository's `.clang-format`.
