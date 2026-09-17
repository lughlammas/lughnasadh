#include "bitboard.hpp"
#include "position.hpp"
#include "uci.hpp"
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    lugh::init_bitboards();
    lugh::init_zobrist();
    lugh::uci_loop();
    return 0;
}
