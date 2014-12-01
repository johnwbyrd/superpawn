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

#include "wblegmov.h"

void            white_leg_mov(piece * w, piece * b)
{
    int             i;

    for (i = 0; i <= 15; i++) {
        if (w[i].bm_pos == 0)
            continue;
        switch (w[i].type) {
        case 'r':
            w[i].bm_legmov = rook_legmovs(i, w, b);
            break;
        case 'n':
            w[i].bm_legmov = knight_legmovs(i, w, b);
            break;
        case 'b':
            w[i].bm_legmov = bishop_legmovs(i, w, b);
            break;
        case 'q':
            w[i].bm_legmov = queen_legmovs(i, w, b);
            break;
        case 'k':
            w[i].bm_legmov = king_legmovs(i, w, b);
            break;
        case 'p':
            w[i].bm_legmov = white_pawn_legmovs(i, w, b);
            break;
        }
    }
}

void            black_leg_mov(piece * w, piece * b)
{                               /* ~0.5ms */
    int             i;

    for (i = 0; i <= 15; i++) {
        if (b[i].bm_pos == 0) {
            continue;
        }
        switch (b[i].type) {
        case 'r':
            b[i].bm_legmov = rook_legmovs(i, b, w);
            break;
        case 'n':
            b[i].bm_legmov = knight_legmovs(i, b, w);
            break;
        case 'b':
            b[i].bm_legmov = bishop_legmovs(i, b, w);
            break;
        case 'q':
            b[i].bm_legmov = queen_legmovs(i, b, w);
            break;
        case 'k':
            b[i].bm_legmov = king_legmovs(i, b, w);
            break;
        case 'p':
            b[i].bm_legmov = black_pawn_legmovs(i, w, b);       /* attention ! */
            break;
        }
    }
}

int             white_exists_leg_moves(piece * w, piece * b)
{
    int             i,
                    moves_nonzero;

    moves_nonzero = 0;
    for (i = 0; i <= 15; i++) {
        if (w[i].bm_pos == 0)
            continue;
        switch (w[i].type) {
        case 'r':
            if (exist_rook_legmov(i, w, b))
                return 1;
            break;
        case 'n':
            if (exist_knight_legmov(i, w, b))
                return 1;
            break;
        case 'b':
            if (exist_bishop_legmov(i, w, b))
                return 1;
            break;
        case 'q':
            if (exist_queen_legmov(i, w, b))
                return 1;
            break;
        case 'k':
            if (exist_king_legmov(i, w, b))
                return 1;
            break;
        case 'p':
            if (exist_white_pawn_legmov(i, w, b))
                return 1;
            break;
        }
    }

    return 0;
}

int             black_exists_leg_moves(piece * w, piece * b)
{
    int             i,
                    moves_nonzero;

    moves_nonzero = 0;
    for (i = 0; i <= 15; i++) {
        if (b[i].bm_pos == 0)
            continue;
        switch (b[i].type) {
        case 'r':
            if (exist_rook_legmov(i, b, w))
                return 1;
            break;
        case 'n':
            if (exist_knight_legmov(i, b, w))
                return 1;
            break;
        case 'b':
            if (exist_bishop_legmov(i, b, w))
                return 1;
            break;
        case 'q':
            if (exist_queen_legmov(i, b, w))
                return 1;
            break;
        case 'k':
            if (exist_king_legmov(i, b, w))
                return 1;
            break;
        case 'p':
            if (exist_black_pawn_legmov(i, w, b))
                return 1;
            break;
        }
    }

    return 0;
}

int             push_leg_mov(piece * x, MOVE_LIST * movelist)
{
    int             i,
                    j,
                    k,
                    arrnum;
    char            color;
    BITMASK         bm_work;

    int             startindex = (int) ((16 - 0.0) * rand() / (RAND_MAX + 1.0));

    arrnum = 0;

    color = x[0].color;

    for (i = 0; i <= 15; i++) { /* insted of 0 -> 15, that should be random */
        k = (i + startindex) % 16;
        if (x[k].bm_pos == 0)
            continue;
        for (j = 0; j < 64; j++) {
            bm_work = array[j];
            if ((bm_work & x[k].bm_legmov) != 0) {      /* legal move */

                if ((x[k].type == 'p') && (((bm_work & ROW1) != 0) || ((bm_work & ROW8) != 0))) {       /* promotion */
                    *movelist = ins_head(movelist, k, color, 'q', bm_work, *movelist);
                    *movelist = ins_head(movelist, k, color, 'r', bm_work, *movelist);
                    *movelist = ins_head(movelist, k, color, 'b', bm_work, *movelist);
                    *movelist = ins_head(movelist, k, color, 'n', bm_work, *movelist);
                    arrnum += 4;
                } else {
                    *movelist = ins_head(movelist, k, color, '-', bm_work, *movelist);
                    arrnum++;
                }
            }
        }
    }

    return arrnum;
}
