# Superpawn

UCI chess engine in C++23, AGPL-3.0. `main` is a complete rewrite; the
previous engine lives on `feature/old` and nothing from it is used.

## Build and test

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ./build/superpawn_tests          # doctest unit tests, about a second
    ./build/superpawn bench          # depth 13 bench; the node count is the search signature
    ./build/superpawn bench 16 1 8   # faster variant for quick checks
    tools/sprt/sprt.sh --base HEAD   # SPRT of the working tree against a commit (see tools/sprt/README.md)
    ./build/superpawn datagen out.bin --threads 4 --nodes 5000 --games 100   # self-play training data
    ./build/superpawn convert quantised.bin net.nnue                          # bullet export -> engine format

NNUE training lives in tools/train (bullet, GPU only); see its README. The
architecture constants in tools/train/src/main.rs must match src/nnue.h.

Debug builds enable `assert()` including `Position::pos_is_ok()` after every
move; run the perft tests in a debug build after touching `position.cpp` or
`movegen.cpp`.

## Conventions

- Format with `clang-format` using `.clang-format`.
- File headers say "Superpawn, a UCI chess engine" with no version number.
- Every search heuristic carries a comment explaining what it does and why.
- Search and eval changes must not change `bench` unless intended; when they
  do, state the new bench count in the commit message.
- Functional changes to search or evaluation are accepted on SPRT results,
  not on intuition.
- Mate scores are relative to the root; `value_to_tt`/`value_from_tt` convert
  for the table. Keep `VALUE_TB*` thresholds in `types.h` consistent.

## Layout

See `DESIGN.md` for the architecture and roadmap.
