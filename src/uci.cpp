#include "uci.hpp"
#include "position.hpp"
#include "search.hpp"
#include "tt.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <thread>
#include <vector>

namespace lugh {

namespace {
int hash_mb = 16;
int threads = 1;
int move_overhead = 50;
Position pos;
StateInfo root_state;
StateInfo move_states[1024];
int state_idx = 0;
std::thread search_thread;

Move parse_move(const Position& p, const std::string& token) {
    if (token.size() < 4) return Move::null();
    Square from = make_square(File(token[0] - 'a'), Rank(token[1] - '1'));
    Square to = make_square(File(token[2] - 'a'), Rank(token[3] - '1'));
    PieceType promo = KNIGHT;
    MoveType mt = NORMAL;
    if (token.size() >= 5) {
        mt = PROMOTION;
        switch (token[4]) {
        case 'n': promo = KNIGHT; break;
        case 'b': promo = BISHOP; break;
        case 'r': promo = ROOK; break;
        case 'q': promo = QUEEN; break;
        default: break;
        }
    }
    // Match against legal moves
    MoveList list;
    generate_legal(p, list);
    for (auto& em : list) {
        Move m = em.move;
        if (m.from() != from) continue;
        if (m.type() == CASTLING) {
            // Accept e1g1 / e1c1 style
            if (m.to() == to) return m;
            continue;
        }
        if (m.to() != to) continue;
        if (mt == PROMOTION) {
            if (m.type() == PROMOTION && m.promotion_type() == promo) return m;
        } else if (m.type() != PROMOTION) {
            return m;
        }
    }
    return Move::null();
}

void set_position(std::istringstream& is) {
    std::string token, fen;
    is >> token;
    state_idx = 0;
    bool have_moves = false;
    if (token == "startpos") {
        pos.set_startpos(&root_state);
        if (is >> token && token == "moves") have_moves = true;
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            if (!fen.empty()) fen += ' ';
            fen += token;
        }
        pos.set(fen, &root_state);
        if (token == "moves") have_moves = true;
    } else return;

    if (have_moves) {
        while (is >> token) {
            Move m = parse_move(pos, token);
            if (m.none()) break;
            state_idx++;
            pos.do_move(m, move_states[state_idx]);
        }
    }
}

void go_command(std::istringstream& is) {
    Limits lim;
    std::string token;
    while (is >> token) {
        if (token == "depth") is >> lim.depth;
        else if (token == "movetime") is >> lim.movetime;
        else if (token == "wtime") is >> lim.wtime;
        else if (token == "btime") is >> lim.btime;
        else if (token == "winc") is >> lim.winc;
        else if (token == "binc") is >> lim.binc;
        else if (token == "movestogo") is >> lim.movestogo;
        else if (token == "nodes") is >> lim.nodes;
        else if (token == "infinite") lim.infinite = true;
        else if (token == "perft") {
            lim.perft = true;
            is >> lim.perft_depth;
        }
    }
    // Apply move overhead to clock
    if (lim.wtime > move_overhead) lim.wtime -= move_overhead;
    if (lim.btime > move_overhead) lim.btime -= move_overhead;

    if (search_thread.joinable()) {
        Search.stop = true;
        search_thread.join();
    }
    search_thread = std::thread([&]() {
        // copy limits
        start_search(pos, lim);
    });
    // Problem: lim is local and thread captures by ref incorrectly with [&]
    // Fix: join immediately for simplicity (single-threaded search), or copy lim
}

} // namespace

void uci_loop() {
    pos.set_startpos(&root_state);
    TT.resize(hash_mb);
    init_search();

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::istringstream is(line);
        std::string token;
        is >> token;

        if (token == "uci") {
            std::cout << "id name Lughnasadh 0.4.0 Fourth Harvest\n"
                      << "id author Guilherme Cavalcanti / ARBOCK LABS\n"
                      << "option name Hash type spin default 16 min 1 max 4096\n"
                      << "option name Threads type spin default 1 min 1 max 1\n"
                      << "option name Move Overhead type spin default 50 min 0 max 5000\n"
                      << "option name Clear Hash type button\n"
                      << "uciok" << std::endl;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            if (search_thread.joinable()) {
                Search.stop = true;
                search_thread.join();
            }
            clear_search();
            pos.set_startpos(&root_state);
            state_idx = 0;
        } else if (token == "setoption") {
            std::string name, value, t;
            is >> t; // name
            std::string option_name;
            while (is >> t && t != "value") {
                if (!option_name.empty()) option_name += " ";
                option_name += t;
            }
            std::string option_value;
            while (is >> t) {
                if (!option_value.empty()) option_value += " ";
                option_value += t;
            }
            if (option_name == "Hash") {
                hash_mb = std::stoi(option_value);
                TT.resize(hash_mb);
            } else if (option_name == "Threads") {
                threads = std::max(1, std::stoi(option_value));
            } else if (option_name == "Move Overhead") {
                move_overhead = std::stoi(option_value);
            } else if (option_name == "Clear Hash") {
                TT.clear();
            }
        } else if (token == "position") {
            if (search_thread.joinable()) {
                Search.stop = true;
                search_thread.join();
            }
            set_position(is);
        } else if (token == "go") {
            if (search_thread.joinable()) {
                Search.stop = true;
                search_thread.join();
            }
            Limits lim;
            while (is >> token) {
                if (token == "depth") is >> lim.depth;
                else if (token == "movetime") is >> lim.movetime;
                else if (token == "wtime") is >> lim.wtime;
                else if (token == "btime") is >> lim.btime;
                else if (token == "winc") is >> lim.winc;
                else if (token == "binc") is >> lim.binc;
                else if (token == "movestogo") is >> lim.movestogo;
                else if (token == "nodes") is >> lim.nodes;
                else if (token == "infinite") lim.infinite = true;
                else if (token == "perft") {
                    lim.perft = true;
                    is >> lim.perft_depth;
                }
            }
            if (lim.wtime > move_overhead) lim.wtime -= move_overhead;
            if (lim.btime > move_overhead) lim.btime -= move_overhead;
            // 0.4.0: search runs on its own thread so "stop" (and "go infinite") work from a GUI
            search_thread = std::thread([lim]() { start_search(pos, lim); });
        } else if (token == "stop") {
            Search.stop = true;
            if (search_thread.joinable()) search_thread.join();
        } else if (token == "quit") {
            Search.stop = true;
            if (search_thread.joinable()) search_thread.join();
            break;
        } else if (token == "d") {
            std::cout << pos.fen() << std::endl;
        } else if (token == "perft") {
            int d = 1;
            is >> d;
            auto n = perft(pos, d);
            std::cout << "info string perft " << d << " nodes " << n << std::endl;
        }
    }
    // Input closed without "quit": let a running search finish and print bestmove
    if (search_thread.joinable()) search_thread.join();
}

} // namespace lugh
