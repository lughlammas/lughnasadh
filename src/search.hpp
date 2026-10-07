#pragma once
#include "position.hpp"
#include "movegen.hpp"
#include <atomic>
#include <chrono>
#include <string>

namespace lugh {

struct Limits {
    int depth = 0;
    int movetime = 0;
    int wtime = 0, btime = 0, winc = 0, binc = 0;
    int movestogo = 0;
    uint64_t nodes = 0;
    bool infinite = false;
    bool perft = false;
    int perft_depth = 0;
};

struct SearchState {
    std::atomic<bool> stop{false};
    std::atomic<uint64_t> nodes{0};
    Move best_move = Move::null();
    Move ponder_move = Move::null();
    Value best_score = 0;
    int seldepth = 0;
};

extern SearchState Search;

void init_search();
void clear_search();
void start_search(Position& pos, const Limits& limits);
uint64_t perft(Position& pos, int depth);

} // namespace lugh
