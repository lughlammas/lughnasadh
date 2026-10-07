# Lughnasadh 0.4.0 — Technical Validation

**Project:** Lughnasadh  
**Release evaluated:** 0.4.0 — *Fourth Harvest*  
**Evaluation date:** 7 October 2026  
**Scope:** Independent technical audit of the supplied `Engine 0.4.zip` package

## Purpose

This document summarizes the technical evidence collected during an audit of Lughnasadh 0.4.0.

It is intended as **portfolio and engineering evidence**: a concise record of what was actually inspected, compiled, executed, measured, and reproduced.

It is **not** an absolute Elo certification, a commercial valuation, or a claim that the engine is defect-free.

---

## Executive Summary

Lughnasadh 0.4.0 is a functional original UCI chess engine written in C++20.

The evaluated build:

- compiled and completed full games on Linux x86-64;
- passed all 27 perft tests used in the audit;
- matched an independent chess library across 1,498 valid differential-test positions;
- completed 80 controlled games against earlier Lughnasadh versions;
- showed a favorable result against both 0.2 and 0.3;
- solved all 24 generated short-mate test positions;
- exposed a substantial classical chess-engine architecture including search, evaluation, move generation, transposition tables, and UCI integration.

The audit also identified reproducible edge cases and robustness defects. Their inclusion is intentional: the goal of the evaluation was technical verification rather than promotional scoring.

---

## Evaluated Artifact

The evaluated ZIP had the following SHA-256:

```text
a4c5a483a66f7df425922a23d7f09167ff968e1025efecc137e18760e61aa14c
```

The engine identified itself through UCI as:

```text
Lughnasadh 0.4.0 Fourth Harvest
```

The core package contained 16 C++ source/header files, totaling approximately 2,412 lines in the evaluated engine core.

The strength measurements described below were performed on the **original 0.4 package received for evaluation**.

---

## Validation Summary

| Area | Result |
|---|---|
| Linux x86-64 compilation | Passed |
| Full-game execution | Passed |
| Perft test cases | 27 / 27 passed |
| Differential valid positions | 1,498 checked |
| Differential move transitions | 46,511 checked |
| Illegal moves in 80 match games | 0 observed |
| Short tactical mate set | 24 / 24 solved |
| Relative match vs 0.2 | 24.5 / 40 |
| Relative match vs 0.3 | 23.5 / 40 |
| Absolute Elo | Not determined |

---

## Rules and Move-Generation Validation

The audit executed **27 perft tests**, all with correct reference counts.

Selected deepest verified counts:

| Position | Depth | Leaf nodes |
|---|---:|---:|
| Initial position | 6 | 119,060,324 |
| Kiwipete | 5 | 193,690,690 |
| Rook-and-pawn ending reference | 6 | 11,030,083 |
| Castling/check/promotion reference | 5 | 15,833,292 |
| Tactical/promotion reference | 5 | 89,941,194 |

Across the complete perft set, the reported leaf-node counts summed to **442,133,692**, with overlap between tests.

Perft validates the legal move tree. It does not measure playing strength, but it provides strong evidence for the correctness of the engine's rule-handling foundation.

### Differential validation

The engine was also compared against an independent chess library over **1,498 valid positions**.

The audit checked **46,511 move transitions**, including:

- legal move lists;
- resulting board state;
- side to move;
- castling rights;
- counters;
- recomputed internal hash consistency;
- restoration after undo.

No divergence was observed in those valid-position checks.

---

## Relative Playing-Strength Evidence

Lughnasadh 0.4.0 played **80 controlled games** against earlier versions from the same engine family.

| Opponent | Wins | Draws | Losses | Score |
|---|---:|---:|---:|---:|
| Lughnasadh 0.2 reference | 21 | 7 | 12 | 24.5 / 40 (61.25%) |
| Lughnasadh 0.3 reference | 19 | 9 | 12 | 23.5 / 40 (58.75%) |

The matches used 20 opening families, with colors reversed for each pairing.

The earlier references received only a promotion-notation correction required to avoid communication failures; evaluation features from 0.4 were not backported.

Across the games, the arbiter processed **12,039 half-moves after the opening sequences** with no illegal move or execution error recorded.

These results provide evidence of **relative progress within the Lughnasadh family**.

They do **not** establish an absolute Elo rating or prove superiority at every time control, hardware configuration, or opponent set.

---

## Tactical Sanity Check

A fixed-seed test generated 24 independently verified short-mate positions:

- 12 mate-in-one positions;
- 12 mate-in-two positions.

Lughnasadh 0.4.0 solved:

```text
24 / 24
```

This confirms basic short tactical finishing ability.

