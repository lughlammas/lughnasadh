#include "bitboard.hpp"
#include <random>

namespace lugh {

Bitboard SquareBB[SQUARE_NB];
Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Magic RookMagics[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];
Bitboard RookTable[0x19000];
Bitboard BishopTable[0x1480];

namespace {

Bitboard sliding_attack(PieceType pt, Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    static const int RookDirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
    static const int BishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    const int (*dirs)[2] = (pt == ROOK) ? RookDirs : BishopDirs;
    for (int d = 0; d < 4; ++d) {
        int f = file_of(sq) + dirs[d][0];
        int r = rank_of(sq) + dirs[d][1];
        while (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
            Square s = make_square(File(f), Rank(r));
            attacks |= square_bb(s);
            if (occupied & square_bb(s)) break;
            f += dirs[d][0];
            r += dirs[d][1];
        }
    }
    return attacks;
}

Bitboard sliding_mask(PieceType pt, Square sq) {
    Bitboard mask = 0;
    static const int RookDirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
    static const int BishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    const int (*dirs)[2] = (pt == ROOK) ? RookDirs : BishopDirs;
    for (int d = 0; d < 4; ++d) {
        int f = file_of(sq) + dirs[d][0];
        int r = rank_of(sq) + dirs[d][1];
        while (true) {
            int nf = f + dirs[d][0];
            int nr = r + dirs[d][1];
            if (nf < 0 || nf > 7 || nr < 0 || nr > 7) break;
            mask |= square_bb(make_square(File(f), Rank(r)));
            f = nf;
            r = nr;
        }
    }
    return mask;
}

uint64_t sparse_rand(std::mt19937_64& rng) {
    return rng() & rng() & rng();
}

void init_magics(PieceType pt, Magic magics[], Bitboard table[]) {
    std::mt19937_64 rng(239070430ULL ^ (uint64_t(pt) * 7919ULL));
    Bitboard occupancy[4096], reference[4096];
    int epoch[4096] = {};
    int cnt = 0, size = 0;

    for (int si = 0; si < 64; ++si) {
        Square s = Square(si);
        Magic& m = magics[s];
        m.mask = sliding_mask(pt, s);
        m.shift = 64 - popcount(m.mask);
        m.attacks = (si == 0) ? table : magics[si - 1].attacks + size;

        Bitboard b = 0;
        size = 0;
        do {
            occupancy[size] = b;
            reference[size] = sliding_attack(pt, s, b);
            size++;
            b = (b - m.mask) & m.mask;
        } while (b);

        for (int i = 0; i < size; ) {
            for (m.magic = 0; popcount((m.magic * m.mask) >> 56) < 6; )
                m.magic = sparse_rand(rng);

            ++cnt;
            for (i = 0; i < size; ++i) {
                unsigned idx = unsigned(m.index(occupancy[i]));
                if (epoch[idx] < cnt) {
                    epoch[idx] = cnt;
                    m.attacks[idx] = reference[i];
                } else if (m.attacks[idx] != reference[i]) {
                    break;
                }
            }
        }
    }
}

} // namespace

void init_bitboards() {
    for (int s = 0; s < 64; ++s)
        SquareBB[s] = 1ULL << s;

    for (int si = 0; si < 64; ++si) {
        Square s = Square(si);
        int f = file_of(s), r = rank_of(s);

        PawnAttacks[WHITE][s] = 0;
        PawnAttacks[BLACK][s] = 0;
        if (f > 0) {
            if (r < 7) PawnAttacks[WHITE][s] |= SquareBB[si + 7];
            if (r > 0) PawnAttacks[BLACK][s] |= SquareBB[si - 9];
        }
        if (f < 7) {
            if (r < 7) PawnAttacks[WHITE][s] |= SquareBB[si + 9];
            if (r > 0) PawnAttacks[BLACK][s] |= SquareBB[si - 7];
        }

        Bitboard kn = 0;
        static const int knD[8][2] = {
            {1,2},{1,-2},{-1,2},{-1,-2},{2,1},{2,-1},{-2,1},{-2,-1}
        };
        for (auto& d : knD) {
            int nf = f + d[0], nr = r + d[1];
            if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7)
                kn |= SquareBB[make_square(File(nf), Rank(nr))];
        }
        PseudoAttacks[KNIGHT][s] = kn;

        Bitboard kg = 0;
        for (int df = -1; df <= 1; ++df)
            for (int dr = -1; dr <= 1; ++dr) {
                if (!df && !dr) continue;
                int nf = f + df, nr = r + dr;
                if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7)
                    kg |= SquareBB[make_square(File(nf), Rank(nr))];
            }
        PseudoAttacks[KING][s] = kg;
    }

    init_magics(ROOK, RookMagics, RookTable);
    init_magics(BISHOP, BishopMagics, BishopTable);

    for (int si = 0; si < 64; ++si) {
        Square s = Square(si);
        PseudoAttacks[BISHOP][s] = attacks_bb(BISHOP, s, 0);
        PseudoAttacks[ROOK][s] = attacks_bb(ROOK, s, 0);
        PseudoAttacks[QUEEN][s] = PseudoAttacks[BISHOP][s] | PseudoAttacks[ROOK][s];
    }

    // Ray-walk BetweenBB (exclusive) and full-line LineBB through both squares
    static const int Dirs[8] = {8, -8, 1, -1, 9, -9, 7, -7};
    auto step_ok = [](int from, int delta, int to) {
        int f = from & 7, r = from >> 3, nf = to & 7, nr = to >> 3;
        int df = nf - f, dr = nr - r;
        if (delta == 8 || delta == -8) return df == 0;
        if (delta == 1 || delta == -1) return dr == 0;
        if (delta == 9 || delta == -9) return df == dr;
        if (delta == 7 || delta == -7) return df == -dr;
        return false;
    };
    auto walk = [&](int start, int delta) {
        Bitboard b = 0;
        int sq = start;
        while (true) {
            int nsq = sq + delta;
            if (nsq < 0 || nsq > 63 || !step_ok(sq, delta, nsq)) break;
            sq = nsq;
            b |= SquareBB[sq];
        }
        return b;
    };
    for (int s1 = 0; s1 < 64; ++s1)
        for (int s2 = 0; s2 < 64; ++s2) {
            BetweenBB[s1][s2] = 0;
            LineBB[s1][s2] = 0;
            if (s1 == s2) continue;
            for (int delta : Dirs) {
                Bitboard forward = 0;
                int sq = s1;
                bool hit = false;
                while (true) {
                    int nsq = sq + delta;
                    if (nsq < 0 || nsq > 63 || !step_ok(sq, delta, nsq)) break;
                    sq = nsq;
                    forward |= SquareBB[sq];
                    if (sq == s2) { hit = true; break; }
                }
                if (hit) {
                    BetweenBB[s1][s2] = forward & ~SquareBB[s2];
                    // Full line: both directions from s1, plus s1
                    LineBB[s1][s2] = SquareBB[s1] | walk(s1, delta) | walk(s1, -delta);
                    break;
                }
            }
        }
}

} // namespace lugh
