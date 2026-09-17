#include "search.hpp"
#include "evaluate.hpp"
#include "tt.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <thread>

namespace lugh {

SearchState Search;

namespace {

constexpr int MAX_HISTORY = 10000;

Move killers[MAX_PLY][2];
int history[COLOR_NB][SQUARE_NB][SQUARE_NB];
int counter_move[SQUARE_NB][SQUARE_NB]; // unused simple

Limits limits;
std::chrono::steady_clock::time_point start_time;
int allocated_ms = 0;
Position* root_pos = nullptr;
StateInfo states[MAX_PLY + 64];
Move root_moves[MAX_MOVES];
int root_move_count = 0;
int root_depth = 0;

Value mate_in(int ply) { return VALUE_MATE - ply; }
Value mated_in(int ply) { return -VALUE_MATE + ply; }

bool time_up() {
    if (Search.stop) return true;
    if (limits.infinite || limits.depth) {
        if (limits.depth && !limits.movetime && !limits.wtime) return false;
    }
    if (allocated_ms <= 0) return false;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();
    return ms >= allocated_ms;
}

void allocate_time(const Position& pos) {
    allocated_ms = 0;
    if (limits.movetime) {
        allocated_ms = std::max(1, limits.movetime - 50);
        return;
    }
    int time = pos.side_to_move() == WHITE ? limits.wtime : limits.btime;
    int inc = pos.side_to_move() == WHITE ? limits.winc : limits.binc;
    if (time <= 0) return;
    int mtg = limits.movestogo > 0 ? limits.movestogo : 30;
    allocated_ms = time / mtg + inc / 2;
    allocated_ms = std::max(10, std::min(allocated_ms, time / 2));
}

int mvv_lva(const Position& pos, Move m) {
    static const int V[8] = {0,100,300,300,500,900,0};
    if (m.type() == EN_PASSANT) return 105;
    if (m.type() == PROMOTION) return 800 + V[m.promotion_type()];
    Piece victim = pos.piece_on(m.to());
    Piece attacker = pos.piece_on(m.from());
    if (victim == NO_PIECE) return 0;
    return V[type_of(victim)] * 16 - V[type_of(attacker)];
}

void score_moves(Position& pos, MoveList& list, Move tt_move, int ply) {
    Color us = pos.side_to_move();
    for (auto& em : list) {
        Move m = em.move;
        if (m == tt_move) em.score = 2000000;
        else if (pos.is_capture(m) || m.type() == PROMOTION) {
            em.score = 1000000 + mvv_lva(pos, m);
            if (pos.is_capture(m) && !pos.see_ge(m, 0))
                em.score -= 500000;
        } else if (m == killers[ply][0]) em.score = 900000;
        else if (m == killers[ply][1]) em.score = 800000;
        else em.score = history[us][m.from()][m.to()];
    }
}

Move pick_best(MoveList& list, int start) {
    int best = start;
    for (int i = start + 1; i < list.size; ++i)
        if (list.moves[i].score > list.moves[best].score) best = i;
    std::swap(list.moves[start], list.moves[best]);
    return list.moves[start].move;
}

Value quiescence(Position& pos, StateInfo* st_stack, int ply, Value alpha, Value beta) {
    Search.nodes++;
    if ((Search.nodes & 2047) == 0 && time_up()) {
        Search.stop = true;
        return 0;
    }
    if (ply >= MAX_PLY - 1) return evaluate(pos);

    Value stand = evaluate(pos);
    if (stand >= beta) return stand;
    if (stand > alpha) alpha = stand;

    MoveList list;
    if (pos.in_check()) {
        generate_legal(pos, list);
        if (list.size == 0) return mated_in(ply);
    } else {
        generate<CAPTURES>(pos, list);
        // Filter illegal
        MoveList legal;
        for (auto& em : list)
            if (pos.legal(em.move)) legal.push(em.move);
        list = legal;
    }

    score_moves(pos, list, Move::null(), ply);
    for (int i = 0; i < list.size; ++i) {
        Move m = pick_best(list, i);
        if (!pos.in_check()) {
            // Delta / SEE prune
            if (!pos.see_ge(m, Value(alpha - stand - 200)))
                continue;
        }
        pos.do_move(m, st_stack[ply]);
        Value score = -quiescence(pos, st_stack, ply + 1, -beta, -alpha);
        pos.undo_move(m);
        if (Search.stop) return 0;
        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

Value search_node(Position& pos, StateInfo* st_stack, int ply, Depth depth,
                  Value alpha, Value beta, bool cut_node);

Value search_node(Position& pos, StateInfo* st_stack, int ply, Depth depth,
                  Value alpha, Value beta, bool cut_node) {
    const bool root = (ply == 0);
    const bool pv = (beta - alpha > 1);
    Search.nodes++;
    Search.seldepth = std::max(Search.seldepth, ply);

    if ((Search.nodes & 2047) == 0 && time_up()) {
        Search.stop = true;
        return 0;
    }

    if (!root) {
        if (pos.is_draw(ply)) return VALUE_DRAW;
        alpha = std::max(alpha, mated_in(ply));
        beta = std::min(beta, mate_in(ply + 1));
        if (alpha >= beta) return alpha;
    }

    if (depth <= 0)
        return quiescence(pos, st_stack, ply, alpha, beta);
    if (ply >= MAX_PLY - 1) return evaluate(pos);

    // Mate distance already handled

    bool tt_hit = false;
    TTEntry* tte = TT.probe(pos.key(), tt_hit);
    Move tt_move = (tt_hit && tte) ? tte->move : Move::null();
    Value tt_score = VALUE_NONE;
    if (tt_hit && tte) {
        tt_score = Value(tte->score);
        if (tt_score >= VALUE_MATE_IN_MAX) tt_score -= ply;
        else if (tt_score <= -VALUE_MATE_IN_MAX) tt_score += ply;
        if (!pv && tte->depth >= depth) {
            if (tte->bound == BOUND_EXACT) return tt_score;
            if (tte->bound == BOUND_LOWER && tt_score >= beta) return tt_score;
            if (tte->bound == BOUND_UPPER && tt_score <= alpha) return tt_score;
        }
    }

    Value eval = evaluate(pos);
    bool improving = true; // simplified

    // Null move pruning
    if (!pv && !pos.in_check() && depth >= 3
        && eval >= beta
        && popcount(pos.pieces(pos.side_to_move()) & ~pos.pieces(PAWN) & ~pos.pieces(KING)) > 0
        && pos.state()->plies_from_null > 0) {
        Depth R = 3 + depth / 4;
        pos.do_null_move(st_stack[ply]);
        Value null_score = -search_node(pos, st_stack, ply + 1, depth - R - 1, -beta, -beta + 1, !cut_node);
        pos.undo_null_move();
        if (Search.stop) return 0;
        if (null_score >= beta)
            return null_score >= VALUE_MATE_IN_MAX ? beta : null_score;
    }

    MoveList list;
    generate_legal(pos, list);
    if (list.size == 0)
        return pos.in_check() ? mated_in(ply) : VALUE_DRAW;

    score_moves(pos, list, tt_move, ply);

    Move best_move = Move::null();
    Value best = -VALUE_INFINITE;
    int move_count = 0;
    TTBound bound = BOUND_UPPER;

    for (int i = 0; i < list.size; ++i) {
        Move m = pick_best(list, i);
        if (root) {
            // skip if not in root moves? all legal
        }
        move_count++;

        bool capture = pos.is_capture(m) || m.type() == PROMOTION;
        bool gives_check = pos.gives_check(m);

        Depth new_depth = depth - 1;

        // LMR
        Depth reduction = 0;
        if (!root && move_count > 3 && depth >= 3 && !capture && !gives_check && !pos.in_check()) {
            reduction = 1;
            if (move_count > 6) reduction++;
            if (cut_node) reduction++;
            if (reduction > new_depth - 1) reduction = new_depth - 1;
        }

        pos.do_move(m, st_stack[ply]);
        Value score;
        if (move_count == 1) {
            score = -search_node(pos, st_stack, ply + 1, new_depth, -beta, -alpha, false);
        } else {
            score = -search_node(pos, st_stack, ply + 1, new_depth - reduction, -alpha - 1, -alpha, true);
            if (reduction && score > alpha)
                score = -search_node(pos, st_stack, ply + 1, new_depth, -alpha - 1, -alpha, !cut_node);
            if (score > alpha && score < beta)
                score = -search_node(pos, st_stack, ply + 1, new_depth, -beta, -alpha, false);
        }
        pos.undo_move(m);
        if (Search.stop) return 0;

        if (score > best) {
            best = score;
            best_move = m;
            if (score > alpha) {
                alpha = score;
                bound = BOUND_EXACT;
                if (root) {
                    Search.best_move = m;
                    Search.best_score = score;
                }
                if (!capture) {
                    history[pos.side_to_move()][m.from()][m.to()] =
                        std::min(MAX_HISTORY, history[pos.side_to_move()][m.from()][m.to()] + depth * depth);
                }
                if (score >= beta) {
                    bound = BOUND_LOWER;
                    if (!capture) {
                        killers[ply][1] = killers[ply][0];
                        killers[ply][0] = m;
                    }
                    break;
                }
            }
        }
    }

    TT.store(pos.key(), best_move, best, eval, bound, depth, ply);
    return best;
}

uint64_t perft_inner(Position& pos, StateInfo* st_stack, int depth, int ply) {
    if (depth == 0) return 1;
    MoveList list;
    generate_legal(pos, list);
    if (depth == 1) return list.size;
    uint64_t nodes = 0;
    for (auto& em : list) {
        pos.do_move(em.move, st_stack[ply]);
        nodes += perft_inner(pos, st_stack, depth - 1, ply + 1);
        pos.undo_move(em.move);
    }
    return nodes;
}

} // namespace

void init_search() {
    std::memset(killers, 0, sizeof(killers));
    std::memset(history, 0, sizeof(history));
}

void clear_search() {
    init_search();
    TT.clear();
}

uint64_t perft(Position& pos, int depth) {
    StateInfo st_stack[MAX_PLY];
    return perft_inner(pos, st_stack, depth, 0);
}

void start_search(Position& pos, const Limits& lim) {
    limits = lim;
    Search.stop = false;
    Search.nodes = 0;
    Search.seldepth = 0;
    Search.best_move = Move::null();
    Search.best_score = 0;
    start_time = std::chrono::steady_clock::now();
    allocate_time(pos);
    TT.new_search();

    if (limits.perft) {
        auto t0 = start_time;
        uint64_t n = perft(pos, limits.perft_depth);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
        std::cout << "info string perft " << limits.perft_depth << " nodes " << n
                  << " time " << ms << std::endl;
        std::cout << "bestmove 0000" << std::endl;
        return;
    }

    MoveList list;
    generate_legal(pos, list);
    if (list.size == 0) {
        std::cout << "bestmove 0000" << std::endl;
        return;
    }
    Search.best_move = list.moves[0].move;

    StateInfo st_stack[MAX_PLY + 8];
    int max_d = limits.depth > 0 ? limits.depth : 64;
    Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;

    for (int depth = 1; depth <= max_d; ++depth) {
        root_depth = depth;
        Value score = search_node(pos, st_stack, 0, depth, alpha, beta, false);
        if (Search.stop && depth > 1) break;

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        std::cout << "info depth " << depth
                  << " seldepth " << Search.seldepth
                  << " score cp " << score
                  << " nodes " << Search.nodes
                  << " time " << ms
                  << " pv " << move_str(Search.best_move)
                  << std::endl;

        if (limits.depth && depth >= limits.depth) break;
        if (time_up()) break;
        // Soft time: stop if used most of allocation
        if (allocated_ms > 0 && ms > allocated_ms * 7 / 10 && depth >= 6) break;
    }

    std::cout << "bestmove " << move_str(Search.best_move) << std::endl;
}

} // namespace lugh
