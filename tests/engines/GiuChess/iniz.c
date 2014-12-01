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

#include "iniz.h"

void            iniz(piece * w, piece * b)
{                               /* tableaux passés par référence */
    int             i;

    rule50 = 0;
    depthmax = STARTDPTH;

    w[0].type = 'r';
    w[0].value = ROOK_VAL;
    w[0].bm_pos = 1;
    w[1].type = 'n';
    w[1].value = KNIGHT_VAL;
    w[1].bm_pos = 1 << 1;
    w[2].type = 'b';
    w[2].value = BISHOP_VAL;
    w[2].bm_pos = 1 << 2;
    w[3].type = 'q';
    w[3].value = QUEEN_VAL;
    w[3].bm_pos = 1 << 3;
    w[4].type = 'k';
    w[4].value = 2 * ROOK_VAL + 2 * KNIGHT_VAL + 2 * BISHOP_VAL + QUEEN_VAL + 8 * PAWN_VAL;
    w[4].bm_pos = 1 << 4;
    w[5].type = 'b';
    w[5].value = BISHOP_VAL;
    w[5].bm_pos = 1 << 5;
    w[6].type = 'n';
    w[6].value = KNIGHT_VAL;
    w[6].bm_pos = 1 << 6;
    w[7].type = 'r';
    w[7].value = ROOK_VAL;
    w[7].bm_pos = 1 << 7;
    for (i = 8; i <= 15; i++) {
        w[i].type = 'p';
        w[i].value = PAWN_VAL;
        w[i].bm_pos = 1 << i;
    }

    b[0].type = 'r';
    b[0].value = ROOK_VAL;
    b[0].bm_pos = pow(2, 56);
    b[1].type = 'n';
    b[1].value = KNIGHT_VAL;
    b[1].bm_pos = pow(2, 57);
    b[2].type = 'b';
    b[2].value = BISHOP_VAL;
    b[2].bm_pos = pow(2, 58);
    b[3].type = 'q';
    b[3].value = QUEEN_VAL;
    b[3].bm_pos = pow(2, 59);
    b[4].type = 'k';
    b[4].value = 2 * ROOK_VAL + 2 * KNIGHT_VAL + 2 * BISHOP_VAL + QUEEN_VAL + 8 * PAWN_VAL;
    b[4].bm_pos = pow(2, 60);
    b[5].type = 'b';
    b[5].value = BISHOP_VAL;
    b[5].bm_pos = pow(2, 61);
    b[6].type = 'n';
    b[6].value = KNIGHT_VAL;
    b[6].bm_pos = pow(2, 62);
    b[7].type = 'r';
    b[7].value = ROOK_VAL;
    b[7].bm_pos = pow(2, 63);

    for (i = 8; i <= 15; i++) {
        b[i].type = 'p';
        b[i].value = PAWN_VAL;
        b[i].bm_pos = pow(2, 40 + i);
    }

    for (i = 0; i <= 15; i++) {
        w[i].color = 'W';
        w[i].bm_legmov = 0;
        w[i].last_double_move = 0;
        w[i].deja_moved = 0;
        w[i].under_check = 0;

        b[i].color = 'B';
        b[i].bm_legmov = 0;
        b[i].last_double_move = 0;
        b[i].deja_moved = 0;
        b[i].under_check = 0;
    }
}
