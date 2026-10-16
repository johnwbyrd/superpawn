Networks
========

This directory holds the NNUE network embedded into the engine.

`NETWORK` names the project's current network and its SHA-256. When that
file is not present here, the build downloads it from the `nets` GitHub
release and verifies the checksum. `tools/train/publish_net.sh` updates
`NETWORK` and uploads a new network.

A local `default.nnue` takes precedence: drop a candidate there and the next
build embeds it instead, which is how a new network is tested before it is
published. Point `SP_EMBED_NET` at another file to embed that one, or set it
to `OFF` to embed nothing. A network can also be loaded at run time with the
`EvalFile` UCI option.

With no network available the engine uses its hand-crafted evaluation and
says so in an `info string` at startup.

The file format and the network architecture are documented in `src/nnue.h`.
Networks are produced by the trainer in `tools/train/`.

Network files themselves are not committed to this repository.
