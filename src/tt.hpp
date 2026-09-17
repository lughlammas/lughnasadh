#pragma once
#include "types.hpp"
#include <vector>
#include <cstring>

namespace lugh {

enum TTBound : uint8_t { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };

struct TTEntry {
    Key key = 0;
    Move move = Move::null();
    int16_t score = 0;
    int16_t eval = 0;
    uint8_t depth = 0;
    uint8_t bound = BOUND_NONE;
    uint8_t age = 0;
};

class TranspositionTable {
public:
    void resize(size_t mb);
    void clear();
    TTEntry* probe(Key key, bool& hit) const;
    void store(Key key, Move m, Value score, Value eval, TTBound bound, Depth depth, int ply);
    void new_search() { age = (age + 1) & 0xFF; }
    size_t hashfull() const;

private:
    static Value to_tt(Value v, int ply) {
        if (v >= VALUE_MATE_IN_MAX) return v + ply;
        if (v <= -VALUE_MATE_IN_MAX) return v - ply;
        return v;
    }
    static Value from_tt(Value v, int ply) {
        if (v >= VALUE_MATE_IN_MAX) return v - ply;
        if (v <= -VALUE_MATE_IN_MAX) return v + ply;
        return v;
    }
    std::vector<TTEntry> table;
    uint8_t age = 0;
};

extern TranspositionTable TT;

} // namespace lugh
