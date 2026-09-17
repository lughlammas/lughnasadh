#pragma once
#include "position.hpp"

namespace lugh {

enum GenType { CAPTURES, QUIETS, EVASIONS, NON_EVASIONS, LEGAL };

template<GenType> void generate(const Position& pos, MoveList& list);

inline void generate_legal(const Position& pos, MoveList& list) {
    generate<LEGAL>(pos, list);
}

} // namespace lugh
