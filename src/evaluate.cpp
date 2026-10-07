#include "evaluate.hpp"
#include <algorithm>
#include <cstdlib>

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

inline int dist(Square a, Square b) {
    return std::max(std::abs(int(file_of(a)) - int(file_of(b))), std::abs(int(rank_of(a)) - int(rank_of(b))));
}

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

    // Pawn structure + passed pawns (0.4.0)
    Bitboard ourPawns = pos.pieces(us, PAWN);
    Bitboard theirPawns = pos.pieces(them, PAWN);
    Square theirK = pos.king_square(them);
    Square ourK = ksq;
    const bool theirPawnsOnly = !(pos.pieces(them) & ~pos.pieces(them, PAWN) & ~square_bb(theirK));
    Bitboard b = ourPawns;
    while (b) {
        Square s = pop_lsb(b);
        File f = file_of(s);
        Bitboard adj = 0;
        if (f > 0) adj |= FileABB << (f - 1);
        if (f < 7) adj |= FileABB << (f + 1);
        // Isolated and doubled pawns
        if (!(ourPawns & adj)) { score.mg -= 10; score.eg -= 15; }
        if (popcount(ourPawns & (FileABB << f)) > 1) { score.mg -= 6; score.eg -= 12; }

        Bitboard span = 0;
        Bitboard files = (FileABB << f) | adj;
        if (us == WHITE) {
            for (int r = rank_of(s) + 1; r <= 7; ++r)
                span |= files & (Rank1BB << (8 * r));
        } else {
            for (int r = 0; r < rank_of(s); ++r)
                span |= files & (Rank1BB << (8 * r));
        }
        if (span & theirPawns) continue;

        int rr = relative_rank(us, s);
        static const int PassedBonusMg[8] = {0,5,10,20,35,60,100,0};
        static const int PassedBonusEg[8] = {0,10,20,40,70,120,200,0};
        score.mg += PassedBonusMg[rr];
        score.eg += PassedBonusEg[rr];

        Square stop = us == WHITE ? s + 8 : s - 8;
        Square queenSq = make_square(f, us == WHITE ? RANK_8 : RANK_1);
        if (rr >= 3) {
            // Kings: the defender wants to be in front of the pawn, the owner beside it
            int w = rr - 2;
            score.eg += (dist(theirK, stop) * 5 - dist(ourK, stop) * 2) * w;
            // Free path to the promotion square
            if (!(pos.pieces() & square_bb(stop))) score.eg += 5 * w;
        }
        // Rule of the square: in a pure pawn ending an unstoppable passer is worth almost a queen
        if (theirPawnsOnly) {
            int pawnDist = std::min(5, 7 - rr);
            int kingDist = dist(theirK, queenSq) - (pos.side_to_move() == them ? 1 : 0);
            Bitboard path = 0;
            for (Square t = stop; ; t = (us == WHITE ? t + 8 : t - 8)) {
                path |= square_bb(t);
                if (t == queenSq) break;
            }
            if (kingDist > pawnDist && !(path & pos.pieces(us)))
                score.eg += 500;
        }
    }

    // Rooks on open / half-open files
    Bitboard rks = pos.pieces(us, ROOK);
    while (rks) {
        Square s = pop_lsb(rks);
        Bitboard fm = FileABB << file_of(s);
        if (!(fm & ourPawns)) {
            if (!(fm & theirPawns)) { score.mg += 20; score.eg += 10; }
            else { score.mg += 10; score.eg += 5; }
        }
    }

    return score;
}

// Endgame scaling (0.4.0): drawish material makes the score shrink toward zero,
// so the engine stops calling a dead draw "+3".
int scale_factor(const Position& pos, Color strong) {
    Color weak = ~strong;
    auto npm = [&](Color c) {
        return popcount(pos.pieces(c, KNIGHT)) * 320 + popcount(pos.pieces(c, BISHOP)) * 330
             + popcount(pos.pieces(c, ROOK)) * 500 + popcount(pos.pieces(c, QUEEN)) * 900;
    };
    int sp = popcount(pos.pieces(strong, PAWN));
    int snpm = npm(strong), wnpm = npm(weak);
    // Bare minors / nothing: insufficient material
    if (sp == 0 && snpm <= 330) return 0;
    // No pawns and less than a rook up: very hard to win
    if (sp == 0 && snpm - wnpm < 500) return 8;
    // One lonely minor + one pawn against pawns: usually a draw if the defender has pawns
    if (sp == 1 && snpm <= 330 && wnpm == 0 && popcount(pos.pieces(weak, PAWN)) >= 1) return 24;
    // Opposite-coloured bishops with only bishops left
    if (snpm == 330 && wnpm == 330 && pos.pieces(strong, BISHOP) && pos.pieces(weak, BISHOP)) {
        constexpr Bitboard DarkSquares = 0xAA55AA55AA55AA55ULL;
        bool sd = pos.pieces(strong, BISHOP) & DarkSquares;
        bool wd = pos.pieces(weak, BISHOP) & DarkSquares;
        if (sd != wd) return 32;
    }
    return 64;
}

} // namespace

Value evaluate(const Position& pos) {
    Tapered w = evaluate_side(pos, WHITE);
    Tapered b = evaluate_side(pos, BLACK);
    Tapered score = w - b;
    int phase = phase_weight(pos);
    int v = (score.mg * phase + score.eg * (24 - phase)) / 24;
    v = v * scale_factor(pos, v > 0 ? WHITE : BLACK) / 64;
    // Tempo
    v += (pos.side_to_move() == WHITE) ? 10 : -10;
    return Value(pos.side_to_move() == WHITE ? v : -v);
}

} // namespace lugh
