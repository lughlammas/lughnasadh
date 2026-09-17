#include "movegen.hpp"

namespace lugh {

namespace {

template<Color Us, bool Checks>
void generate_pawn_moves(const Position& pos, MoveList& list, Bitboard target) {
    constexpr Color Them = ~Us;
    constexpr Bitboard TRank7BB = (Us == WHITE) ? 0x00FF000000000000ULL : 0x000000000000FF00ULL;
    constexpr Bitboard TRank3BB = (Us == WHITE) ? 0x0000000000FF0000ULL : 0x0000FF0000000000ULL;
    constexpr int Up = (Us == WHITE) ? 8 : -8;

    Bitboard pawns = pos.pieces(Us, PAWN);
    Bitboard empty = ~pos.pieces();

    Bitboard pawnsNon7 = pawns & ~TRank7BB;
    Bitboard push1 = (Us == WHITE ? (pawnsNon7 << 8) : (pawnsNon7 >> 8)) & empty;
    Bitboard push2 = (Us == WHITE ? ((push1 & TRank3BB) << 8) : ((push1 & TRank3BB) >> 8)) & empty;

    push1 &= target;
    push2 &= target;

    while (push1) {
        Square to = pop_lsb(push1);
        list.push(Move::make(Square(int(to) - Up), to));
    }
    while (push2) {
        Square to = pop_lsb(push2);
        list.push(Move::make(Square(int(to) - 2 * Up), to));
    }

    // Captures (non-promo): toward file A and file H
    Bitboard pawns7 = pawns & TRank7BB;
    Bitboard capA, capH;
    if constexpr (Us == WHITE) {
        capA = ((pawnsNon7 & ~FileABB) << 7) & pos.pieces(Them) & target;
        capH = ((pawnsNon7 & ~FileHBB) << 9) & pos.pieces(Them) & target;
    } else {
        capA = ((pawnsNon7 & ~FileABB) >> 9) & pos.pieces(Them) & target;
        capH = ((pawnsNon7 & ~FileHBB) >> 7) & pos.pieces(Them) & target;
    }
    while (capA) {
        Square to = pop_lsb(capA);
        Square from = Square(Us == WHITE ? int(to) - 7 : int(to) + 9);
        list.push(Move::make(from, to));
    }
    while (capH) {
        Square to = pop_lsb(capH);
        Square from = Square(Us == WHITE ? int(to) - 9 : int(to) + 7);
        list.push(Move::make(from, to));
    }

    // Promotions
    if (pawns7) {
        Bitboard promoPush = (Us == WHITE ? (pawns7 << 8) : (pawns7 >> 8)) & empty & target;
        Bitboard promoA, promoH;
        if constexpr (Us == WHITE) {
            promoA = ((pawns7 & ~FileABB) << 7) & pos.pieces(Them) & target;
            promoH = ((pawns7 & ~FileHBB) << 9) & pos.pieces(Them) & target;
        } else {
            promoA = ((pawns7 & ~FileABB) >> 9) & pos.pieces(Them) & target;
            promoH = ((pawns7 & ~FileHBB) >> 7) & pos.pieces(Them) & target;
        }
        auto add_promos = [&](Square from, Square to) {
            for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                list.push(Move::make(from, to, PROMOTION, pt));
        };
        while (promoPush) {
            Square to = pop_lsb(promoPush);
            add_promos(Square(int(to) - Up), to);
        }
        while (promoA) {
            Square to = pop_lsb(promoA);
            Square from = Square(Us == WHITE ? int(to) - 7 : int(to) + 9);
            add_promos(from, to);
        }
        while (promoH) {
            Square to = pop_lsb(promoH);
            Square from = Square(Us == WHITE ? int(to) - 9 : int(to) + 7);
            add_promos(from, to);
        }
    }

    // En passant
    if (pos.ep_square() != SQ_NONE) {
        Square ep = pos.ep_square();
        Bitboard b = pawns & pawn_attacks_bb(Them, ep);
        while (b) {
            Square from = pop_lsb(b);
            list.push(Move::make(from, ep, EN_PASSANT));
        }
    }
    (void)Checks;
}

template<Color Us, PieceType Pt>
void generate_piece_moves(const Position& pos, MoveList& list, Bitboard target) {
    Bitboard bb = pos.pieces(Us, Pt);
    while (bb) {
        Square from = pop_lsb(bb);
        Bitboard attacks = (Pt == KNIGHT || Pt == KING)
            ? PseudoAttacks[Pt][from]
            : attacks_bb(Pt, from, pos.pieces());
        attacks &= target;
        while (attacks) {
            Square to = pop_lsb(attacks);
            list.push(Move::make(from, to));
        }
    }
}

template<Color Us>
void generate_castling(const Position& pos, MoveList& list) {
    if (pos.in_check()) return;
    const int cr_king = Us == WHITE ? WHITE_OO : BLACK_OO;
    const int cr_queen = Us == WHITE ? WHITE_OOO : BLACK_OOO;
    Square ksq = pos.king_square(Us);

    auto try_castle = [&](int cr, Square kto) {
        if (!(pos.castling_rights() & cr)) return;
        if (pos.pieces() & pos.castling_path_bb(cr)) return;
        // Intermediate squares
        int step = kto > ksq ? 1 : -1;
        for (Square s = ksq; s != kto; s = Square(int(s) + step)) {
            if (pos.attackers_to(s) & pos.pieces(~Us)) return;
        }
        if (pos.attackers_to(kto) & pos.pieces(~Us)) return;
        list.push(Move::make(ksq, kto, CASTLING));
    };

    try_castle(cr_king, relative_square(Us, SQ_G1));
    try_castle(cr_queen, relative_square(Us, SQ_C1));
}

template<Color Us, GenType Type>
void generate_all(const Position& pos, MoveList& list) {
    const Bitboard usPieces = pos.pieces(Us);
    Bitboard target = (Type == CAPTURES) ? pos.pieces(~Us)
                    : (Type == QUIETS) ? ~pos.pieces()
                    : ~usPieces; // NON_EVASIONS / LEGAL

    if (Type == EVASIONS) {
        Bitboard checkers = pos.checkers();
        Square ksq = pos.king_square(Us);
        Bitboard sliderAttacks = 0;
        Bitboard b = checkers;
        while (b) {
            Square checksq = pop_lsb(b);
            PieceType pt = type_of(pos.piece_on(checksq));
            if (pt == BISHOP || pt == ROOK || pt == QUEEN)
                sliderAttacks |= LineBB[checksq][ksq] ^ square_bb(checksq);
        }
        // King escapes
        Bitboard kingMoves = PseudoAttacks[KING][ksq] & ~usPieces & ~sliderAttacks;
        while (kingMoves) {
            Square to = pop_lsb(kingMoves);
            if (!(pos.attackers_to(to, pos.pieces() ^ square_bb(ksq)) & pos.pieces(~Us)))
                list.push(Move::make(ksq, to));
        }
        if (more_than_one(checkers)) return; // double check: only king moves

        Square checksq = lsb(checkers);
        Bitboard block = BetweenBB[ksq][checksq]; // between including checker? Our BetweenBB excludes ends
        target = block | square_bb(checksq);

        generate_pawn_moves<Us, false>(pos, list, target);
        generate_piece_moves<Us, KNIGHT>(pos, list, target);
        generate_piece_moves<Us, BISHOP>(pos, list, target);
        generate_piece_moves<Us, ROOK>(pos, list, target);
        generate_piece_moves<Us, QUEEN>(pos, list, target);
        return;
    }

    generate_pawn_moves<Us, false>(pos, list, target);
    generate_piece_moves<Us, KNIGHT>(pos, list, target);
    generate_piece_moves<Us, BISHOP>(pos, list, target);
    generate_piece_moves<Us, ROOK>(pos, list, target);
    generate_piece_moves<Us, QUEEN>(pos, list, target);
    generate_piece_moves<Us, KING>(pos, list, target);

    if (Type != CAPTURES)
        generate_castling<Us>(pos, list);
}

} // namespace

template<GenType Type>
void generate(const Position& pos, MoveList& list) {
    if constexpr (Type == LEGAL) {
        MoveList raw;
        if (pos.in_check()) {
            if (pos.side_to_move() == WHITE) generate_all<WHITE, EVASIONS>(pos, raw);
            else generate_all<BLACK, EVASIONS>(pos, raw);
        } else {
            if (pos.side_to_move() == WHITE) generate_all<WHITE, NON_EVASIONS>(pos, raw);
            else generate_all<BLACK, NON_EVASIONS>(pos, raw);
        }
        for (auto& em : raw) {
            if (pos.legal(em.move))
                list.push(em.move);
        }
    } else if constexpr (Type == EVASIONS) {
        if (pos.side_to_move() == WHITE) generate_all<WHITE, EVASIONS>(pos, list);
        else generate_all<BLACK, EVASIONS>(pos, list);
    } else if constexpr (Type == CAPTURES) {
        if (pos.side_to_move() == WHITE) generate_all<WHITE, CAPTURES>(pos, list);
        else generate_all<BLACK, CAPTURES>(pos, list);
    } else if constexpr (Type == QUIETS) {
        if (pos.side_to_move() == WHITE) generate_all<WHITE, QUIETS>(pos, list);
        else generate_all<BLACK, QUIETS>(pos, list);
    } else {
        if (pos.side_to_move() == WHITE) generate_all<WHITE, NON_EVASIONS>(pos, list);
        else generate_all<BLACK, NON_EVASIONS>(pos, list);
    }
}

template void generate<CAPTURES>(const Position&, MoveList&);
template void generate<QUIETS>(const Position&, MoveList&);
template void generate<EVASIONS>(const Position&, MoveList&);
template void generate<NON_EVASIONS>(const Position&, MoveList&);
template void generate<LEGAL>(const Position&, MoveList&);

} // namespace lugh
