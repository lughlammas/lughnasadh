#include "position.hpp"
#include <sstream>
#include <cstring>
#include <random>
#include <cmath>

namespace lugh {

Key ZobristPsq[PIECE_NB][SQUARE_NB];
Key ZobristEp[8];
Key ZobristCastling[16];
Key ZobristSide;

void init_zobrist() {
    std::mt19937_64 rng(1070372ULL);
    for (int pc = 0; pc < PIECE_NB; ++pc)
        for (int s = 0; s < SQUARE_NB; ++s)
            ZobristPsq[pc][s] = rng();
    for (int f = 0; f < 8; ++f) ZobristEp[f] = rng();
    for (int i = 0; i < 16; ++i) ZobristCastling[i] = rng();
    ZobristSide = rng();
}

void Position::put_piece(Piece pc, Square s) {
    board[s] = pc;
    by_type[0] |= square_bb(s);
    by_type[type_of(pc)] |= square_bb(s);
    by_color[color_of(pc)] |= square_bb(s);
    piece_count[pc]++;
    if (type_of(pc) == KING) king_sq[color_of(pc)] = s;
}

void Position::remove_piece(Square s) {
    Piece pc = board[s];
    by_type[0] ^= square_bb(s);
    by_type[type_of(pc)] ^= square_bb(s);
    by_color[color_of(pc)] ^= square_bb(s);
    board[s] = NO_PIECE;
    piece_count[pc]--;
}

void Position::move_piece(Square from, Square to) {
    Piece pc = board[from];
    Bitboard fromTo = square_bb(from) | square_bb(to);
    by_type[0] ^= fromTo;
    by_type[type_of(pc)] ^= fromTo;
    by_color[color_of(pc)] ^= fromTo;
    board[from] = NO_PIECE;
    board[to] = pc;
    if (type_of(pc) == KING) king_sq[color_of(pc)] = to;
}

void Position::set_castling_right(Color c, Square rook_from) {
    Square ksq = king_sq[c];
    bool king_side = file_of(rook_from) > file_of(ksq);
    CastlingRights cr = c == WHITE ? (king_side ? WHITE_OO : WHITE_OOO)
                                   : (king_side ? BLACK_OO : BLACK_OOO);
    st->castling |= cr;
    castling_rights_mask[ksq] |= cr;
    castling_rights_mask[rook_from] |= cr;
    castling_rook_square[cr] = rook_from;

    Square kto = relative_square(c, king_side ? SQ_G1 : SQ_C1);
    Square rto = relative_square(c, king_side ? SQ_F1 : SQ_D1);
    Bitboard path = 0;
    for (int s = std::min(int(ksq), int(kto)); s <= std::max(int(ksq), int(kto)); ++s)
        path |= square_bb(Square(s));
    for (int s = std::min(int(rook_from), int(rto)); s <= std::max(int(rook_from), int(rto)); ++s)
        path |= square_bb(Square(s));
    path &= ~(square_bb(ksq) | square_bb(rook_from));
    castling_path[cr] = path;
}

void Position::set_startpos(StateInfo* si) {
    set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", si);
}

void Position::set(const std::string& fenStr, StateInfo* si) {
    std::memset(this, 0, sizeof(Position));
    st = si;
    std::memset(st, 0, sizeof(StateInfo));
    for (int i = 0; i < SQUARE_NB; ++i) castling_rights_mask[i] = 0;

    std::istringstream ss(fenStr);
    std::string board_part, stm, castle, ep;
    int rule50 = 0, fullmove = 1;
    ss >> board_part >> stm >> castle >> ep >> rule50 >> fullmove;

    Square sq = SQ_A8;
    for (char ch : board_part) {
        if (ch == '/') sq = Square(int(sq) - 16);
        else if (ch >= '1' && ch <= '8') sq = Square(int(sq) + (ch - '0'));
        else {
            Color c = (ch >= 'a') ? BLACK : WHITE;
            PieceType pt;
            switch (ch | 32) {
            case 'p': pt = PAWN; break;
            case 'n': pt = KNIGHT; break;
            case 'b': pt = BISHOP; break;
            case 'r': pt = ROOK; break;
            case 'q': pt = QUEEN; break;
            case 'k': pt = KING; break;
            default: continue;
            }
            put_piece(make_piece(c, pt), sq);
            sq = Square(int(sq) + 1);
        }
    }

    side = (stm == "w") ? WHITE : BLACK;
    st->castling = NO_CASTLING;
    for (char ch : castle) {
        if (ch == 'K') set_castling_right(WHITE, SQ_H1);
        else if (ch == 'Q') set_castling_right(WHITE, SQ_A1);
        else if (ch == 'k') set_castling_right(BLACK, SQ_H8);
        else if (ch == 'q') set_castling_right(BLACK, SQ_A8);
        else if (ch >= 'A' && ch <= 'H') {
            // FRC not supported fully; ignore
        }
    }

    st->ep_square = SQ_NONE;
    if (ep != "-" && ep.size() == 2) {
        Square eps = make_square(File(ep[0] - 'a'), Rank(ep[1] - '1'));
        // Only set if a pawn can capture
        if (pawn_attacks_bb(~side, eps) & pieces(side, PAWN))
            st->ep_square = eps;
    }

    st->rule50 = rule50;
    gamePly = std::max(2 * (fullmove - 1), 0) + (side == BLACK);
    set_state();
}

void Position::set_state() {
    st->key = compute_key();
    st->checkers = attackers_to(king_sq[side]) & pieces(~side);
    update_slider_blockers<WHITE>(st);
    update_slider_blockers<BLACK>(st);
}

Key Position::compute_key() const {
    Key k = 0;
    Bitboard b = pieces();
    while (b) {
        Square s = pop_lsb(b);
        k ^= ZobristPsq[board[s]][s];
    }
    if (st->ep_square != SQ_NONE)
        k ^= ZobristEp[file_of(st->ep_square)];
    k ^= ZobristCastling[st->castling];
    if (side == BLACK) k ^= ZobristSide;
    return k;
}

Bitboard Position::attackers_to(Square s, Bitboard occ) const {
    return (pawn_attacks_bb(BLACK, s) & pieces(WHITE, PAWN))
         | (pawn_attacks_bb(WHITE, s) & pieces(BLACK, PAWN))
         | (PseudoAttacks[KNIGHT][s] & pieces(KNIGHT))
         | (attacks_bb(BISHOP, s, occ) & pieces(BISHOP, QUEEN))
         | (attacks_bb(ROOK, s, occ) & pieces(ROOK, QUEEN))
         | (PseudoAttacks[KING][s] & pieces(KING));
}

// Need pieces(PieceType, PieceType) overload - add via by_type
// Fix: pieces(BISHOP, QUEEN) isn't defined. Use:
// (by_type[BISHOP]|by_type[QUEEN])

template<Color Us>
void Position::update_slider_blockers(StateInfo* state) const {
    const Color Them = ~Us;
    Square ksq = king_sq[Us];
    state->blockers[Us] = 0;
    state->pinners[Them] = 0;

    Bitboard snipers = ((PseudoAttacks[ROOK][ksq] & pieces(Them, ROOK, QUEEN))
                      | (PseudoAttacks[BISHOP][ksq] & pieces(Them, BISHOP, QUEEN)));
    Bitboard occupancy = pieces() ^ snipers;

    while (snipers) {
        Square sniper = pop_lsb(snipers);
        Bitboard b = BetweenBB[ksq][sniper] & occupancy;
        if (b && !more_than_one(b)) {
            state->blockers[Us] |= b;
            if (b & pieces(Us))
                state->pinners[Them] |= square_bb(sniper);
        }
    }
}

template void Position::update_slider_blockers<WHITE>(StateInfo*) const;
template void Position::update_slider_blockers<BLACK>(StateInfo*) const;

bool Position::legal(Move m) const {
    Color us = side;
    Square from = m.from();
    Square to = m.to();
    Square ksq = king_sq[us];

    if (m.type() == EN_PASSANT) {
        Square capsq = to + ((us == WHITE) ? -8 : 8);
        Bitboard occupied = (pieces() ^ square_bb(from) ^ square_bb(capsq)) | square_bb(to);
        return !(attacks_bb(ROOK, ksq, occupied) & pieces(~us, ROOK, QUEEN))
            && !(attacks_bb(BISHOP, ksq, occupied) & pieces(~us, BISHOP, QUEEN));
    }

    if (m.type() == CASTLING) {
        // King path must not be attacked (handled in movegen mostly)
        to = relative_square(us, to > from ? SQ_G1 : SQ_C1);
        int step = to > from ? 1 : -1;
        // Check squares king passes through
        for (Square s = from; s != to; s = Square(int(s) + step))
            if (attackers_to(s) & pieces(~us))
                return false;
        return !(attackers_to(to) & pieces(~us));
    }

    if (type_of(board[from]) == KING)
        return !(attackers_to(to, pieces() ^ square_bb(from)) & pieces(~us));

    // Moving a pinned piece off the pin ray is illegal
    return !(st->blockers[us] & square_bb(from))
        || (LineBB[from][to] & square_bb(ksq));
}

bool Position::pseudo_legal(Move m) const {
    Color us = side;
    Square from = m.from(), to = m.to();
    Piece pc = board[from];
    if (pc == NO_PIECE || color_of(pc) != us) return false;
    if (board[to] != NO_PIECE && color_of(board[to]) == us) return false;

    // Rough check — used for TT moves
    PieceType pt = type_of(pc);
    if (pt == PAWN) {
        int dir = (us == WHITE) ? 8 : -8;
        if (m.type() == EN_PASSANT)
            return to == st->ep_square && (pawn_attacks_bb(us, from) & square_bb(to));
        if (m.type() == PROMOTION) {
            if (relative_rank(us, to) != RANK_8) return false;
        } else if (relative_rank(us, to) == RANK_8) return false;

        if (file_of(from) == file_of(to)) {
            if (board[to] != NO_PIECE) return false;
            if (int(to) == int(from) + dir) return true;
            if (int(to) == int(from) + 2 * dir
                && relative_rank(us, from) == RANK_2
                && board[Square(int(from) + dir)] == NO_PIECE)
                return true;
            return false;
        }
        return (pawn_attacks_bb(us, from) & square_bb(to) & pieces(~us)) != 0;
    }
    if (m.type() == CASTLING) {
        CastlingRights cr = (to > from)
            ? (us == WHITE ? WHITE_OO : BLACK_OO)
            : (us == WHITE ? WHITE_OOO : BLACK_OOO);
        return (st->castling & cr)
            && !(pieces() & castling_path[cr])
            && !in_check();
    }
    return (attacks_bb(pt, from, pieces()) & square_bb(to)) != 0;
}

bool Position::gives_check(Move m) const {
    // Approximate: make not needed for search often; optional
    Square from = m.from(), to = m.to();
    PieceType pt = type_of(board[from]);
    Color us = side;
    Square ksq = king_sq[~us];

    if (PseudoAttacks[pt == PAWN ? KNIGHT : pt][to] & square_bb(ksq)) {
        // Direct check for non-sliding handled below properly
    }
    if (pt == PAWN) {
        if (pawn_attacks_bb(us, to) & square_bb(ksq)) return true;
    } else if (pt == KNIGHT || pt == KING) {
        if (PseudoAttacks[pt][to] & square_bb(ksq)) return true;
    } else {
        if (attacks_bb(pt, to, pieces() ^ square_bb(from)) & square_bb(ksq))
            return true;
    }
    // Discovered check
    Bitboard occ = pieces() ^ square_bb(from) ^ (m.type() == CASTLING ? 0 : square_bb(to));
    if (m.type() == EN_PASSANT)
        occ ^= square_bb(to + ((us == WHITE) ? -8 : 8));
    if ((st->blockers[~us] & square_bb(from))
        && !(LineBB[from][ksq] & square_bb(to)))
        return true;
    if (m.type() == CASTLING) {
        Square rto = relative_square(us, to > from ? SQ_F1 : SQ_D1);
        return attacks_bb(ROOK, rto, occ) & square_bb(ksq);
    }
    if (m.type() == PROMOTION)
        return attacks_bb(m.promotion_type(), to, occ) & square_bb(ksq);
    return false;
}

void Position::do_move(Move m, StateInfo& new_st) {
    Key k = st->key ^ ZobristSide;
    std::memcpy(&new_st, st, offsetof(StateInfo, previous));
    new_st.previous = st;
    st = &new_st;

    Color us = side;
    Color them = ~us;
    Square from = m.from();
    Square to = m.to();
    Piece pc = board[from];
    Piece captured = (m.type() == EN_PASSANT) ? make_piece(them, PAWN) : board[to];

    st->rule50++;
    st->plies_from_null++;
    gamePly++;

    if (m.type() == CASTLING) {
        bool king_side = to > from;
        Square kto = relative_square(us, king_side ? SQ_G1 : SQ_C1);
        Square rto = relative_square(us, king_side ? SQ_F1 : SQ_D1);
        Square rfrom = castling_rook_square[king_side
            ? (us == WHITE ? WHITE_OO : BLACK_OO)
            : (us == WHITE ? WHITE_OOO : BLACK_OOO)];
        Piece rook = board[rfrom];
        k ^= ZobristPsq[pc][from] ^ ZobristPsq[pc][kto];
        k ^= ZobristPsq[rook][rfrom] ^ ZobristPsq[rook][rto];
        remove_piece(from);
        remove_piece(rfrom);
        board[kto] = board[rto] = NO_PIECE;
        put_piece(pc, kto);
        put_piece(rook, rto);
        captured = NO_PIECE;
    } else {
        if (captured != NO_PIECE) {
            Square capsq = to;
            if (m.type() == EN_PASSANT)
                capsq = Square(int(to) + (us == WHITE ? -8 : 8));
            k ^= ZobristPsq[captured][capsq];
            remove_piece(capsq);
            st->rule50 = 0;
        }
        k ^= ZobristPsq[pc][from] ^ ZobristPsq[pc][to];
        move_piece(from, to);

        if (m.type() == PROMOTION) {
            Piece promotion = make_piece(us, m.promotion_type());
            remove_piece(to);
            put_piece(promotion, to);
            k ^= ZobristPsq[pc][to] ^ ZobristPsq[promotion][to];
        }
    }

    if (st->ep_square != SQ_NONE) {
        k ^= ZobristEp[file_of(st->ep_square)];
        st->ep_square = SQ_NONE;
    }

    if (type_of(pc) == PAWN && m.type() != CASTLING) {
        st->rule50 = 0;
        if (std::abs(int(to) - int(from)) == 16) {
            Square eps = Square((int(from) + int(to)) / 2);
            if (pawn_attacks_bb(us, eps) & pieces(them, PAWN)) {
                st->ep_square = eps;
                k ^= ZobristEp[file_of(eps)];
            }
        }
    }

    st->captured = captured;

    k ^= ZobristCastling[st->castling];
    st->castling &= ~(castling_rights_mask[from] | castling_rights_mask[to]);
    // Also clear if rook was captured on its original square — handled by to mask
    k ^= ZobristCastling[st->castling];

    st->key = k;
    side = them;
    st->checkers = attackers_to(king_sq[side]) & pieces(~side);
    update_slider_blockers<WHITE>(st);
    update_slider_blockers<BLACK>(st);
}

void Position::undo_move(Move m) {
    side = ~side;
    Color us = side;
    Square from = m.from();
    Square to = m.to();

    if (m.type() == PROMOTION) {
        remove_piece(to);
        put_piece(make_piece(us, PAWN), to);
    }

    if (m.type() == CASTLING) {
        bool king_side = to > from;
        Square kto = relative_square(us, king_side ? SQ_G1 : SQ_C1);
        Square rto = relative_square(us, king_side ? SQ_F1 : SQ_D1);
        Square rfrom = castling_rook_square[king_side
            ? (us == WHITE ? WHITE_OO : BLACK_OO)
            : (us == WHITE ? WHITE_OOO : BLACK_OOO)];
        remove_piece(kto);
        remove_piece(rto);
        board[from] = board[rfrom] = NO_PIECE;
        put_piece(make_piece(us, KING), from);
        put_piece(make_piece(us, ROOK), rfrom);
    } else {
        move_piece(to, from);
        if (st->captured != NO_PIECE) {
            Square capsq = to;
            if (m.type() == EN_PASSANT)
                capsq = Square(int(to) + (us == WHITE ? -8 : 8));
            put_piece(st->captured, capsq);
        }
    }

    st = st->previous;
    gamePly--;
}

void Position::do_null_move(StateInfo& new_st) {
    std::memcpy(&new_st, st, offsetof(StateInfo, previous));
    new_st.previous = st;
    st = &new_st;
    if (st->ep_square != SQ_NONE) {
        st->key ^= ZobristEp[file_of(st->ep_square)];
        st->ep_square = SQ_NONE;
    }
    st->key ^= ZobristSide;
    st->rule50++;
    st->plies_from_null = 0;
    side = ~side;
    st->checkers = 0;
    update_slider_blockers<WHITE>(st);
    update_slider_blockers<BLACK>(st);
}

void Position::undo_null_move() {
    st = st->previous;
    side = ~side;
}

bool Position::is_draw(int ply) const {
    if (st->rule50 > 99 && (!in_check() /* or has legal move - simplify */))
        return st->rule50 >= 100;
    // Repetition
    int end = std::min(st->rule50, st->plies_from_null);
    if (end < 4) return false;
    StateInfo* stp = st->previous->previous;
    for (int i = 4; i <= end; i += 2) {
        stp = stp->previous->previous;
        if (stp->key == st->key)
            return true; // 2-fold for search
    }
    (void)ply;
    return false;
}

bool Position::has_repeated() const {
    return false;
}

// SEE (Static Exchange Evaluation)
Value Position::see_ge(Move m, Value threshold) const {
    // Simplified SEE >= threshold
    static const Value SeeValues[PIECE_TYPE_NB] = {
        0, 100, 300, 300, 500, 900, 0
    };
    Square from = m.from(), to = m.to();
    PieceType nextVictim = type_of(board[from]);
    Color us = side;
    Bitboard occupied = pieces() ^ square_bb(from);
    Value balance = -threshold;

    if (m.type() == EN_PASSANT) {
        occupied ^= square_bb(to + ((us == WHITE) ? -8 : 8));
        balance += SeeValues[PAWN];
    } else {
        if (board[to] != NO_PIECE)
            balance += SeeValues[type_of(board[to])];
    }
    if (m.type() == PROMOTION) {
        balance += SeeValues[m.promotion_type()] - SeeValues[PAWN];
        nextVictim = m.promotion_type();
    }
    if (balance < 0) return false;
    balance -= SeeValues[nextVictim];
    if (balance >= 0) return true;

    occupied |= square_bb(to); // wait, from already removed
    // Actually occupied should not include 'to' destination piece which is captured
    occupied &= ~square_bb(to);
    Bitboard attackers = attackers_to(to, occupied) & occupied;
    Color stm = ~us;

    while (true) {
        Bitboard myAttackers = attackers & pieces(stm);
        if (!myAttackers) break;

        // Least valuable attacker
        PieceType pt;
        for (pt = PAWN; pt <= KING; ++pt)
            if (myAttackers & pieces(pt)) break;

        occupied ^= square_bb(lsb(myAttackers & pieces(pt)));
        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= attacks_bb(BISHOP, to, occupied) & pieces(BISHOP, QUEEN);
        if (pt == ROOK || pt == QUEEN)
            attackers |= attacks_bb(ROOK, to, occupied) & pieces(ROOK, QUEEN);
        attackers &= occupied;

        stm = ~stm;
        balance = -balance - 1 - SeeValues[pt];
        if (balance >= 0) {
            if (pt == KING && (attackers & pieces(stm)))
                stm = ~stm;
            break;
        }
    }
    return stm != us;
}

int Position::see_signed(Move m) const {
    // Binary search style approximate — for ordering use see_ge
    if (!is_capture(m) && m.type() != PROMOTION) return 0;
    static const int V[8] = {0,100,300,300,500,900,20000};
    int score = V[type_of(board[m.to()])];
    if (m.type() == EN_PASSANT) score = 100;
    if (m.type() == PROMOTION) score += V[m.promotion_type()] - 100;
    score -= V[type_of(board[m.from()])] / 16;
    return score;
}

std::string Position::fen() const {
    std::ostringstream ss;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece pc = board[make_square(File(f), Rank(r))];
            if (pc == NO_PIECE) empty++;
            else {
                if (empty) { ss << empty; empty = 0; }
                static const char* chars = " PNBRQK  pnbrqk";
                ss << chars[pc];
            }
        }
        if (empty) ss << empty;
        if (r) ss << '/';
    }
    ss << (side == WHITE ? " w " : " b ");
    if (st->castling & WHITE_OO) ss << 'K';
    if (st->castling & WHITE_OOO) ss << 'Q';
    if (st->castling & BLACK_OO) ss << 'k';
    if (st->castling & BLACK_OOO) ss << 'q';
    if (!(st->castling & ANY_CASTLING)) ss << '-';
    ss << ' ';
    if (st->ep_square == SQ_NONE) ss << '-';
    else ss << square_str(st->ep_square);
    ss << ' ' << st->rule50 << ' ' << (1 + gamePly / 2);
    return ss.str();
}

} // namespace lugh
