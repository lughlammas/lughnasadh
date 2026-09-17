#pragma once
#include "types.hpp"

namespace lugh {

constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

constexpr Bitboard Rank1BB = 0xFFULL;
constexpr Bitboard Rank2BB = Rank1BB << 8;
constexpr Bitboard Rank3BB = Rank1BB << 16;
constexpr Bitboard Rank4BB = Rank1BB << 24;
constexpr Bitboard Rank5BB = Rank1BB << 32;
constexpr Bitboard Rank6BB = Rank1BB << 40;
constexpr Bitboard Rank7BB = Rank1BB << 48;
constexpr Bitboard Rank8BB = Rank1BB << 56;

inline int popcount(Bitboard b) { return __builtin_popcountll(b); }
inline Square lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }
inline Square pop_lsb(Bitboard& b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}
inline bool more_than_one(Bitboard b) { return b & (b - 1); }

inline Bitboard shift_bb(Bitboard b, int d) {
    // directional shifts with file wrap protection applied by caller
    if (d > 0) return b << d;
    return b >> -d;
}

template<Color C>
constexpr Bitboard pawn_push(Bitboard b) {
    return C == WHITE ? (b << 8) : (b >> 8);
}

void init_bitboards();

extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];

// Magic bitboards for sliding pieces
struct Magic {
    Bitboard* attacks;
    Bitboard mask;
    Bitboard magic;
    int shift;
    Bitboard index(Bitboard occupied) const {
        return ((occupied & mask) * magic) >> shift;
    }
};

extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];
extern Bitboard RookTable[0x19000];
extern Bitboard BishopTable[0x1480];

inline Bitboard attacks_bb(PieceType pt, Square s, Bitboard occ) {
    switch (pt) {
    case BISHOP: {
        const Magic& m = BishopMagics[s];
        return m.attacks[m.index(occ)];
    }
    case ROOK: {
        const Magic& m = RookMagics[s];
        return m.attacks[m.index(occ)];
    }
    case QUEEN:
        return attacks_bb(BISHOP, s, occ) | attacks_bb(ROOK, s, occ);
    default:
        return PseudoAttacks[pt][s];
    }
}

inline Bitboard pawn_attacks_bb(Color c, Square s) { return PawnAttacks[c][s]; }
inline Bitboard pawn_attacks_bb(Color c, Bitboard b) {
    return c == WHITE
        ? ((b & ~FileHBB) << 9) | ((b & ~FileABB) << 7)
        : ((b & ~FileABB) >> 9) | ((b & ~FileHBB) >> 7);
}

} // namespace lugh
