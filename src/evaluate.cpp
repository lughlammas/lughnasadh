#include "evaluate.hpp"
#include <algorithm>

namespace lugh {

namespace {

constexpr Value PieceValue[PIECE_TYPE_NB] = {
    0, 100, 320, 330, 500, 900, 0
};

// Tapered PST: mg/eg packed as Score = mg + (eg << 16) style via separate tables
struct Tapered {
    int mg, eg;
    Tapered operator+(Tapered o) const { return {mg + o.mg, eg + o.eg}; }
    Tapered operator-(Tapered o) const { return {mg - o.mg, eg - o.eg}; }
    Tapered& operator+=(Tapered o) { mg += o.mg; eg += o.eg; return *this; }
    Tapered operator-() const { return {-mg, -eg}; }
};

constexpr Tapered S(int mg, int eg) { return {mg, eg}; }

// Piece-square tables from White's perspective (a1 = index 0)
const Tapered Pst[PIECE_TYPE_NB][SQUARE_NB] = {
{}, // none
{ // Pawn
S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),
S(50,80),S(50,80),S(50,80),S(50,80),S(50,80),S(50,80),S(50,80),S(50,80),
S(10,20),S(10,20),S(20,20),S(30,30),S(30,30),S(20,20),S(10,20),S(10,20),
S(5,10),S(5,10),S(10,15),S(25,25),S(25,25),S(10,15),S(5,10),S(5,10),
S(0,5),S(0,5),S(0,10),S(20,20),S(20,20),S(0,10),S(0,5),S(0,5),
S(5,5),S(-5,5),S(-10,0),S(0,10),S(0,10),S(-10,0),S(-5,5),S(5,5),
S(5,10),S(10,10),S(10,-5),S(-20,0),S(-20,0),S(10,-5),S(10,10),S(5,10),
S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0)
},
{ // Knight
S(-50,-50),S(-40,-40),S(-30,-30),S(-30,-30),S(-30,-30),S(-30,-30),S(-40,-40),S(-50,-50),
S(-40,-40),S(-20,-20),S(0,0),S(0,0),S(0,0),S(0,0),S(-20,-20),S(-40,-40),
S(-30,-30),S(0,0),S(10,10),S(15,15),S(15,15),S(10,10),S(0,0),S(-30,-30),
S(-30,-30),S(5,5),S(15,15),S(20,20),S(20,20),S(15,15),S(5,5),S(-30,-30),
S(-30,-30),S(0,0),S(15,15),S(20,20),S(20,20),S(15,15),S(0,0),S(-30,-30),
S(-30,-30),S(5,5),S(10,10),S(15,15),S(15,15),S(10,10),S(5,5),S(-30,-30),
S(-40,-40),S(-20,-20),S(0,0),S(5,5),S(5,5),S(0,0),S(-20,-20),S(-40,-40),
S(-50,-50),S(-40,-40),S(-30,-30),S(-30,-30),S(-30,-30),S(-30,-30),S(-40,-40),S(-50,-50)
},
{ // Bishop
S(-20,-20),S(-10,-10),S(-10,-10),S(-10,-10),S(-10,-10),S(-10,-10),S(-10,-10),S(-20,-20),
S(-10,-10),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-10,-10),
S(-10,-10),S(0,0),S(5,5),S(10,10),S(10,10),S(5,5),S(0,0),S(-10,-10),
S(-10,-10),S(5,5),S(5,5),S(10,10),S(10,10),S(5,5),S(5,5),S(-10,-10),
S(-10,-10),S(0,0),S(10,10),S(10,10),S(10,10),S(10,10),S(0,0),S(-10,-10),
S(-10,-10),S(10,10),S(10,10),S(10,10),S(10,10),S(10,10),S(10,10),S(-10,-10),
S(-10,-10),S(5,5),S(0,0),S(0,0),S(0,0),S(0,0),S(5,5),S(-10,-10),
S(-20,-20),S(-10,-10),S(-10,-10),S(-10,-10),S(-10,-10),S(-10,-10),S(-10,-10),S(-20,-20)
},
{ // Rook
S(0,0),S(0,0),S(0,0),S(0,5),S(0,5),S(0,0),S(0,0),S(0,0),
S(5,5),S(10,10),S(10,10),S(10,10),S(10,10),S(10,10),S(10,10),S(5,5),
S(-5,-5),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-5,-5),
S(-5,-5),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-5,-5),
S(-5,-5),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-5,-5),
S(-5,-5),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-5,-5),
S(-5,-5),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-5,-5),
S(0,0),S(0,0),S(0,0),S(5,0),S(5,0),S(0,0),S(0,0),S(0,0)
},
{ // Queen
S(-20,-20),S(-10,-10),S(-10,-10),S(-5,-5),S(-5,-5),S(-10,-10),S(-10,-10),S(-20,-20),
S(-10,-10),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-10,-10),
S(-10,-10),S(0,0),S(5,5),S(5,5),S(5,5),S(5,5),S(0,0),S(-10,-10),
S(-5,-5),S(0,0),S(5,5),S(5,5),S(5,5),S(5,5),S(0,0),S(-5,-5),
S(0,0),S(0,0),S(5,5),S(5,5),S(5,5),S(5,5),S(0,0),S(-5,-5),
S(-10,-10),S(5,5),S(5,5),S(5,5),S(5,5),S(5,5),S(0,0),S(-10,-10),
S(-10,-10),S(0,0),S(5,0),S(0,0),S(0,0),S(0,0),S(0,0),S(-10,-10),
S(-20,-20),S(-10,-10),S(-10,-10),S(-5,-5),S(-5,-5),S(-10,-10),S(-10,-10),S(-20,-20)
},
{ // King
S(-30,-50),S(-40,-40),S(-40,-30),S(-50,-20),S(-50,-20),S(-40,-30),S(-40,-40),S(-30,-50),
S(-30,-30),S(-40,-30),S(-40,-20),S(-50,-10),S(-50,-10),S(-40,-20),S(-40,-30),S(-30,-30),
S(-30,-30),S(-40,-20),S(-40,-10),S(-50,0),S(-50,0),S(-40,-10),S(-40,-20),S(-30,-30),
S(-30,-30),S(-40,-20),S(-40,-10),S(-50,0),S(-50,0),S(-40,-10),S(-40,-20),S(-30,-30),
S(-20,-20),S(-30,-10),S(-30,0),S(-40,10),S(-40,10),S(-30,0),S(-30,-10),S(-20,-20),
S(-10,-20),S(-20,-10),S(-20,0),S(-20,10),S(-20,10),S(-20,0),S(-20,-10),S(-10,-20),
S(20,-30),S(20,-20),S(0,-10),S(0,0),S(0,0),S(0,-10),S(20,-20),S(20,-30),
S(20,-50),S(30,-40),S(10,-30),S(0,-20),S(0,-20),S(10,-30),S(30,-40),S(20,-50)
}
};

