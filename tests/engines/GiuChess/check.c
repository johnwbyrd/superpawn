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

#include "check.h"

int             check(int moved_order, piece * friends, piece * enem)
{
    char            fr_color,
                    fr_type;
    int             j;
    BITMASK         king_pos;
    int             k_row,
                    k_col;

    king_pos = enem[4].bm_pos;
    conv_bm_cases(king_pos, &k_row, &k_col);

    fr_color = friends[0].color;

    for (j = 0; j <= 15; j++) { /* examine all pieces */
        if (friends[j].bm_pos == 0)     /* a dead piece can do nothing */
            continue;
        fr_type = friends[j].type;

        /* r, b, q always to test (can give a discovery check) */
        if (fr_type == 'r') {
            if (rook_eats_king(j, friends, enem, k_row, k_col) == 1) {  /* a rook could always
                                                                         * eat the king */
                return 1;
            }
        } else if (fr_type == 'b') {
            if (bishop_eats_king(j, friends, enem, k_row, k_col) == 1)
                return 1;
        } else if (fr_type == 'q') {
            if (queen_eats_king(j, friends, enem, k_row, k_col) == 1)
                return 1;
        } else if (fr_type == 'n') {
            if (j != moved_order)       /* if (moved_type != 'n') */
                continue;
            if (knight_eats_king(j, friends, enem, k_row, k_col) == 1)
                return 1;
        } else if (fr_type == 'p') {
            if (j != moved_order)       /* not the moved pawn */
                continue;
            if (fr_color == 'W') {
                if (white_pawn_eats_king(j, friends, enem, king_pos) == 1)
                    return 1;
            } else {
                if (black_pawn_eats_king(j, friends, enem, king_pos) == 1)
                    return 1;
            }
        } else                  /* king cannot give check directly */
            continue;
    }

    return 0;
}
