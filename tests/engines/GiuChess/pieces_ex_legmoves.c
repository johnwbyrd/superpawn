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

#include "pieces_ex_legmoves.h"

int             exist_rook_legmov(int order, piece * friends, piece * enem)
{
    int             i;
    BITMASK         bm_pos,
                    bm_work,
                    focc,
                    eocc;       /* bm_result,  */

    focc = eocc = 0;            /* bmasks for busy squares */
    for (i = 0; i <= 15; i++) {
        eocc = eocc | enem[i].bm_pos;
        focc = focc | friends[i].bm_pos;
    }


    bm_pos = friends[order].bm_pos;
    focc = focc & (bm_pos ^ RET_ERR);

    if ((bm_pos & LEFT) == 0) { /* left */
        bm_work = bm_pos;

        while (1) {
            bm_work = bm_work >> 1;
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & LEFT) != 0)
                break;
        }
    }
    if ((bm_pos & RIGHT) == 0) {/* right */
        bm_work = bm_pos;

        while (1) {
            bm_work = bm_work << 1;
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & RIGHT) != 0)
                break;
        }
    }
    if ((bm_pos & BOTTOM) == 0) {       /* bottom */
        bm_work = bm_pos;

        while (1) {
            bm_work = bm_work >> 8;
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & BOTTOM) != 0)
                break;
        }
    }
    if ((bm_pos & TOP) == 0) {  /* top */
        bm_work = bm_pos;

        while (1) {
            bm_work = bm_work << 8;
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & TOP) != 0)
                break;
        }
    }
    return 0;
}

int             exist_bishop_legmov(int order, piece * friends, piece * enem)
{
    int             i;
    BITMASK         bm_pos,
                    bm_work,
                    focc,
                    eocc;

    focc = eocc = 0;            /* bmasks for busy squares */
    for (i = 0; i <= 15; i++) {
        eocc = eocc | enem[i].bm_pos;
        focc = focc | friends[i].bm_pos;
    }

    bm_pos = friends[order].bm_pos;
    focc = focc & (bm_pos ^ RET_ERR);

    if ((bm_pos & TOPLEFT) == 0) {
        bm_work = bm_pos;
        while (1) {
            bm_work = bm_work << 7;     /* top left */
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                else
                    break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & TOPLEFT) != 0)       /* border */
                break;
        }
    }
    if ((bm_pos & TOPRIGHT) == 0) {
        bm_work = bm_pos;
        while (1) {
            bm_work = bm_work << 9;     /* top right */
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                else
                    break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & TOPRIGHT) != 0)      /* border */
                break;
        }
    }
    if ((bm_pos & BOTTOMRIGHT) == 0) {
        bm_work = bm_pos;
        while (1) {
            bm_work = bm_work >> 7;     /* bottom right */
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                else
                    break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & BOTTOMRIGHT) != 0)   /* border */
                break;
        }
    }
    if ((bm_pos & BOTTOMLEFT) == 0) {
        bm_work = bm_pos;
        while (1) {
            bm_work = bm_work >> 9;     /* bottom left */
            if ((bm_work & focc) != 0)
                break;
            if ((bm_work & eocc) != 0) {
                if (is_legal_mov(order, friends, enem, bm_work))
                    return 1;
                else
                    break;
            }
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
            if ((bm_work & BOTTOMLEFT) != 0)    /* border */
                break;
        }
    }
    return 0;
}

int             exist_queen_legmov(int order, piece * friends, piece * enem)
{
    if (exist_rook_legmov(order, friends, enem))
        return 1;
    if (exist_bishop_legmov(order, friends, enem))
        return 1;

    return 0;
}

