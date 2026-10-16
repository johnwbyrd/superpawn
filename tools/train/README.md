Training the NNUE
=================

The network is trained with [bullet](https://github.com/jw1912/bullet) on
positions the engine generates by playing itself. The whole loop is:

    generate data  ->  shuffle  ->  train  ->  convert  ->  embed  ->  SPRT

Everything below runs on the training machine, **from the repository root**.
Paths are examples.

Requirements
------------

- The engine built in `build/` (`cmake -S . -B build && cmake --build build`).
- Rust via [rustup](https://rustup.rs).
- For training, the CUDA toolkit including nvrtc, installed from
  https://developer.nvidia.com/cuda-downloads so that it lives in
  `/usr/local/cuda` with `include/` and `lib64/` inside. bullet needs
  `CUDA_PATH` to point there; `tools/train/train.sh` finds it and sets it.
  If you see `CUDA_PATH is not defined`, that is the missing piece.
  AMD GPUs use `SP_TRAIN_FEATURES=rocm` with `HIP_PATH`, Apple Silicon
  `SP_TRAIN_FEATURES=metal`.

1. Generate data
----------------

    build/superpawn datagen data/run1.bin --threads 15 --nodes 5000 --positions 200000000

Each thread plays games on its own: 8 or 9 random opening plies, then fixed
node searches for both sides. Openings scored beyond 10 pawns are discarded,
games are adjudicated after four plies beyond 20 pawns or at a forced mate,
and drawn by repetition, the fifty-move rule, insufficient material or 400
plies. Quiet positions (not in check, best move not a capture or promotion)
are written as 32-byte bulletformat `ChessBoard` records with the search
score and the game result. The file is appended to, so a run can be resumed
or several runs merged by pointing at the same file; it prints progress every
ten seconds and Ctrl-C stops it cleanly at a game boundary.

Expect a few hundred positions per second per thread at 5000 nodes. A first
network needs about 100 million positions; later ones gain from several
hundred million and from regenerating data with the latest network.

Other options: `--games N`, `--seed N`, `--random-plies N`, `--max-ply N`,
`--hash MB`.

2. Shuffle
----------

Training data must be shuffled, since positions from one game are strongly
correlated. Build `bullet-utils` once (no GPU needed):

    git clone --depth 1 https://github.com/jw1912/bullet ../bullet
    (cd ../bullet && cargo build --release --package bullet-utils)

Then:

    ../bullet/target/release/bullet-utils validate -i data/run1.bin
    ../bullet/target/release/bullet-utils shuffle -i data/run1.bin -o data/run1.shuffled.bin -m 8000
    ../bullet/target/release/bullet-utils interleave data/a.shuffled.bin data/b.shuffled.bin -o data/all.bin

`-m` is the memory to use in MB. `interleave` merges several shuffled files.

3. Train
--------

    tools/train/train.sh --data data/run1.shuffled.bin --id spn-001 --superbatches 40

The script locates the CUDA toolkit, builds the trainer in release mode and
runs it. A superbatch is about 100 million positions; a 4090 does one for
this network in well under a minute. Checkpoints land in
`tools/train/checkpoints/<id>-<n>/`.

Defaults: learning rate 0.001 with cosine decay to 0.3^5 of that, WDL
weight 0.1 (the loss target is 0.1 x game result + 0.9 x sigmoid of the
search score), AdamW. `--lr`, `--wdl`, `--threads`, `--out` and `--resume
<checkpoint dir>` adjust them. `--data` may be repeated.

**Choosing the WDL weight.** The game result is only a useful label when the
games are good. Results of fixed-node games between weak versions are
decided by blunders, so a network trained mostly on results learns noise:
it ranks positions roughly right but stretches its outputs towards win or
loss, and plays worse than the evaluation that generated the data. Start
with a low weight such as 0.1 and raise it towards 0.5 as the engine and
its data improve. `evalstats` (below) shows whether a network predicts the
search scores better than the hand-crafted evaluation; a good first network
must.

**Passes over the data.** A superbatch is about 100 million positions, so
`--superbatches` times 100 million divided by the number of positions is how
many times each position is seen. Keep that below about 10 for small
datasets; the network can otherwise memorise games. More data beats more
passes.

The architecture constants at the top of `tools/train/src/main.rs` must
match `src/nnue.h` in the engine. Change them together.

4. Convert and embed
--------------------

    build/superpawn convert tools/train/checkpoints/spn-001-40/quantised.bin nets/spn-001.nnue

checks the output weights fit the 16-bit inference and writes the engine's
file format with its checksum. To try it without rebuilding:

    setoption name EvalFile value nets/spn-001.nnue

To embed it, copy it to `nets/default.nnue` (ignored by git) and rebuild,
or configure with `-DSP_EMBED_NET=/path/to/net.nnue`.

Checking a network
------------------

Before playing games, check that the network predicts the search scores in
the data better than the hand-crafted evaluation does:

    printf 'evalstats data/run1.bin 2000000\nquit\n' | build/superpawn

With the network embedded this prints the mean absolute error, RMSE and
correlation of both evaluations against two million stored scores. The
hand-crafted evaluation lands around 85 centipawns mean absolute error on
its own data; a usable network must be well below that. A network far above
it has learnt the wrong target (see the WDL weight above) or has not trained
enough. `python3 tools/train/check_net.py nets/default.nnue` evaluates a few
positions independently of the engine and prints weight statistics.

5. Test
-------

A new network is a functional change and goes through SPRT like anything
else. With the candidate at `nets/default.nnue` in the working tree:

    tools/sprt/sprt.sh --base origin/main --name spn-001

The first network is tested against the hand-crafted evaluation and should
win by a wide margin; later ones are tested against the previous network
with the usual bounds.

6. Publish
----------

A network that passed becomes the project's current one:

    tools/train/publish_net.sh nets/spn-002.nnue
    git add nets/NETWORK && git commit -m "Use network spn-002" && git push

The script records the file name and SHA-256 in `nets/NETWORK` and uploads
the file to the `nets` GitHub release with the `gh` CLI. Builds without a
local `nets/default.nnue` download and embed the network named there, so
remove or rename `nets/default.nnue` once the network is published. Network
files are never committed to git.

Public data
-----------

The pipeline also accepts any file in bulletformat, so public datasets
converted with `bullet-utils convert` can be mixed in. Self-play data from
the engine's own search is the simplest start and keeps the network matched
to the engine.
