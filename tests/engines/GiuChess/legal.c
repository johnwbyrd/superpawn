/* This file is a part of the GiuChess Project. */
/* */
/* Copyright (c) 2005 Giuliano Ippoliti aka JSorel (ippo@linuxmail.org) */
/* */
/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License */
/* as published by the Free Software Foundation; either */
/* version 2 of the License, or (at your option) any later version. */
/* */
/* This program is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */
/* */
/* You should have received a copy of the GNU General Public License */
/* along with this program; if not, write to the Free Software */
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "legal.h"

int             rook_eats_king(int order, piece * friends, piece * enem, int en_k_row, int en_k_col)
{
    BITMASK         bm_pos;
    BITMASK         bm_work,
                    focc,
                    eocc,
                    ekocc;
    int             i,
                    row,
                    col;

    bm_pos = friends[order].bm_pos;
    conv_bm_cases(bm_pos, &row, &col);

    if (en_k_row == row) {      /* same row for rook and king */
        focc = eocc = ekocc = 0;/* bmasks for busy squares */
        for (i = 0; i <= 15; i++) {
            eocc = eocc | enem[i].bm_pos;
            if (enem[i].type == 'k')
                ekocc = enem[i].bm_pos;
            if (i != order)
                focc = focc | friends[i].bm_pos;
        }

        bm_work = bm_pos;

        if (col > en_k_col) {   /* left */
            for (i = col; i > 0; i--) {
                bm_work = bm_work >> 1;
                if ((bm_work & focc) != 0)
                    return 0;   /* friend between rook and king */
                if ((bm_work & ekocc) != 0)
                    return 1;   /* illegality! */
                if ((bm_work & eocc) != 0)
                    return 0;
            }
        } else {                /* right */
            for (i = col; i < 7; i++) {
                bm_work = bm_work << 1;
                if ((bm_work & focc) != 0)
                    return 0;   /* friend between rook and king */
                if ((bm_work & ekocc) != 0)
                    return 1;   /* illegality! */
                if ((bm_work & eocc) != 0)
                    return 0;
            }
        }
    } else if (en_k_col == col) {
        focc = eocc = ekocc = 0;
        for (i = 0; i <= 15; i++) {
            eocc = eocc | enem[i].bm_pos;
            if (enem[i].type == 'k')
                ekocc = enem[i].bm_pos;
            if (i != order)
                focc = focc | friends[i].bm_pos;
        }

        bm_work = bm_pos;

        if (row > en_k_row) {   /* bottom */
            for (i = row; i > 0; i--) {
                bm_work = bm_work >> 8;
                if ((bm_work & focc) != 0)
                    return 0;
                if ((bm_work & ekocc) != 0)
                    return 1;
                if ((bm_work & eocc) != 0)
                    return 0;
            }
        } else {                /* top */
            for (i = row; i < 7; i++) {
                bm_work = bm_work << 8;
                if ((bm_work & focc) != 0)
                    return 0;
                if ((bm_work & ekocc) != 0)
                    return 1;
                if ((bm_work & eocc) != 0)
                    return 0;
            }
        }
    }
    return 0;
}

