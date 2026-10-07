# Lughnasadh 0.4.0 — Fourth Harvest

A classical UCI chess engine written in C++20 by Guilherme Cavalcanti / Lugh Labs, maintained within **ARBOCK LABS**, an independent software and applied-AI lab currently being structured.

**Status:** experimental engine for study, interface integration, and chess analysis. The source includes Fourth Harvest search, evaluation, and UCI changes. It uses classical evaluation, not NNUE; no playing-strength or benchmark claim is made here.

## Build

Requires CMake 3.16 or newer and a C++20 compiler with standard threading support.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

On Linux/macOS the executable is `build/lughnasadh`. With a multi-configuration Windows generator, use `cmake --build build --config Release`; the executable is normally `build/Release/lughnasadh.exe`.

## UCI interface

Run the executable in a terminal or configure it as an engine in a UCI-compatible chess interface.

```text
uci
isready
ucinewgame
position startpos moves e2e4 e7e5
go depth 8
```

Wait for `bestmove` before sending `quit`. Search also accepts `movetime`, clock/increment fields, `movestogo`, `nodes`, and `infinite`; `stop` requests termination of the search thread.

Identification: `id name Lughnasadh 0.4.0 Fourth Harvest`.

| Option | Default | Scope |
|---|---|---|
| Hash | 16 MB | Transposition-table allocation |
| Threads | 1 | One search worker; this is not parallel search |
| Move Overhead | 50 ms | Clock allowance |
| Clear Hash | button | Clear the transposition table |

## Architecture

- `src/bitboard.*`, `src/position.*`, `src/movegen.*`: board representation, make/unmake, and legal move generation.
- `src/evaluate.*`: tapered classical evaluation, including material, piece-square tables, pawn structure, mobility, and king safety.
- `src/search.*`: iterative deepening and alpha-beta search with transposition tables, aspiration windows, pruning/reductions, and quiescence search.
- `src/tt.*`: transposition table.
- `src/uci.*`: text protocol, options, position input, and asynchronous search lifecycle.

See [architecture](doc/architecture.md), [UCI notes](doc/uci.md), [NEWS](NEWS), and [ChangeLog](ChangeLog).

## Validation

The implementation exposes a synchronous `perft` command:

```text
position startpos
perft 5
quit
```

The standard start-position reference count at depth 5 is 4,865,609. This is a move-generation reference, not a strength or speed measurement. The repository build check compiles the engine and checks UCI identification/readiness and this perft result. No historical match result is presented as a reproducible benchmark.

## Android source helpers

[Android notes](android/README.md) describe the optional arm64 cross-build and legacy APK packaging helpers. Packaging requires an external reference APK skeleton, an Android NDK, and locally configured signing credentials. An Android 0.4.0 APK release is not included or claimed. Build output and signing material are excluded from source control.

## Authorship and license

Original attribution and available Git history are preserved in [AUTHORS](AUTHORS) and the commit history. Lughnasadh retains its existing [MIT license](LICENSE); this consolidation does not introduce a new license.
