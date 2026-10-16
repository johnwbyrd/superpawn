![Superpawn logo designed by Angela M. Eads](logo/logo-medium.jpg "Superpawn logo designed by Angela M. Eads")

Superpawn
=========

Superpawn is a [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface)
chess engine written in C++23, rewritten from the ground up for strength
(the previous engine is preserved on the `feature/old` branch): bitboard move generation, a lockless shared
transposition table, Lazy SMP parallel search, a modern alpha-beta search with
the full set of pruning, reduction and extension heuristics, and an NNUE
evaluator trained on our own hardware.

Superpawn is free software under the GNU Affero General Public License,
version 3 or later. See [LICENSE](LICENSE).

Status
------

The engine generates moves correctly (verified against the standard perft
suite), speaks UCI, plays legal games on the clock with any number of
threads, and has SPRT tooling, an NNUE evaluator with incremental updates,
and the data generation and training pipeline for it
([tools/train/README.md](tools/train/README.md)). No trained network has
been produced yet, so builds fall back to a placeholder hand-crafted
evaluation until one is. See [DESIGN.md](DESIGN.md) for the architecture and
the roadmap.

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
| `SP_EMBED_NET`  | `nets/default.nnue` | NNUE network file to embed in the binary, or `OFF`     |

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
    datagen data.bin --threads 8 --nodes 5000 --positions 100000000
                              generate NNUE training data by self-play
    convert quantised.bin net.nnue
                              convert a bullet export to the engine's network format

Running `superpawn bench` from the command line searches a fixed set of
positions and prints the total node count. Identical builds give identical
counts, so the number acts as a signature of the search when reviewing
changes.

UCI options: `Threads`, `Hash` (MB), `Clear Hash`, `MultiPV`, `Move Overhead`,
`EvalFile` (path to an NNUE network; the default is the embedded one).

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
against the previous version, in the usual way for chess engines:

    tools/sprt/sprt.sh --base origin/main

builds both versions, fetches fastchess and an opening book on first use,
and runs the test. See [tools/sprt/README.md](tools/sprt/README.md) for the
time controls and bounds. Format code with `clang-format` using the
repository's `.clang-format`.
