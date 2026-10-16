// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "position.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <string>

#include "movegen.h"

namespace sp {

namespace Zobrist {
Key psq[PIECE_NB][SQUARE_NB];
Key enpassant[FILE_NB];
Key castling[CASTLING_RIGHT_NB];
Key side;
Key noPawns;
}  // namespace Zobrist

namespace {

constexpr std::string_view PieceToChar(" PNBRQK  pnbrqk");

// Deterministic generator so that hash keys are identical across runs and
// platforms, which keeps `bench` reproducible.
struct ZobristPRNG {
    std::uint64_t s = 1070372;
    std::uint64_t next() {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }
};

}  // namespace

void Position::init() {
    ZobristPRNG rng;

    for (Piece pc :
         {W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING, B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING})
        for (Square s = SQ_A1; s <= SQ_H8; ++s)
            Zobrist::psq[pc][s] = rng.next();

    for (File f = FILE_A; f <= FILE_H; ++f)
        Zobrist::enpassant[f] = rng.next();

    for (int cr = NO_CASTLING; cr < CASTLING_RIGHT_NB; ++cr)
        Zobrist::castling[cr] = rng.next();

    Zobrist::side    = rng.next();
    Zobrist::noPawns = rng.next();
}

Position::Position() {
    states.reserve(1024);
    accumulatorStack.reserve(1024);
    set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

// Parses a FEN string. Malformed input is tolerated where possible; a position
// that fails pos_is_ok() afterwards is the caller's problem.
Position& Position::set(std::string_view fenString) {
    std::memset(board, NO_PIECE, sizeof(board));
    std::memset(byTypeBB, 0, sizeof(byTypeBB));
    std::memset(byColorBB, 0, sizeof(byColorBB));
    std::memset(castlingRightsMask, 0, sizeof(castlingRightsMask));
    std::memset(castlingPath, 0, sizeof(castlingPath));
    std::fill(std::begin(castlingRookSquare), std::end(castlingRookSquare), SQ_NONE);
    std::fill(std::begin(castlingKingTo), std::end(castlingKingTo), SQ_NONE);

    states.clear();
    states.emplace_back();
    StateInfo& si = st();
    std::memset(static_cast<void*>(&si), 0, sizeof(StateInfo));
    si.epSquare      = SQ_NONE;
    si.capturedPiece = NO_PIECE;

    accumulatorStack.clear();
    accumulatorStack.emplace_back();
    accumulatorStack.back().computed = false;

    std::istringstream ss{std::string(fenString)};
    ss >> std::noskipws;

    unsigned char token;
    Square        sq = SQ_A8;

    // 1. Piece placement
    while ((ss >> token) && !std::isspace(token)) {
        if (std::isdigit(token))
            sq += (token - '0') * EAST;
        else if (token == '/')
            sq += 2 * SOUTH;
        else if (auto idx = PieceToChar.find(char(token)); idx != std::string_view::npos && is_ok(sq)) {
            put_piece(Piece(idx), sq);
            ++sq;
        }
    }

    // 2. Active colour
    ss >> token;
    sideToMove = token == 'w' ? WHITE : BLACK;
    ss >> token;

    // 3. Castling availability. Standard chess only: K, Q, k, q.
    while ((ss >> token) && !std::isspace(token)) {
        const Color c = std::isupper(token) ? WHITE : BLACK;
        token         = std::toupper(token);

        if (token == 'K' || token == 'Q') {
            const Square ksq = pieces(c, KING) ? square<KING>(c) : SQ_NONE;
            if (ksq == SQ_NONE)
                continue;
            Square rsq = relative_square(c, token == 'K' ? SQ_H1 : SQ_A1);
            if (piece_on(rsq) == make_piece(c, ROOK) && rank_of(ksq) == relative_rank(c, RANK_1))
                set_castling_right(c, rsq);
        }
    }

    // 4. En passant square. Only kept when a capture is actually possible so
    // that keys of equivalent positions match.
    bool          enpassant = false;
    unsigned char col, row;
    if (((ss >> col) && (col >= 'a' && col <= 'h')) && ((ss >> row) && (row == (sideToMove == WHITE ? '6' : '3')))) {
        si.epSquare = make_square(File(col - 'a'), Rank(row - '1'));

        enpassant = (pawn_attacks_bb(~sideToMove, si.epSquare) & pieces(sideToMove, PAWN))
                 && (pieces(~sideToMove, PAWN) & (si.epSquare + pawn_push(~sideToMove)))
                 && !(pieces() & (si.epSquare | (si.epSquare + pawn_push(sideToMove))));
    }
    if (!enpassant)
        si.epSquare = SQ_NONE;

    // 5-6. Halfmove clock and fullmove number
    ss >> std::skipws >> si.rule50 >> gamePly;
    gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);

    set_state();
    return *this;
}

void Position::set_castling_right(Color c, Square rfrom) {
    const Square         kfrom = square<KING>(c);
    const CastlingRights cr    = c & (kfrom < rfrom ? KING_SIDE : QUEEN_SIDE);

    st().castlingRights |= cr;
    castlingRightsMask[kfrom] |= cr;
    castlingRightsMask[rfrom] |= cr;
    castlingRookSquare[cr] = rfrom;

    const Square kto   = relative_square(c, cr & KING_SIDE ? SQ_G1 : SQ_C1);
    const Square rto   = relative_square(c, cr & KING_SIDE ? SQ_F1 : SQ_D1);
    castlingKingTo[cr] = kto;

    castlingPath[cr] = (between_bb(rfrom, rto) | between_bb(kfrom, kto)) & ~(square_bb(kfrom) | square_bb(rfrom));
}

// Fills the derived fields of the current state from scratch.
void Position::set_state() {
    StateInfo& si = st();
    si.key        = 0;
    si.pawnKey    = Zobrist::noPawns;
    si.checkersBB = attackers_to(square<KING>(sideToMove)) & pieces(~sideToMove);

    set_check_info();

    for (Bitboard b = pieces(); b;) {
        const Square s  = pop_lsb(b);
        const Piece  pc = piece_on(s);
        si.key ^= Zobrist::psq[pc][s];
        if (type_of(pc) == PAWN)
            si.pawnKey ^= Zobrist::psq[pc][s];
    }

    if (si.epSquare != SQ_NONE)
        si.key ^= Zobrist::enpassant[file_of(si.epSquare)];

    if (sideToMove == BLACK)
        si.key ^= Zobrist::side;

    si.key ^= Zobrist::castling[si.castlingRights];
}

// Pieces of colour c that stand between their own king and an enemy slider
// (the pinned pieces) and enemy pieces that could give a discovered check.
void Position::update_slider_blockers(Color c) {
    const Square ksq = square<KING>(c);

    st().blockersForKing[c] = 0;
    st().pinners[~c]        = 0;

    Bitboard snipers =
        ((attacks_bb<ROOK>(ksq) & pieces(QUEEN, ROOK)) | (attacks_bb<BISHOP>(ksq) & pieces(QUEEN, BISHOP)))
        & pieces(~c);
    const Bitboard occupancy = pieces() ^ snipers;

    while (snipers) {
        const Square   sniperSq = pop_lsb(snipers);
        const Bitboard b        = between_bb(ksq, sniperSq) & occupancy;

        if (b && !more_than_one(b)) {
            st().blockersForKing[c] |= b;
            if (b & pieces(c))
                st().pinners[~c] |= sniperSq;
        }
    }
}

void Position::set_check_info() {
    update_slider_blockers(WHITE);
    update_slider_blockers(BLACK);

    const Square ksq = square<KING>(~sideToMove);
    StateInfo&   si  = st();

    si.checkSquares[PAWN]   = pawn_attacks_bb(~sideToMove, ksq);
    si.checkSquares[KNIGHT] = attacks_bb<KNIGHT>(ksq);
    si.checkSquares[BISHOP] = attacks_bb<BISHOP>(ksq, pieces());
    si.checkSquares[ROOK]   = attacks_bb<ROOK>(ksq, pieces());
    si.checkSquares[QUEEN]  = si.checkSquares[BISHOP] | si.checkSquares[ROOK];
    si.checkSquares[KING]   = 0;
}

std::string Position::fen() const {
    std::ostringstream ss;

    for (Rank r = RANK_8; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_H; ++f) {
            int emptyCnt = 0;
            for (; f <= FILE_H && empty(make_square(f, r)); ++f)
                ++emptyCnt;
            if (emptyCnt)
                ss << emptyCnt;
            if (f <= FILE_H)
                ss << PieceToChar[piece_on(make_square(f, r))];
        }
        if (r > RANK_1)
            ss << '/';
    }

