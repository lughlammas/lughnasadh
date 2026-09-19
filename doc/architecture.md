# Architecture

```
uci.cpp        UCI loop / options
search.cpp     Iterative deepening, alpha-beta, TT, LMR, qsearch
evaluate.cpp   Tapered eval (material, PST, mobility, king safety, passers, bishop pair)
movegen.cpp    Move generation
position.cpp   Board state, make/unmake
bitboard.cpp   Bitboard helpers
tt.cpp         Transposition table
main.cpp       Entry
```

Data flow: GUI/tool → UCI text → `Position` → `search` → bestmove.

Version identity: `id name Lughnasadh <ver> <Harvest>`.
