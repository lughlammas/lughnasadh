#include "tt.hpp"
#include <algorithm>
#include <cstring>

namespace lugh {

TranspositionTable TT;

void TranspositionTable::resize(size_t mb) {
    size_t n = (mb * 1024 * 1024) / sizeof(TTEntry);
    if (n < 1) n = 1;
    // keep power of two
    size_t p = 1;
    while (p * 2 <= n) p *= 2;
    table.assign(p, TTEntry{});
    age = 0;
}

void TranspositionTable::clear() {
    std::memset(table.data(), 0, table.size() * sizeof(TTEntry));
    age = 0;
}

TTEntry* TranspositionTable::probe(Key key, bool& hit) const {
    if (table.empty()) { hit = false; return nullptr; }
    TTEntry* e = const_cast<TTEntry*>(&table[key & (table.size() - 1)]);
    hit = (e->key == key);
    return e;
}

void TranspositionTable::store(Key key, Move m, Value score, Value eval,
                               TTBound bound, Depth depth, int ply) {
    if (table.empty()) return;
    TTEntry& e = table[key & (table.size() - 1)];
    if (e.key != key || depth + 2 >= e.depth || bound == BOUND_EXACT || e.age != age) {
        e.key = key;
        e.move = m;
        e.score = int16_t(to_tt(score, ply));
        e.eval = int16_t(eval);
        e.depth = uint8_t(std::max(0, int(depth)));
        e.bound = bound;
        e.age = age;
    }
}

size_t TranspositionTable::hashfull() const {
    if (table.empty()) return 0;
    size_t cnt = 0;
    size_t n = std::min(table.size(), size_t(1000));
    for (size_t i = 0; i < n; ++i)
        if (table[i].age == age && table[i].key) cnt++;
    return cnt;
}

} // namespace lugh
