# Lughnasadh 0.3.0 — Third Harvest

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

- `id name Lughnasadh 0.3.0 Third Harvest`
- `id author Guilherme / Lugh Labs`

### Options

| Option | Default | Notes |
|--------|---------|--------|
| Hash | 16 | MB transposition table |
| Threads | 1 | Single-threaded search in 0.3.0 |
| Move Overhead | 50 | ms subtracted from clock |
| Clear Hash | button | Clears TT |

### Search features

Bitboards · iterative deepening · TT · null-move · killers · history · LMR · quiescence + SEE · tapered eval (material, PST, mobility, king safety, passed pawns).

### Honesty

This is a **classical** educational engine. It is far weaker than Stockfish / NNUE engines. Use it for learning, testing GUIs, and casual analysis — not as a claim of elite strength.

### Android (optional packaging)

See [`android/README.md`](android/README.md) for notes on wrapping the native binary as an engine APK (`enginelist` + `.so`), matching the 0.1.0 First Harvest layout.

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

MIT — Guilherme Cavalcanti / Lugh Labs.
