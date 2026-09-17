#pragma once
#include <cstdint>
#include <string>
#include <cassert>
#include <algorithm>
#include <cstring>

namespace lugh {

using Bitboard = uint64_t;
using Key = uint64_t;
using Score = int32_t;
using Depth = int32_t;
using Value = int32_t;

constexpr Value VALUE_ZERO = 0;
constexpr Value VALUE_DRAW = 0;
constexpr Value VALUE_MATE = 32000;
constexpr Value VALUE_MATE_IN_MAX = VALUE_MATE - 256;
constexpr Value VALUE_INFINITE = 32001;
constexpr Value VALUE_NONE = 32002;

enum Color : int { WHITE = 0, BLACK = 1, COLOR_NB = 2 };
enum PieceType : int {
    NO_PIECE_TYPE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3,
    ROOK = 4, QUEEN = 5, KING = 6, PIECE_TYPE_NB = 7
};
enum Piece : int {
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};
enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE = 64, SQUARE_NB = 64
};
enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };
enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };
enum CastlingRights : int {
    NO_CASTLING = 0,
    WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8,
    KING_SIDE = WHITE_OO | BLACK_OO,
    QUEEN_SIDE = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING
};

constexpr Color operator~(Color c) { return Color(c ^ 1); }
inline PieceType& operator++(PieceType& pt) { return pt = PieceType(int(pt) + 1); }
inline Square operator+(Square s, int d) { return Square(int(s) + d); }
inline Square operator-(Square s, int d) { return Square(int(s) - d); }

constexpr PieceType type_of(Piece pc) { return PieceType(pc & 7); }
constexpr Color color_of(Piece pc) { return Color(pc >> 3); }
constexpr Piece make_piece(Color c, PieceType pt) { return Piece((c << 3) | pt); }
constexpr Square make_square(File f, Rank r) { return Square((r << 3) | f); }
constexpr File file_of(Square s) { return File(s & 7); }
constexpr Rank rank_of(Square s) { return Rank(s >> 3); }
constexpr Square relative_square(Color c, Square s) {
    return Square(s ^ (c == WHITE ? 0 : 56));
}
constexpr Rank relative_rank(Color c, Rank r) {
    return Rank(r ^ (c == WHITE ? 0 : 7));
}
constexpr Rank relative_rank(Color c, Square s) {
    return relative_rank(c, rank_of(s));
}

inline Bitboard square_bb(Square s) { return 1ULL << s; }

enum MoveType : int {
    NORMAL = 0, PROMOTION = 1 << 14, EN_PASSANT = 2 << 14, CASTLING = 3 << 14
};

// Move encoding: from (6) | to (6) | promotion piece type-1 in bits 12-13 with PROMOTION flag
// bits 0-5: from, 6-11: to, 12-13: promo (N=0,B=1,R=2,Q=3), 14-15: MoveType
struct Move {
    uint16_t data = 0;
    constexpr Move() = default;
    constexpr explicit Move(uint16_t d) : data(d) {}
    constexpr Square from() const { return Square(data & 0x3F); }
    constexpr Square to() const { return Square((data >> 6) & 0x3F); }
    constexpr MoveType type() const { return MoveType(data & (3 << 14)); }
    constexpr PieceType promotion_type() const {
        return PieceType(((data >> 12) & 3) + KNIGHT);
    }
    constexpr bool none() const { return data == 0; }
    constexpr explicit operator bool() const { return data != 0; }
    constexpr bool operator==(Move m) const { return data == m.data; }
    constexpr bool operator!=(Move m) const { return data != m.data; }

    static constexpr Move make(Square from, Square to) {
        return Move(uint16_t(from | (to << 6)));
    }
    static constexpr Move make(Square from, Square to, MoveType mt, PieceType pt = KNIGHT) {
        return Move(uint16_t(from | (to << 6) | mt | ((pt - KNIGHT) << 12)));
    }
    static constexpr Move null() { return Move(0); }
};

inline std::string square_str(Square s) {
    return std::string{char('a' + file_of(s)), char('1' + rank_of(s))};
}
inline std::string move_str(Move m) {
    if (m.none()) return "0000";
    std::string s = square_str(m.from()) + square_str(m.to());
    if (m.type() == PROMOTION) {
        static const char pcs[] = " nbrq";
        s += pcs[m.promotion_type()];
    }
    return s;
}

constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY = 128;

struct ExtMove {
    Move move;
    int score = 0;
};

struct MoveList {
    ExtMove moves[MAX_MOVES];
    int size = 0;
    ExtMove* begin() { return moves; }
    ExtMove* end() { return moves + size; }
    const ExtMove* begin() const { return moves; }
    const ExtMove* end() const { return moves + size; }
    void push(Move m) { moves[size++].move = m; }
    void push(Move m, int sc) { moves[size].move = m; moves[size++].score = sc; }
};

} // namespace lugh