    ss << (sideToMove == WHITE ? " w " : " b ");

    if (can_castle(WHITE_OO))
        ss << 'K';
    if (can_castle(WHITE_OOO))
        ss << 'Q';
    if (can_castle(BLACK_OO))
        ss << 'k';
    if (can_castle(BLACK_OOO))
        ss << 'q';
    if (!can_castle(ANY_CASTLING))
        ss << '-';

    ss << (st().epSquare == SQ_NONE ? " - " : " " + square_to_string(st().epSquare) + " ") << st().rule50 << " "
       << 1 + (gamePly - (sideToMove == BLACK)) / 2;

    return ss.str();
}

std::string Position::pretty() const {
    std::ostringstream ss;
    ss << "\n +---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_H; ++f)
            ss << " | " << PieceToChar[piece_on(make_square(f, r))];
        ss << " | " << (1 + r) << "\n +---+---+---+---+---+---+---+---+\n";
    }

    ss << "   a   b   c   d   e   f   g   h\n"
       << "\nFen: " << fen() << "\nKey: " << std::hex << std::uppercase << key() << std::dec << "\nCheckers: ";
    for (Bitboard b = checkers(); b;)
        ss << square_to_string(pop_lsb(b)) << " ";
    ss << "\n";
    return ss.str();
}

