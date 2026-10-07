# Lughnasadh 0.4.0 — Fourth Harvest

**Documentation (GNU-style hierarchy):** see [`doc/README`](doc/README), `INSTALL`, `AUTHORS`, `NEWS`, `ChangeLog`, `COPYING`, `CONTRIBUTING`.

Classical UCI chess engine (bitboards + alpha-beta). Written from scratch for study and prep/training. **Not** a Stockfish fork and **not** Stockfish-strength.

> Prep / training only — not for live official championship assistance. Do not brand as cheating.

---

## English

### Build (Linux / macOS)

```bash
cmake -B build
cmake --build build -j
```

Binary: `build/lughnasadh`

### Run (UCI)

```bash
./build/lughnasadh
```

Smoke test:

```bash
printf 'uci\nisready\nposition startpos\ngo depth 8\nquit\n' | ./build/lughnasadh
```

Perft (startpos depth 5 = **4865609**):

```text
position startpos
go perft 5
```

or:

```text
perft 5
```

### UCI identity

- `id name Lughnasadh 0.4.0 Fourth Harvest`
- `id author Guilherme / Lugh Labs`

### Options

| Option | Default | Notes |
|--------|---------|--------|
| Hash | 16 | MB transposition table |
| Threads | 1 | Single search thread (0.4.0 runs it off the UCI thread, so `stop` works) |
| Move Overhead | 50 | ms subtracted from clock |
| Clear Hash | button | Clears TT |

### Search features

Bitboards · iterative deepening · aspiration windows · check extension · TT · null-move · killers · history · LMR · quiescence + SEE · dynamic contempt · tapered eval (material, PST, mobility, king safety, bishop pair, pawn structure, rooks on open files, passed pawns with king distance / free path / rule of the square, drawish-material scaling).

Limits: `go depth N | movetime N | nodes N | wtime/btime/winc/binc | infinite`, plus `stop`.

### Verification (0.4.0)

| Check | Expected | Result |
|---|---|---|
| `perft 5` from the start position | 4865609 | 4865609 |
| `perft 4` from Kiwipete (`r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`) | 4085603 | 4085603 |
| promotion output (`8/P7/8/8/8/8/8/k6K w - - 0 1`, `go depth 6`) | `a7a8q` | `a7a8q` |
| `go infinite` then `stop` | bestmove printed | yes |

Strength: 50 games vs 0.3.0 (+promotion fix) at 0.2 s/move = 24.5/50, i.e. no measurable strength change yet (see `NEWS`).

Note: `quit` stops a running search immediately; to let a scripted `go` finish, close stdin instead of sending `quit`.

### Honesty

This is a **classical** educational engine. It is far weaker than Stockfish / NNUE engines. Use it for learning, testing GUIs, and casual analysis — not as a claim of elite strength.

### Android (optional packaging)

See [`android/README.md`](android/README.md) for notes on wrapping the native binary as an engine APK (`enginelist` + `.so`), matching the 0.1.0 First Harvest layout. The last packaged APK is 0.2.0 Second Harvest (arm64); 0.4.0 has not been repackaged yet.

---

## Português

### Compilar

```bash
cmake -B build
cmake --build build -j
```

Binário: `build/lughnasadh`

### Executar

```bash
./build/lughnasadh
```

Teste rápido:

```bash
printf 'uci\nisready\nposition startpos\ngo depth 8\nquit\n' | ./build/lughnasadh
```

Perft (posição inicial, profundidade 5 = **4865609**):

```text
go perft 5
```

### Honestidade

Motor **clássico** (não NNUE). Feito para estudo e treino — **não** é nível Stockfish. Não usar como ajuda em campeonato oficial ao vivo; não promover como trapaça.

### Android

Notas de empacotamento em [`android/README.md`](android/README.md).

---

## License

MIT — Guilherme Cavalcanti / Lugh Labs (see `LICENSE`).