int             bishop_eats_king(int order, piece * friends, piece * enem, int en_k_row, int en_k_col)
{
    BITMASK         bm_pos;
    BITMASK         bm_work,
                    focc,
                    eocc,
                    ekocc;
    int             i,
                    row,
                    col,
                    delta_r,
                    delta_c;

    bm_pos = friends[order].bm_pos;
    conv_bm_cases(bm_pos, &row, &col);

    delta_r = row - en_k_row;
    delta_c = col - en_k_col;

    if ((delta_r == delta_c) || (delta_r == -delta_c)) {        /* same diagonal */
        focc = eocc = ekocc = 0;
        for (i = 0; i <= 15; i++) {
            eocc = eocc | enem[i].bm_pos;
            if (enem[i].type == 'k')
                ekocc = enem[i].bm_pos;
            if (i != order)
                focc = focc | friends[i].bm_pos;
        }

        bm_work = bm_pos;

        if ((delta_r > 0) && (delta_c > 0)) {   /* bottom left */
            while (1) {         /* we are sure to find at least the enemy king */
                bm_work = bm_work >> 9;
                if ((bm_work & focc) != 0)
                    return 0;
                if ((bm_work & ekocc) != 0)
                    return 1;

                if ((bm_work & eocc) != 0) {
                    return 0;
                }
            }
        } else if ((delta_r > 0) && (delta_c < 0)) {    /* bottom right */
            while (1) {
                bm_work = bm_work >> 7;
                if ((bm_work & focc) != 0)
                    return 0;
                if ((bm_work & ekocc) != 0)
                    return 1;
                if ((bm_work & eocc) != 0) {
                    return 0;
                }
            }
        } else if ((delta_r < 0) && (delta_c > 0)) {    /* top left */
            while (1) {
                bm_work = bm_work << 7;
                if ((bm_work & focc) != 0)
                    return 0;
                if ((bm_work & ekocc) != 0)
                    return 1;
                if ((bm_work & eocc) != 0) {
                    return 0;
                }
            }
        } else {                /* top right */
            while (1) {
                bm_work = bm_work << 9;
                if ((bm_work & focc) != 0)
                    return 0;
                if ((bm_work & ekocc) != 0)
                    return 1;
                if ((bm_work & eocc) != 0) {
                    return 0;
                }
            }
        }
    } else
        return 0;
}

int             queen_eats_king(int order, piece * friends, piece * enem, int en_k_row, int en_k_col)
{
    if (rook_eats_king(order, friends, enem, en_k_row, en_k_col) == 1)  /* move as a tower */
        return 1;
    else if (bishop_eats_king(order, friends, enem, en_k_row, en_k_col) == 1)   /* move as a bishop */
        return 1;
    else
        return 0;
}

int             knight_eats_king(int order, piece * friends, piece * enem, int en_k_row, int en_k_col)
{
    BITMASK         bm_pos;
    int             row,
                    col,
                    delta_r,
                    delta_c;

    bm_pos = friends[order].bm_pos;
    conv_bm_cases(bm_pos, &row, &col);

    delta_r = row - en_k_row;
    delta_c = col - en_k_col;

    if ((delta_r > 2) || (delta_r < -2))
        return 0;
    else if ((delta_c > 2) || (delta_c < -2))
        return 0;
    else if ((((delta_r == 2) || (delta_r == -2)) && ((delta_c == 1) || (delta_c == -1))) ||
             (((delta_r == 1) || (delta_r == -1)) && ((delta_c == 2) || (delta_c == -2))))
        return 1;
    else
        return 0;
}

int             white_pawn_eats_king(int order, piece * friends, piece * enem, BITMASK king_pos)
{
    BITMASK         bm_pos;

    bm_pos = friends[order].bm_pos;

    if ((bm_pos & COL1) == 0) {
        if (((bm_pos << 7) & king_pos) != 0)
            return 1;
    }
    if ((bm_pos & COL8) == 0) {
        if (((bm_pos << 9) & king_pos) != 0)
            return 1;
    }
    return 0;
}

int             black_pawn_eats_king(int order, piece * friends, piece * enem, BITMASK king_pos)
{
    BITMASK         bm_pos;

    bm_pos = friends[order].bm_pos;

    if ((bm_pos & COL1) == 0) {
        if (((bm_pos >> 9) & king_pos) != 0)
            return 1;
    }
    if ((bm_pos & COL8) == 0) {
        if (((bm_pos >> 7) & king_pos) != 0)
            return 1;
    }
    return 0;
}

int             king_eats_king(piece * friends, piece * enem, int en_k_row, int en_k_col)
{
    BITMASK         bm_pos;
    int             row,
                    col,
                    delta_r,
                    delta_c;

    bm_pos = friends[4].bm_pos;
    conv_bm_cases(bm_pos, &row, &col);

    delta_r = row - en_k_row;
    delta_c = col - en_k_col;

    if (((delta_r >= -1) && (delta_r <= 1)) && ((delta_c >= -1) && (delta_c <= 1)))
        return 1;
    return 0;
}