// All pieces of either colour attacking square s with the given occupancy.
Bitboard Position::attackers_to(Square s, Bitboard occupied) const {
    return (pawn_attacks_bb(BLACK, s) & pieces(WHITE, PAWN)) | (pawn_attacks_bb(WHITE, s) & pieces(BLACK, PAWN))
         | (attacks_bb<KNIGHT>(s) & pieces(KNIGHT)) | (attacks_bb<ROOK>(s, occupied) & pieces(ROOK, QUEEN))
         | (attacks_bb<BISHOP>(s, occupied) & pieces(BISHOP, QUEEN)) | (attacks_bb<KING>(s) & pieces(KING));
}

bool Position::attackers_to_exist(Square s, Bitboard occupied, Color c) const {
    return ((attacks_bb<ROOK>(s) & pieces(c, ROOK, QUEEN)) && (attacks_bb<ROOK>(s, occupied) & pieces(c, ROOK, QUEEN)))
        || ((attacks_bb<BISHOP>(s) & pieces(c, BISHOP, QUEEN))
            && (attacks_bb<BISHOP>(s, occupied) & pieces(c, BISHOP, QUEEN)))
        || (((pawn_attacks_bb(~c, s) & pieces(PAWN)) | (attacks_bb<KNIGHT>(s) & pieces(KNIGHT))
             | (attacks_bb<KING>(s) & pieces(KING)))
            & pieces(c));
}

// Tests whether a pseudo-legal move leaves the king safe. Only moves of pinned
// pieces, king moves, en passant and castling can fail, so the move generator
// filters with this only where needed.
bool Position::legal(Move m) const {
    assert(m.is_ok());

    const Color  us   = sideToMove;
    const Square from = m.from();
    Square       to   = m.to();

    assert(color_of(moved_piece(m)) == us);
    assert(piece_on(square<KING>(us)) == make_piece(us, KING));

    // En passant: the captured pawn and the capturing pawn both leave their
    // squares, which can expose the king along a rank or diagonal.
    if (m.type() == EN_PASSANT) {
        const Square   ksq      = square<KING>(us);
        const Square   capsq    = to - pawn_push(us);
        const Bitboard occupied = (pieces() ^ from ^ capsq) | to;

        assert(to == st().epSquare);
        assert(moved_piece(m) == make_piece(us, PAWN));
        assert(piece_on(capsq) == make_piece(~us, PAWN));
        assert(piece_on(to) == NO_PIECE);

        return !(attacks_bb<ROOK>(ksq, occupied) & pieces(~us, QUEEN, ROOK))
            && !(attacks_bb<BISHOP>(ksq, occupied) & pieces(~us, QUEEN, BISHOP));
    }

    // Castling: no square the king crosses or lands on may be attacked. The
    // generator never produces castling moves while in check.
    if (m.type() == CASTLING) {
        const Direction step = to > from ? WEST : EAST;
        for (Square s = to; s != from; s += step)
            if (attackers_to(s) & pieces(~us))
                return false;
        return true;
    }

    // A king move must not land on an attacked square; the king itself is
    // removed from the occupancy so that it cannot shield the destination.
    if (type_of(piece_on(from)) == KING)
        return !(attackers_to(to, pieces() ^ from) & pieces(~us));

    // Any other piece is fine unless pinned and moving off the pin line.
    return !(blockers_for_king(us) & from) || aligned(from, to, square<KING>(us));
}