It should not be interpreted as a general tactical rating or as coverage of deep combinations, defensive studies, zugzwang, or complex middlegames.

---

## Example Search Measurements

Observed completed depths from selected fixed-time tests:

| Position | Requested time | Observed time | Last completed depth |
|---|---:|---:|---:|
| Initial position | 1 s | 0.951 s | 14 |
| Initial position | 5 s | 4.953 s | 17 |
| Kiwipete | 1 s | 0.744 s | 9 |
| Kiwipete | 5 s | 4.955 s | 11 |
| Rook ending | 1 s | 0.875 s | 15 |
| Rook ending | 5 s | 3.894 s | 20 |
| Promotion position | 1 s | 0.955 s | 12 |
| Promotion position | 5 s | 3.888 s | 14 |

Nominal search depth is not directly comparable across different engines because reductions, extensions, pruning, and stopping conditions differ.

---

## Engine Architecture Observed

The evaluated source contained a classical chess-engine architecture including:

- bitboards and magic-bitboard move generation;
- Zobrist hashing;
- make / unmake move handling;
- transposition table;
- iterative deepening;
- aspiration windows;
- alpha-beta search;
- null-move pruning;
- late-move reductions;
- history and killer-move ordering;
- static exchange evaluation (SEE);
- quiescence search;
- phase-interpolated evaluation;
- UCI protocol integration.

These components are evidence of substantive engine-development work.

Their presence alone does not imply a specific Elo rating.

---

## Audit Integrity and Known Limitations

The audit was intentionally adversarial rather than promotional.

It identified reproducible edge cases involving, among other areas:

- immediate UCI search cancellation;
- quiescence behavior in check and terminal positions;
- non-capturing promotions at the quiescence horizon;
- en-passant discovered-check prediction;
- very-low-time clock allocation;
- extremely long UCI move histories;
- invalid numeric input handling;
- incomplete support for some optional UCI behaviors.

These findings do not invalidate the successful rule, match, tactical, and architectural evidence above.

They demonstrate that the evaluation included failure-oriented testing rather than only successful demonstrations.

The audit did not claim:

- an absolute Elo rating;
- Stockfish-class strength;
- complete UCI feature coverage;
- absence of all defects;
- a commercial valuation.

---

## Evaluation Environment

The principal audit environment was:

- Linux x86-64 virtualized;
- AMD EPYC 9V74 reported CPU;
- 9 logical CPUs exposed to the environment;
- GCC 13.3.0;
- C++20;
- `-O3`;
- `-pthread`;
- single-threaded engine search;
- 16 MB requested hash for match testing.

AddressSanitizer and UndefinedBehaviorSanitizer were used for selected robustness checks.

Android, Windows, macOS, and specific third-party GUI integrations were outside the executed runtime scope of this audit.

---

## What This Validation Demonstrates

For portfolio purposes, the 0.4.0 audit provides direct evidence of work across several engineering areas:

**Algorithms and data structures**
- game-tree search;
- pruning and reductions;
- hashing;
- move ordering;
- board-state representation.

**Correctness validation**
- perft;
- differential testing;
- state restoration;
- tactical fixtures.

**Experimental methodology**
- paired openings;
- controlled historical comparisons;
- PGN verification;
- explicit uncertainty and scope limits.

**Systems and protocol work**
- UCI command handling;
- asynchronous search control;
- native C++ build and runtime behavior.

**Engineering review**
- sanitizer-assisted robustness testing;
- reproducible defect isolation;
- separation between observed fact, inference, and hypothesis.

---

## Interpretation

The audit supports the following conservative statement:

> **Lughnasadh 0.4.0 is a functional, original C++20 UCI chess engine with validated move-generation foundations, measurable improvement over earlier Lughnasadh versions under the tested conditions, and a substantive classical search/evaluation architecture.**

The evidence supports describing Lughnasadh as a serious engineering project.

It does not support assigning an absolute competitive rating without external calibrated opponents and a larger testing program.

---

## Evidence Package

The original evaluation also produced an evidence bundle containing:

- 80 PGNs;
- per-move logs;
- statistical summaries;
- perft results;
- differential-test data;
- tactical positions and solutions;
- UCI transcripts;
- benchmark outputs;
- sanitizer diagnostics;
- Python test scripts;
- a C++ probe using the original engine functions;
- reference copies of earlier Lughnasadh versions;
- build/reproduction instructions.

This document is a summary of that evaluation, not a replacement for the underlying evidence.

---

## Status Note

The technical measurements above refer to the evaluated `Engine 0.4.zip` artifact identified by its SHA-256.

Subsequent repository work may improve documentation, CI, packaging, branding, or release organization. Such later repository changes should not be retroactively described as part of the measured evaluation unless they are separately re-tested.