int             is_illegal(int order, piece * friends, piece * enem)
{                               /* looks for an enemy piece eating king */
    char            en_type,
                    fr_type;
    char            enem_color;
    int             j;
    BITMASK         king_pos;
    int             k_row,
                    k_col;

    enem_color = enem[0].color;

    king_pos = friends[4].bm_pos;
    conv_bm_cases(king_pos, &k_row, &k_col);

    fr_type = friends[order].type;      /* type of moving piece */

    for (j = 0; j <= 15; j++) {
        if (enem[j].bm_pos == 0)/* a dead piece can do nothing */
            continue;
        en_type = enem[j].type;
        if (en_type == 'r') {
            if (rook_eats_king(j, enem, friends, k_row, k_col) == 1) {  /* a rook could always eat the king */
                return 1;
            }
        } else if (en_type == 'b') {
            if (bishop_eats_king(j, enem, friends, k_row, k_col) == 1)
                return 1;
        } else if (en_type == 'q') {
            if (queen_eats_king(j, enem, friends, k_row, k_col) == 1)
                return 1;
        } else if (en_type == 'n') {
            if ((fr_type != 'k') && (friends[4].under_check == 0))
                continue;
            else {
                if (knight_eats_king(j, enem, friends, k_row, k_col) == 1)
                    return 1;
            }
        } else if (en_type == 'k') {
            if (fr_type != 'k')
                continue;
            else {
                if (king_eats_king(enem, friends, k_row, k_col) == 1)
                    return 1;
            }
        } else {                /* en_type == 'p' */
            if ((fr_type != 'k') && (friends[4].under_check == 0))
                continue;

            if (enem_color == 'W') {
                if (white_pawn_eats_king(j, enem, friends, king_pos) == 1)
                    return 1;
            } else {
                if (black_pawn_eats_king(j, enem, friends, king_pos) == 1)
                    return 1;
            }
        }
    }

    return 0;
}