// Validates a move that came from outside the move generator (a hash move or
// a UCI string) by checking that the generator could have produced it.
bool Position::pseudo_legal(Move m) const {
    const Color  us   = sideToMove;
    const Square from = m.from();
    const Square to   = m.to();
    const Piece  pc   = moved_piece(m);

    // Special moves are rare; just regenerate and look for them.
    if (m.type() != NORMAL)
        return MoveList<LEGAL>(*this).contains(m);

    if (pc == NO_PIECE || color_of(pc) != us)
        return false;

    if (pieces(us) & to)
        return false;

    if (type_of(pc) == PAWN) {
        // Promotions are PROMOTION moves, never NORMAL ones.
        if ((Rank8BB | Rank1BB) & to)
            return false;

        if (!(pawn_attacks_bb(us, from) & pieces(~us) & to)  // capture
            && !((from + pawn_push(us) == to) && empty(to))  // single push
            && !((from + 2 * pawn_push(us) == to) && (relative_rank(us, from) == RANK_2) && empty(to)
                 && empty(to - pawn_push(us))))  // double push
            return false;
    } else if (!(attacks_bb(type_of(pc), from, pieces()) & to))
        return false;

    // In check, the move must be an evasion.
    if (checkers()) {
        if (type_of(pc) != KING) {
            if (more_than_one(checkers()))
                return false;
            // Must capture the checker or block the check.
            if (!(between_bb(square<KING>(us), lsb(checkers())) & to))
                return false;
        } else if (attackers_to(to, pieces() ^ from) & pieces(~us))
            return false;
    }

    return true;
}

bool Position::gives_check(Move m) const {
    assert(m.is_ok());
    assert(color_of(moved_piece(m)) == sideToMove);

    const Square from = m.from();
    const Square to   = m.to();

    // Direct check from the destination square
    if (check_squares(type_of(piece_on(from))) & to)
        return true;

    // Discovered check: the moving piece was blocking a line to the enemy king
    if (blockers_for_king(~sideToMove) & from)
        return !aligned(from, to, square<KING>(~sideToMove)) || m.type() == CASTLING;

    switch (m.type()) {
    case NORMAL :
        return false;

    case PROMOTION :
        return attacks_bb(m.promotion_type(), to, pieces() ^ from) & pieces(~sideToMove, KING);

    // En passant can give a discovered check through the captured pawn's square.
    case EN_PASSANT : {
        const Square   capsq = make_square(file_of(to), rank_of(from));
        const Bitboard b     = (pieces() ^ from ^ capsq) | to;
        const Square   ksq   = square<KING>(~sideToMove);

        return (attacks_bb<ROOK>(ksq, b) & pieces(sideToMove, QUEEN, ROOK))
             | (attacks_bb<BISHOP>(ksq, b) & pieces(sideToMove, QUEEN, BISHOP));
    }
    default : {  // CASTLING: only the rook can check
        const Square rto = relative_square(sideToMove, to > from ? SQ_F1 : SQ_D1);
        return check_squares(ROOK) & rto;
    }
    }
}

template <bool Do>
void Position::do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto) {
    const bool kingSide = to > from;
    rfrom               = relative_square(us, kingSide ? SQ_H1 : SQ_A1);
    rto                 = relative_square(us, kingSide ? SQ_F1 : SQ_D1);

    // Remove both pieces first, since squares may overlap in Chess960.
    remove_piece(Do ? from : to);
    remove_piece(Do ? rfrom : rto);
    put_piece(make_piece(us, KING), Do ? to : from);
    put_piece(make_piece(us, ROOK), Do ? rto : rfrom);
}

