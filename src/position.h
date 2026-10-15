// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "bitboard.h"
#include "types.h"

namespace sp {

// Everything about a position that cannot be cheaply recomputed after a move
// is undone. One StateInfo is pushed per move so that undo is a pop.
struct StateInfo {
    Key    key;
    Key    pawnKey;
    int    castlingRights;
    Square epSquare;
    int    rule50;
    int    pliesFromNull;
    Piece  capturedPiece;
    int    repetition;

    Bitboard checkersBB;
    Bitboard blockersForKing[COLOR_NB];
    Bitboard pinners[COLOR_NB];
    Bitboard checkSquares[PIECE_TYPE_NB];
};

// A chess position with full move history. Positions are plain values and may
// be copied, which is how each search thread gets its own root.
class Position {
   public:
    static void init();

    Position();

    Position&   set(std::string_view fen);
    std::string fen() const;

    // Board layout
    Bitboard pieces(PieceType pt = ALL_PIECES) const { return byTypeBB[pt]; }
    template <typename... PieceTypes>
    Bitboard pieces(PieceType pt, PieceTypes... pts) const {
        return pieces(pt) | pieces(pts...);
    }
    Bitboard pieces(Color c) const { return byColorBB[c]; }
    template <typename... PieceTypes>
    Bitboard pieces(Color c, PieceTypes... pts) const {
        return pieces(c) & pieces(pts...);
    }
    Piece piece_on(Square s) const { return board[s]; }
    bool  empty(Square s) const { return board[s] == NO_PIECE; }
    template <PieceType Pt>
    int count(Color c) const {
        return popcount(pieces(c, Pt));
    }
    template <PieceType Pt>
    int count() const {
        return popcount(pieces(Pt));
    }
    template <PieceType Pt>
    Square square(Color c) const {
        assert(count<Pt>(c) == 1);
        return lsb(pieces(c, Pt));
    }

    // Castling
    bool   can_castle(CastlingRights cr) const { return st().castlingRights & cr; }
    bool   castling_impeded(CastlingRights cr) const { return pieces() & castlingPath[cr]; }
    Square castling_rook_square(CastlingRights cr) const { return castlingRookSquare[cr]; }
    Square castling_king_to(CastlingRights cr) const { return castlingKingTo[cr]; }

    // Checking
    Bitboard checkers() const { return st().checkersBB; }
    Bitboard blockers_for_king(Color c) const { return st().blockersForKing[c]; }
    Bitboard check_squares(PieceType pt) const { return st().checkSquares[pt]; }
    Bitboard pinners(Color c) const { return st().pinners[c]; }
    Square   ep_square() const { return st().epSquare; }

    // Attacks to and from a square
    Bitboard attackers_to(Square s) const { return attackers_to(s, pieces()); }
    Bitboard attackers_to(Square s, Bitboard occupied) const;
    bool     attackers_to_exist(Square s, Bitboard occupied, Color c) const;

    // Move properties
    bool  legal(Move m) const;
    bool  pseudo_legal(Move m) const;
    bool  capture(Move m) const;
    bool  capture_stage(Move m) const;
    bool  gives_check(Move m) const;
    Piece moved_piece(Move m) const { return board[m.from()]; }
    Piece captured_piece() const { return st().capturedPiece; }

    // Doing and undoing moves
    void do_move(Move m);
    void do_move(Move m, bool givesCheck);
    void undo_move(Move m);
    void do_null_move();
    void undo_null_move();

    // Static exchange evaluation: true if the exchange on m's destination
    // square wins at least `threshold` centipawns.
    bool see_ge(Move m, int threshold = 0) const;

    // Hashing and state
    Key   key() const { return st().key; }
    Key   pawn_key() const { return st().pawnKey; }
    Key   key_after(Move m) const;
    Color side_to_move() const { return sideToMove; }
    int   game_ply() const { return gamePly; }
    int   rule50_count() const { return st().rule50; }
    bool  is_draw(int ply) const;
    bool  has_repeated() const;
    Value non_pawn_material(Color c) const;
    Value non_pawn_material() const { return non_pawn_material(WHITE) + non_pawn_material(BLACK); }

    // A UCI move string resolved against this position's legal moves, or
    // Move::none() if it does not match one.
    Move uci_to_move(std::string_view str) const;

    std::string pretty() const;
    bool        pos_is_ok() const;

   private:
    void set_castling_right(Color c, Square rfrom);
    void set_state();
    void set_check_info();
    void update_slider_blockers(Color c);
    template <bool Do>
    void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

    void put_piece(Piece pc, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);

    StateInfo&       st() { return states.back(); }
    const StateInfo& st() const { return states.back(); }

    Piece    board[SQUARE_NB];
    Bitboard byTypeBB[PIECE_TYPE_NB];
    Bitboard byColorBB[COLOR_NB];
    int      castlingRightsMask[SQUARE_NB];
    Square   castlingRookSquare[CASTLING_RIGHT_NB];
    Square   castlingKingTo[CASTLING_RIGHT_NB];
    Bitboard castlingPath[CASTLING_RIGHT_NB];
    Color    sideToMove;
    int      gamePly;

    std::vector<StateInfo> states;
};

inline bool Position::capture(Move m) const {
    assert(m.is_ok());
    return (!empty(m.to()) && m.type() != CASTLING) || m.type() == EN_PASSANT;
}

// Captures and queen promotions are searched in the capture stage.
inline bool Position::capture_stage(Move m) const {
    assert(m.is_ok());
    return capture(m) || (m.type() == PROMOTION && m.promotion_type() == QUEEN);
}

inline void Position::put_piece(Piece pc, Square s) {
    board[s] = pc;
    byTypeBB[ALL_PIECES] |= byTypeBB[type_of(pc)] |= s;
    byColorBB[color_of(pc)] |= s;
}

inline void Position::remove_piece(Square s) {
    const Piece pc = board[s];
    byTypeBB[ALL_PIECES] ^= s;
    byTypeBB[type_of(pc)] ^= s;
    byColorBB[color_of(pc)] ^= s;
    board[s] = NO_PIECE;
}

inline void Position::move_piece(Square from, Square to) {
    const Piece    pc     = board[from];
    const Bitboard fromTo = square_bb(from) | square_bb(to);
    byTypeBB[ALL_PIECES] ^= fromTo;
    byTypeBB[type_of(pc)] ^= fromTo;
    byColorBB[color_of(pc)] ^= fromTo;
    board[from] = NO_PIECE;
    board[to]   = pc;
}

inline void Position::do_move(Move m) {
    do_move(m, gives_check(m));
}

}  // namespace sp
