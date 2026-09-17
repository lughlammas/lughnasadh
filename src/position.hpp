#pragma once
#include "bitboard.hpp"
#include <string>
#include <vector>

namespace lugh {

struct StateInfo {
    Key key = 0;
    Bitboard checkers = 0;
    Bitboard blockers[COLOR_NB] = {};
    Bitboard pinners[COLOR_NB] = {};
    Square ep_square = SQ_NONE;
    int castling = NO_CASTLING;
    int rule50 = 0;
    int plies_from_null = 0;
    Piece captured = NO_PIECE;
    StateInfo* previous = nullptr;
};

class Position {
public:
    void set(const std::string& fen, StateInfo* si);
    void set_startpos(StateInfo* si);
    std::string fen() const;

    Bitboard pieces() const { return by_type[0]; }
    Bitboard pieces(PieceType pt) const { return by_type[pt]; }
    Bitboard pieces(Color c) const { return by_color[c]; }
    Bitboard pieces(Color c, PieceType pt) const { return by_color[c] & by_type[pt]; }
    Bitboard pieces(Color c, PieceType p1, PieceType p2) const {
        return by_color[c] & (by_type[p1] | by_type[p2]);
    }
    Bitboard pieces(PieceType p1, PieceType p2) const {
        return by_type[p1] | by_type[p2];
    }
    Bitboard castling_path_bb(int cr) const { return castling_path[cr]; }
    Square castling_rook_sq(int cr) const { return castling_rook_square[cr]; }
    Piece piece_on(Square s) const { return board[s]; }
    Square king_square(Color c) const { return king_sq[c]; }
    Color side_to_move() const { return side; }
    Square ep_square() const { return st->ep_square; }
    int castling_rights() const { return st->castling; }
    Bitboard checkers() const { return st->checkers; }
    bool in_check() const { return st->checkers != 0; }
    Key key() const { return st->key; }
    int game_ply() const { return gamePly; }
    int rule50_count() const { return st->rule50; }
    bool is_capture(Move m) const {
        return board[m.to()] != NO_PIECE || m.type() == EN_PASSANT;
    }
    Piece captured_piece() const { return st->captured; }

    bool legal(Move m) const;
    bool pseudo_legal(Move m) const;
    bool gives_check(Move m) const;

    void do_move(Move m, StateInfo& new_st);
    void undo_move(Move m);
    void do_null_move(StateInfo& new_st);
    void undo_null_move();

    Bitboard attackers_to(Square s, Bitboard occ) const;
    Bitboard attackers_to(Square s) const { return attackers_to(s, pieces()); }
    template<Color Us> void update_slider_blockers(StateInfo* state) const;

    Value see_ge(Move m, Value threshold = 0) const;
    int see_signed(Move m) const;

    bool is_draw(int ply) const;
    bool has_repeated() const;

    StateInfo* state() const { return st; }

private:
    void put_piece(Piece pc, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);
    void set_castling_right(Color c, Square rook_from);
    void set_state();
    Key compute_key() const;
    template<Color Us> Bitboard compute_checkers() const;

    Piece board[SQUARE_NB] = {};
    Bitboard by_type[PIECE_TYPE_NB] = {};
    Bitboard by_color[COLOR_NB] = {};
    Square king_sq[COLOR_NB] = {SQ_NONE, SQ_NONE};
    int piece_count[PIECE_NB] = {};
    Color side = WHITE;
    int gamePly = 0;
    StateInfo* st = nullptr;

    // Castling lookup
    Square castling_rook_square[16] = {};
    Bitboard castling_path[16] = {};
    int castling_rights_mask[SQUARE_NB] = {};
};

void init_zobrist();
extern Key ZobristPsq[PIECE_NB][SQUARE_NB];
extern Key ZobristEp[8];
extern Key ZobristCastling[16];
extern Key ZobristSide;

} // namespace lugh
