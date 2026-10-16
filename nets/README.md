Networks
========

This directory holds the NNUE network embedded into the engine. The build
embeds `nets/default.nnue` when it exists; point `SP_EMBED_NET` at another
file to embed that one instead, or set it to `OFF` to embed nothing. A network
can also be loaded at run time with the `EvalFile` UCI option.

With no network available the engine uses its hand-crafted evaluation and
says so in an `info string` at startup.

The file format and the network architecture are documented in `src/nnue.h`.
Networks are produced by the trainer in `tools/train/` (to come).

Network files are not committed to this repository. They are distributed with
releases.