int phase_weight(const Position& pos) {
    int p = 0;
    p += popcount(pos.pieces(KNIGHT)) * 1;
    p += popcount(pos.pieces(BISHOP)) * 1;
    p += popcount(pos.pieces(ROOK)) * 2;
    p += popcount(pos.pieces(QUEEN)) * 4;
    return std::min(p, 24);
}

Tapered evaluate_side(const Position& pos, Color us) {
    Tapered score = {0, 0};
    Color them = ~us;
    Bitboard occ = pos.pieces();

    // Material + PST
    for (PieceType pt = PAWN; pt <= KING; ++pt) {
        Bitboard b = pos.pieces(us, pt);
        while (b) {
            Square s = pop_lsb(b);
            Square ps = relative_square(us, s);
            score.mg += PieceValue[pt] + Pst[pt][ps].mg;
            score.eg += PieceValue[pt] + Pst[pt][ps].eg;
        }
    }

    // Mobility
    auto add_mob = [&](PieceType pt, int mg_unit, int eg_unit) {
        Bitboard b = pos.pieces(us, pt);
        while (b) {
            Square s = pop_lsb(b);
            Bitboard att = (pt == KNIGHT || pt == KING)
                ? PseudoAttacks[pt][s]
                : attacks_bb(pt, s, occ);
            att &= ~pos.pieces(us);
            int mob = popcount(att);
            score.mg += mob * mg_unit;
            score.eg += mob * eg_unit;
        }
    };
    add_mob(KNIGHT, 4, 3);
    add_mob(BISHOP, 5, 4);
    add_mob(ROOK, 3, 4);
    add_mob(QUEEN, 1, 2);

    // Bishop pair — classical C++ lesson: two bishops often outweigh two knights in open positions
    if (popcount(pos.pieces(us, BISHOP)) >= 2) {
        score.mg += 30;
        score.eg += 50;
    }

    // King safety: count enemy attacks near our king
    Square ksq = pos.king_square(us);
    Bitboard kingZone = PseudoAttacks[KING][ksq] | square_bb(ksq);
    int attackUnits = 0;
    Bitboard knights = pos.pieces(them, KNIGHT);
    while (knights) {
        Square s = pop_lsb(knights);
        attackUnits += popcount(PseudoAttacks[KNIGHT][s] & kingZone) * 2;
    }
    Bitboard bishops = pos.pieces(them, BISHOP);
    while (bishops) {
        Square s = pop_lsb(bishops);
        attackUnits += popcount(attacks_bb(BISHOP, s, occ) & kingZone) * 2;
    }
    Bitboard rooks = pos.pieces(them, ROOK);
    while (rooks) {
        Square s = pop_lsb(rooks);
        attackUnits += popcount(attacks_bb(ROOK, s, occ) & kingZone) * 3;
    }
    Bitboard queens = pos.pieces(them, QUEEN);
    while (queens) {
        Square s = pop_lsb(queens);
        attackUnits += popcount(attacks_bb(QUEEN, s, occ) & kingZone) * 5;
    }
    score.mg -= attackUnits * attackUnits / 4;

    // Passed pawns
    Bitboard ourPawns = pos.pieces(us, PAWN);
    Bitboard theirPawns = pos.pieces(them, PAWN);
    Bitboard b = ourPawns;
    while (b) {
        Square s = pop_lsb(b);
        File f = file_of(s);
        Bitboard span = 0;
        // Forward files: same + adjacent
        Bitboard files = FileABB << f;
        if (f > 0) files |= FileABB << (f - 1);
        if (f < 7) files |= FileABB << (f + 1);
        if (us == WHITE) {
            for (int r = rank_of(s) + 1; r <= 7; ++r)
                span |= files & (Rank1BB << (8 * r));
        } else {
            for (int r = 0; r < rank_of(s); ++r)
                span |= files & (Rank1BB << (8 * r));
        }
        if (!(span & theirPawns)) {
            int rr = relative_rank(us, s);
            static const int PassedBonusMg[8] = {0,5,10,20,35,60,100,0};
            static const int PassedBonusEg[8] = {0,10,20,40,70,120,200,0};
            score.mg += PassedBonusMg[rr];
            score.eg += PassedBonusEg[rr];
        }
    }

    return score;
}

} // namespace

Value evaluate(const Position& pos) {
    Tapered w = evaluate_side(pos, WHITE);
    Tapered b = evaluate_side(pos, BLACK);
    Tapered score = w - b;
    int phase = phase_weight(pos);
    int v = (score.mg * phase + score.eg * (24 - phase)) / 24;
    // Tempo
    v += (pos.side_to_move() == WHITE) ? 10 : -10;
    return Value(pos.side_to_move() == WHITE ? v : -v);
}

} // namespace lugh