int             exist_knight_legmov(int order, piece * friends, piece * enem)
{
    int             row,
                    col,
                    i;
    BITMASK         bm_pos,
                    bm_work,
                    focc,
                    eocc;

    focc = eocc = 0;            /* bmasks for busy squares */
    for (i = 0; i <= 15; i++) {
        eocc = eocc | enem[i].bm_pos;
        focc = focc | friends[i].bm_pos;
    }

    bm_pos = friends[order].bm_pos;
    focc = focc & (bm_pos ^ RET_ERR);
    conv_bm_cases(bm_pos, &row, &col);

    if (((row + 2) <= 7) && ((col + 1) <= 7)) {
        bm_work = bm_pos << 17;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    if (((row + 1) <= 7) && ((col + 2) <= 7)) {
        bm_work = bm_pos << 10;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    if (((row - 1) >= 0) && ((col + 2) <= 7)) {
        bm_work = bm_pos >> 6;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    if (((row - 2) >= 0) && ((col + 1) <= 7)) {
        bm_work = bm_pos >> 15;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    if (((row - 2) >= 0) && ((col - 1) >= 0)) {
        bm_work = bm_pos >> 17;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    if (((row - 1) >= 0) && ((col - 2) >= 0)) {
        bm_work = bm_pos >> 10;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    if (((row + 1) <= 7) && ((col - 2) >= 0)) {
        bm_work = bm_pos << 6;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    if (((row + 2) <= 7) && ((col - 1) >= 0)) {
        bm_work = bm_pos << 15;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enem, bm_work))
                return 1;
        }
    }
    return 0;
}

int             exist_white_pawn_legmov(int order, piece * w, piece * b)
{
    int             col,
                    i;
    BITMASK         bm_pos,
                    bm_work,
                    focc,
                    eocc;

    focc = eocc = 0;            /* bmasks for busy squares */
    for (i = 0; i <= 15; i++) {
        eocc = eocc | b[i].bm_pos;
        focc = focc | w[i].bm_pos;
    }

    bm_pos = w[order].bm_pos;
    focc = focc & (bm_pos ^ RET_ERR);

    bm_work = bm_pos << 8;
    if (((bm_work & focc) == 0) && ((bm_work & eocc) == 0)) {   /* free square */
        if (is_pawn_legal_mov(order, w, b, bm_work))
            return 1;
        if ((bm_pos & ROW2) != 0) {     /* never moved */
            bm_work = bm_pos << 16;
            if (((bm_work & focc) == 0) && ((bm_work & eocc) == 0))
                if (is_pawn_legal_mov(order, w, b, bm_work))
                    return 1;
        }
    }
    if ((bm_pos & COL1) == 0) { /* pas sur LEFT */
        bm_work = bm_pos << 7;
        if ((bm_work & eocc) != 0)
            if (is_pawn_legal_mov(order, w, b, bm_work))
                return 1;
    }
    if ((bm_pos & COL8) == 0) { /* pas sur RIGHT */
        bm_work = bm_pos << 9;
        if ((bm_work & eocc) != 0)
            if (is_pawn_legal_mov(order, w, b, bm_work))
                return 1;
    }
    /* en passant */
    if ((bm_pos & ROW5) != 0) {
        get_column_from_bm(bm_pos, &col);
        if (col != 7) {
            if (b[8 + col + 1].last_double_move == 1) {
                bm_work = bm_pos << 9;
                if (is_pawn_legal_mov(order, w, b, bm_work))
                    return 1;
            }
        }
        if (col != 0) {
            if (b[8 + col - 1].last_double_move == 1) {
                bm_work = bm_pos << 7;
                if (is_pawn_legal_mov(order, w, b, bm_work))
                    return 1;
            }
        }
    }
    return 0;
}


int             exist_black_pawn_legmov(int order, piece * w, piece * b)
{
    int             col,
                    i;
    BITMASK         bm_pos,
                    bm_work,
                    focc,
                    eocc;

    focc = eocc = 0;            /* bmasks for busy squares */
    for (i = 0; i <= 15; i++) {
        eocc = eocc | w[i].bm_pos;
        focc = focc | b[i].bm_pos;
    }

    bm_pos = b[order].bm_pos;
    focc = focc & (bm_pos ^ RET_ERR);

    bm_work = bm_pos >> 8;
    if (((bm_work & focc) == 0) && ((bm_work & eocc) == 0)) {   /* free square */
        if (is_pawn_legal_mov(order, b, w, bm_work))
            return 1;
        if ((bm_pos & ROW7) != 0) {     /* never moved */
            bm_work = bm_pos >> 16;
            if (((bm_work & focc) == 0) && ((bm_work & eocc) == 0))
                if (is_pawn_legal_mov(order, b, w, bm_work))
                    return 1;
        }
    }
    if ((bm_pos & COL1) == 0) { /* pas sur LEFT */
        bm_work = bm_pos >> 9;
        if ((bm_work & eocc) != 0)
            if (is_pawn_legal_mov(order, b, w, bm_work))
                return 1;
    }
    if ((bm_pos & COL8) == 0) { /* pas sur RIGHT */
        bm_work = bm_pos >> 7;
        if ((bm_work & eocc) != 0)
            if (is_pawn_legal_mov(order, b, w, bm_work))
                return 1;
    }
    /* en passant */
    if ((bm_pos & ROW4) != 0) {
        get_column_from_bm(bm_pos, &col);
        if (col != 7) {
            if (w[8 + col + 1].last_double_move == 1) {
                bm_work = bm_pos >> 7;
                if (is_pawn_legal_mov(order, b, w, bm_work))
                    return 1;
            }
        }
        if (col != 0) {
            if (w[8 + col - 1].last_double_move == 1) {
                bm_work = bm_pos >> 9;
                if (is_pawn_legal_mov(order, b, w, bm_work))
                    return 1;
            }
        }
    }
    return 0;
}


int             exist_king_legmov(int order, piece * friends, piece * enems)
{
    int             i;
    BITMASK         bm_pos,
                    bm_work,
                    focc,
                    eocc;

    focc = eocc = 0;
    for (i = 0; i <= 15; i++) {
        eocc = eocc | enems[i].bm_pos;
        focc = focc | friends[i].bm_pos;
    }

    bm_pos = friends[order].bm_pos;
    focc = focc & (bm_pos ^ RET_ERR);

    if ((bm_pos & ROW1) == 0) {
        bm_work = bm_pos >> 8;
        if ((bm_work & focc) == 0) {    /* no friend in that case */
            if (is_legal_mov(order, friends, enems, bm_work))
                return 1;
        }
        if ((bm_pos & COL1) == 0) {
            bm_work = bm_pos >> 9;
            if ((bm_work & focc) == 0) {
                if (is_legal_mov(order, friends, enems, bm_work))
                    return 1;
            }
        }
    }
    if ((bm_pos & COL1) == 0) {
        bm_work = bm_pos >> 1;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enems, bm_work))
                return 1;
        }
        if ((bm_pos & ROW8) == 0) {
            bm_work = bm_pos << 7;
            if ((bm_work & focc) == 0) {
                if (is_legal_mov(order, friends, enems, bm_work))
                    return 1;
            }
        }
    }
    if ((bm_pos & ROW8) == 0) {
        bm_work = bm_pos << 8;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enems, bm_work))
                return 1;
        }
        if ((bm_pos & COL8) == 0) {
            bm_work = bm_pos << 9;
            if ((bm_work & focc) == 0) {
                if (is_legal_mov(order, friends, enems, bm_work))
                    return 1;
            }
        }
    }
    if ((bm_pos & COL8) == 0) {
        bm_work = bm_pos << 1;
        if ((bm_work & focc) == 0) {
            if (is_legal_mov(order, friends, enems, bm_work))
                return 1;
        }
        if ((bm_pos & ROW1) == 0) {
            bm_work = bm_pos >> 7;
            if ((bm_work & focc) == 0) {
                if (is_legal_mov(order, friends, enems, bm_work))
                    return 1;
            }
        }
    }
    /* castle code: useless here */

    return 0;
}