void Position::do_move(Move m, bool givesCheck) {
    assert(m.is_ok());

    Key k = st().key ^ Zobrist::side;

    // Copy the irreversible fields; the derived fields are recomputed below.
    states.push_back(states.back());
    StateInfo& si       = st();
    si.dirtyPiece.count = 0;

    accumulatorStack.emplace_back();
    accumulatorStack.back().computed = false;

    ++gamePly;
    ++si.rule50;
    ++si.pliesFromNull;

    const Color  us       = sideToMove;
    const Color  them     = ~us;
    const Square from     = m.from();
    Square       to       = m.to();
    const Piece  pc       = piece_on(from);
    Piece        captured = m.type() == EN_PASSANT ? make_piece(them, PAWN) : piece_on(to);

    assert(color_of(pc) == us);
    assert(captured == NO_PIECE || color_of(captured) == them);
    assert(type_of(captured) != KING);

    if (m.type() == CASTLING) {
        assert(pc == make_piece(us, KING));
        assert(captured == NO_PIECE);

        Square rfrom, rto;
        do_castling<true>(us, from, to, rfrom, rto);

        k ^= Zobrist::psq[make_piece(us, ROOK)][rfrom] ^ Zobrist::psq[make_piece(us, ROOK)][rto];
        si.dirtyPiece.move(make_piece(us, ROOK), rfrom, rto);
    }

    if (captured) {
        Square capsq = to;

        if (type_of(captured) == PAWN) {
            if (m.type() == EN_PASSANT) {
                capsq -= pawn_push(us);

                assert(pc == make_piece(us, PAWN));
                assert(to == si.epSquare);
                assert(relative_rank(us, to) == RANK_6);
                assert(piece_on(to) == NO_PIECE);
                assert(piece_on(capsq) == make_piece(them, PAWN));
            }
            si.pawnKey ^= Zobrist::psq[captured][capsq];
        }

        remove_piece(capsq);
        k ^= Zobrist::psq[captured][capsq];
        si.rule50 = 0;
        si.dirtyPiece.remove(captured, capsq);
    }

    k ^= Zobrist::psq[pc][from] ^ Zobrist::psq[pc][to];

    if (si.epSquare != SQ_NONE) {
        k ^= Zobrist::enpassant[file_of(si.epSquare)];
        si.epSquare = SQ_NONE;
    }

    if (si.castlingRights && (castlingRightsMask[from] | castlingRightsMask[to])) {
        k ^= Zobrist::castling[si.castlingRights];
        si.castlingRights &= ~(castlingRightsMask[from] | castlingRightsMask[to]);
        k ^= Zobrist::castling[si.castlingRights];
    }

    if (m.type() != CASTLING)
        move_piece(from, to);
    si.dirtyPiece.move(pc, from, to);

    if (type_of(pc) == PAWN) {
        // A double push sets the en passant square only if an enemy pawn can use it.
        if ((int(to) ^ int(from)) == 16 && (pawn_attacks_bb(us, to - pawn_push(us)) & pieces(them, PAWN))) {
            si.epSquare = to - pawn_push(us);
            k ^= Zobrist::enpassant[file_of(si.epSquare)];
        } else if (m.type() == PROMOTION) {
            const Piece promotion = make_piece(us, m.promotion_type());
            assert(relative_rank(us, to) == RANK_8);

            remove_piece(to);
            put_piece(promotion, to);

            k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[promotion][to];
            si.pawnKey ^= Zobrist::psq[pc][to];

            // The pawn's move above becomes a removal, and the promoted piece appears
            si.dirtyPiece.to[si.dirtyPiece.count - 1] = SQ_NONE;
            si.dirtyPiece.add(promotion, to);
        }

        si.pawnKey ^= Zobrist::psq[pc][from] ^ Zobrist::psq[pc][to];
        si.rule50 = 0;
    }

    si.capturedPiece = captured;
    si.key           = k;
    si.checkersBB    = givesCheck ? attackers_to(square<KING>(them)) & pieces(us) : 0;

    sideToMove = them;
    set_check_info();

    // Repetition detection: look back through positions with the same side to
    // move, as far as the last irreversible move allows.
    si.repetition = 0;
    const int end = std::min(si.rule50, si.pliesFromNull);
    if (end >= 4) {
        const std::size_t cur = states.size() - 1;
        for (int i = 4; i <= end; i += 2) {
            const StateInfo& prev = states[cur - i];
            if (prev.key == si.key) {
                si.repetition = prev.repetition ? -i : i;
                break;
            }
        }
    }

    assert(pos_is_ok());
}