int             exists_legal_mov(int order, piece * friends, piece * enem)
{                               /* we test legality for friends[order] */
    int             i,
                    j,
                    eaten,
                    legal;
    int             new_col;    /* for en passant */
    BITMASK         bm_save_pos,
                    bm_eaten_save_pos,
                    bm_legmoves,
                    bm_result,
                    bm_work;

    bm_result = 0;              /* ok */
    bm_save_pos = friends[order].bm_pos;        /* original position */
    bm_legmoves = friends[order].bm_legmov;     /* all candidate legal moves */

    for (i = 0; i < 64; i++) {  /* scorre tutte le mosse... maybe there's a better way */
        bm_work = array[i];
        if ((bm_work & bm_legmoves) != 0) {     /* bm_work: candidate legal move */
            friends[order].bm_pos = bm_work;    /* change piece's position */

            eaten = -1;         /* no eaten pieces right now */
            legal = 1;          /* presunzione di legalità */

            for (j = 0; j <= 15; j++) { /* change eaten piece position */
                if (enem[j].bm_pos == bm_work) {  /* an enemy piece is in the candidate case */
                    eaten = j;
                    bm_eaten_save_pos = bm_work;
                    enem[eaten].bm_pos = 0;
                    break;      /* don't forget about this!  */
                }
            }

            if (friends[order].type == 'p') {   /* en passant */
                if (friends[order].color == 'W') {
                    if ((bm_save_pos & ROW5) != 0) {
                        if ((((bm_work & (bm_save_pos << 7)) != 0) || ((bm_work & (bm_save_pos << 9)) != 0)) &&
                            (eaten == -1)) {    /* diagonal move, not eaten ! */
                            get_column_from_bm(bm_work, &new_col);
                            eaten = 8 + new_col;
                            bm_eaten_save_pos = enem[eaten].bm_pos;
                            enem[eaten].bm_pos = 0;
                        }
                    }
                } else {
                    if ((bm_save_pos & ROW4) != 0) {
/* meglio >>8 == 0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
                        if ((((bm_work & (bm_save_pos >> 7)) != 0) || ((bm_work & (bm_save_pos >> 9)) != 0)) &&
                            (eaten == -1)) {
                            get_column_from_bm(bm_work, &new_col);
                            eaten = 8 + new_col;
                            bm_eaten_save_pos = enem[eaten].bm_pos;
                            enem[eaten].bm_pos = 0;
                        }
                    }
                }
            }
            if (is_illegal(order, friends, enem) == 1)
                legal = 0;

            friends[order].bm_pos = bm_save_pos;
            if (eaten != -1)
                enem[eaten].bm_pos = bm_eaten_save_pos;

            if (legal == 1)
                return 1;       /* there is a legal move! */

        }
    }

    /* not test for castle: if the king cannot move of 1 step, it cannot castle! */

    return 0;
}

int             is_legal_mov(int order, piece * friends, piece * enem, BITMASK bm_mov)
{                               /* we test legality for friends[order] */
    int             j,
                    eaten,
                    legal;
    BITMASK         bm_save_pos,
                    bm_eaten_save_pos;

    bm_save_pos = friends[order].bm_pos;        /* original position */
    friends[order].bm_pos = bm_mov;     /* change piece's position */

    eaten = -1;                 /* no eaten pieces right now */
    legal = 1;                  /* presunzione di legalità */

    for (j = 0; j <= 15; j++) { /* change eaten piece position */
        if (enem[j].bm_pos == bm_mov) { /* an enemy piece is in the candidate case */
            eaten = j;
            bm_eaten_save_pos = bm_mov;
            enem[eaten].bm_pos = 0;
            break;              /* don't forget about this!  */
        }
    }

    if (is_illegal(order, friends, enem) == 1)
        legal = 0;

    friends[order].bm_pos = bm_save_pos;

    if (eaten != -1)
        enem[eaten].bm_pos = bm_eaten_save_pos;

    if (legal == 1)
        return 1;               /* there is a legal move! */

    /* not test for castle: if the king cannot move of 1 step, it cannot
     * castle! */

    return 0;
}

int             is_pawn_legal_mov(int order, piece * friends, piece * enem, BITMASK bm_mov)
{                               /* we test legality for friends[order] which makes the bm_mov move */
    int             j,
                    eaten,
                    legal;
    int             new_row,
                    new_col;    /* for en passant */
    BITMASK         bm_save_pos,
                    bm_eaten_save_pos;

    bm_save_pos = friends[order].bm_pos;        /* original position */

    friends[order].bm_pos = bm_mov;     /* change piece's position */

    eaten = -1;                 /* no eaten pieces right now */
    legal = 1;                  /* presunzione di legalità */

    for (j = 0; j <= 15; j++) { /* change eaten piece position */
        if (enem[j].bm_pos == bm_mov) { /* an enemy piece is in the candidate case */
            eaten = j;
            bm_eaten_save_pos = bm_mov;
            enem[eaten].bm_pos = 0;
            break;              /* don't forget about this!  */
        }
    }

    /* pour en passant ! */
    if (friends[order].color == 'W') {
        if ((bm_save_pos & ROW5) != 0) {
            if ((((bm_mov & (bm_save_pos << 7)) != 0) || ((bm_mov & (bm_save_pos << 9)) != 0)) &&
                (eaten == -1)) {/* diagonal move, not eaten ! */
                conv_bm_cases(bm_mov, &new_row, &new_col);
                eaten = 8 + new_col;
                bm_eaten_save_pos = enem[eaten].bm_pos;
                enem[eaten].bm_pos = 0;
            }
        }
    } else {
        if ((bm_save_pos & ROW4) != 0) {
            if ((((bm_mov & (bm_save_pos >> 7)) != 0) || ((bm_mov & (bm_save_pos >> 9)) != 0)) &&
                (eaten == -1)) {
                conv_bm_cases(bm_mov, &new_row, &new_col);
                eaten = 8 + new_col;
                bm_eaten_save_pos = enem[eaten].bm_pos;
                enem[eaten].bm_pos = 0;
            }
        }
    }

    if (is_illegal(order, friends, enem) == 1)
        legal = 0;

    friends[order].bm_pos = bm_save_pos;
    if (eaten != -1)
        enem[eaten].bm_pos = bm_eaten_save_pos;

    if (legal == 1)
        return 1;               /* there is a legal move! */

    /* not test for castle: if the king cannot move of 1 step, it cannot
     * castle! */

    return 0;
}