void Position::undo_move(Move m) {
    assert(m.is_ok());

    sideToMove = ~sideToMove;

    const Color  us   = sideToMove;
    const Square from = m.from();
    Square       to   = m.to();
    Piece        pc   = piece_on(to);

    assert(empty(from) || m.type() == CASTLING);
    assert(type_of(st().capturedPiece) != KING);

    if (m.type() == PROMOTION) {
        assert(relative_rank(us, to) == RANK_8);
        assert(type_of(pc) == m.promotion_type());

        remove_piece(to);
        pc = make_piece(us, PAWN);
        put_piece(pc, to);
    }

    if (m.type() == CASTLING) {
        Square rfrom, rto;
        do_castling<false>(us, from, to, rfrom, rto);
    } else {
        move_piece(to, from);

        if (st().capturedPiece) {
            Square capsq = to;

            if (m.type() == EN_PASSANT) {
                capsq -= pawn_push(us);
                assert(type_of(pc) == PAWN);
                assert(to == states[states.size() - 2].epSquare);
                assert(relative_rank(us, to) == RANK_6);
                assert(piece_on(capsq) == NO_PIECE);
            }

            put_piece(st().capturedPiece, capsq);
        }
    }

    states.pop_back();
    accumulatorStack.pop_back();
    --gamePly;

    assert(pos_is_ok());
}

// A null move passes the turn. Used by null move pruning.
void Position::do_null_move() {
    assert(!checkers());

    states.push_back(states.back());
    StateInfo& si       = st();
    si.dirtyPiece.count = 0;  // nothing moved; the accumulator is the parent's

    accumulatorStack.emplace_back();
    accumulatorStack.back().computed = false;

    if (si.epSquare != SQ_NONE) {
        si.key ^= Zobrist::enpassant[file_of(si.epSquare)];
        si.epSquare = SQ_NONE;
    }

    si.key ^= Zobrist::side;
    ++si.rule50;
    si.pliesFromNull = 0;
    si.capturedPiece = NO_PIECE;

    sideToMove = ~sideToMove;
    set_check_info();

    si.repetition = 0;

    assert(pos_is_ok());
}

void Position::undo_null_move() {
    assert(!checkers());
    states.pop_back();
    accumulatorStack.pop_back();
    sideToMove = ~sideToMove;
}

// The hash key the position would have after m, without making the move. Used
// to prefetch transposition table entries.
Key Position::key_after(Move m) const {
    const Square from     = m.from();
    const Square to       = m.to();
    const Piece  pc       = piece_on(from);
    const Piece  captured = piece_on(to);
    Key          k        = st().key ^ Zobrist::side;

    if (captured)
        k ^= Zobrist::psq[captured][to];

    return k ^ Zobrist::psq[pc][from] ^ Zobrist::psq[pc][to];
}

// Static exchange evaluation. Simulates the sequence of captures on the
// destination square of m, each side taking with its least valuable attacker,
// and reports whether the side to move comes out at least `threshold` ahead.
bool Position::see_ge(Move m, int threshold) const {
    assert(m.is_ok());

    // Only normal moves are evaluated; castling and promotions are rare and
    // en passant captures are nearly always safe.
    if (m.type() != NORMAL)
        return VALUE_ZERO >= threshold;

    const Square from = m.from();
    const Square to   = m.to();

    int swap = PieceValue[piece_on(to)] - threshold;
    if (swap < 0)
        return false;

    swap = PieceValue[piece_on(from)] - swap;
    if (swap <= 0)
        return true;

    assert(color_of(piece_on(from)) == sideToMove);
    Bitboard occupied  = pieces() ^ from ^ to;
    Color    stm       = sideToMove;
    Bitboard attackers = attackers_to(to, occupied);
    Bitboard stmAttackers, bb;
    int      res = 1;

    while (true) {
        stm = ~stm;
        attackers &= occupied;

        // No more attackers: the side that captured last wins.
        if (!(stmAttackers = attackers & pieces(stm)))
            break;

        // Pinned pieces may not capture while the pinner is still on the board.
        if (pinners(~stm) & occupied) {
            stmAttackers &= ~blockers_for_king(stm);
            if (!stmAttackers)
                break;
        }

        res ^= 1;

        // Capture with the least valuable piece and add any x-ray attackers
        // revealed behind it.
        if ((bb = stmAttackers & pieces(PAWN))) {
            if ((swap = PawnValue - swap) < res)
                break;
            occupied ^= least_significant_square_bb(bb);
            attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
        } else if ((bb = stmAttackers & pieces(KNIGHT))) {
            if ((swap = KnightValue - swap) < res)
                break;
            occupied ^= least_significant_square_bb(bb);
        } else if ((bb = stmAttackers & pieces(BISHOP))) {
            if ((swap = BishopValue - swap) < res)
                break;
            occupied ^= least_significant_square_bb(bb);
            attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
        } else if ((bb = stmAttackers & pieces(ROOK))) {
            if ((swap = RookValue - swap) < res)
                break;
            occupied ^= least_significant_square_bb(bb);
            attackers |= attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN);
        } else if ((bb = stmAttackers & pieces(QUEEN))) {
            if ((swap = QueenValue - swap) < res)
                break;
            occupied ^= least_significant_square_bb(bb);
            attackers |= (attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN))
                       | (attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN));
        } else  // KING: capturing is only possible if the opponent has no attackers left
            return (attackers & ~pieces(stm)) ? res ^ 1 : res;
    }

    return bool(res);
}

// Draw by the fifty-move rule or by repetition. A single repetition inside the
// search tree counts (ply > repetition distance), a repetition of a position
// from before the root needs to occur twice.
bool Position::is_draw(int ply) const {
    if (st().rule50 > 99 && (!checkers() || MoveList<LEGAL>(*this).size()))
        return true;

    return st().repetition && st().repetition < ply;
}

bool Position::has_repeated() const {
    const int end = std::min(st().rule50, st().pliesFromNull);
    for (std::size_t i = states.size() - 1, n = 0; int(n) <= end && i < states.size(); --i, ++n)
        if (states[i].repetition)
            return true;
    return false;
}

Value Position::non_pawn_material(Color c) const {
    return KnightValue * count<KNIGHT>(c) + BishopValue * count<BISHOP>(c) + RookValue * count<ROOK>(c)
         + QueenValue * count<QUEEN>(c);
}

Move Position::uci_to_move(std::string_view str) const {
    std::string s(str);
    if (s.size() == 5)
        s[4] = char(std::tolower(s[4]));

    for (const auto& m : MoveList<LEGAL>(*this))
        if (s == move_to_uci(m))
            return m;

    return Move::none();
}

// Consistency checks, used in debug builds only.
bool Position::pos_is_ok() const {
#ifdef NDEBUG
    return true;
#else
    if (sideToMove != WHITE && sideToMove != BLACK)
        return false;
    if (piece_on(square<KING>(WHITE)) != W_KING || piece_on(square<KING>(BLACK)) != B_KING)
        return false;
    if (st().epSquare != SQ_NONE && relative_rank(sideToMove, st().epSquare) != RANK_6)
        return false;
    if (attackers_to(square<KING>(~sideToMove)) & pieces(sideToMove))
        return false;  // the side that just moved left its king in check
    if (pieces(PAWN) & (Rank1BB | Rank8BB))
        return false;
    if (count<KING>(WHITE) != 1 || count<KING>(BLACK) != 1)
        return false;
    if ((pieces(WHITE) & pieces(BLACK)) || (pieces(WHITE) | pieces(BLACK)) != pieces())
        return false;
    if (popcount(pieces(WHITE)) > 16 || popcount(pieces(BLACK)) > 16)
        return false;

    for (PieceType p1 = PAWN; p1 <= KING; ++p1)
        for (PieceType p2 = PAWN; p2 <= KING; ++p2)
            if (p1 != p2 && (pieces(p1) & pieces(p2)))
                return false;

    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        const Piece pc = piece_on(s);
        if (pc == NO_PIECE) {
            if (pieces() & s)
                return false;
        } else if (!(pieces(color_of(pc), type_of(pc)) & s))
            return false;
    }

    StateInfo check = st();
    Position  copy  = *this;
    copy.set_state();
    if (copy.key() != check.key || copy.pawn_key() != check.pawnKey || copy.checkers() != check.checkersBB)
        return false;

    return true;
#endif
}

}  // namespace sp
