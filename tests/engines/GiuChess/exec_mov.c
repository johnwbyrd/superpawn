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

#include "exec_mov.h"

void            execute_move(int order, piece * friends, piece * enems, BITMASK bm_move, char newpiece)
{
    BITMASK         bm_orig;
    int             busy,
                    new_col,
                    i;

    last_moved_color = friends[order].color;

/*if ((friends[order].type == 'p') || (friends[order].type == 'k')) {
   conv_bm_cases(friends[order].bm_pos, &orig_row, &orig_col);
   conv_bm_cases(bm_move, &new_row, &new_col);
   } */
    bm_orig = friends[order].bm_pos;

    busy = 0;
    for (i = 0; i <= 15; i++) {
        if ((bm_move & enems[i].bm_pos) != 0) { /* an enemy in that square */
            busy = i + 1;       /* it must be > 0 */
            enems[busy - 1].bm_pos = 0;
            enems[4].value -= enems[busy - 1].value;
            rule50 = 0;         /* capture */
            break;
        }
    }

    friends[order].bm_pos = bm_move;

    if (friends[order].type == 'p') {
        rule50 = 0;
        if (newpiece != '-') {
            friends[order].type = newpiece;
            if (newpiece == 'q')
                friends[order].value = QUEEN_VAL;
            else if (newpiece == 'r')
                friends[order].value = ROOK_VAL;
            else if (newpiece == 'b')
                friends[order].value = BISHOP_VAL;
            else if (newpiece == 'n')
                friends[order].value = KNIGHT_VAL;
            else
                llog("ANOMALIE: newpiece = %c\n", newpiece);
            friends[4].value += friends[order].value - 1;       /* -1: a pawn is lost! */
        } else {
            /* double pawn_mov */
            if (((bm_orig & (bm_move << 16)) != 0) || ((bm_orig & (bm_move >> 16)) != 0)) {
                for (i = 8; i <= 15; i++) {     /* we don't care other pieces */
                    if (i == order)
                        friends[order].last_double_move = 1;
                    else
                        friends[i].last_double_move = 0;
                }
            } else {
                for (i = 8; i <= 15; i++)       /* we don't care other pieces */
                    friends[i].last_double_move = 0;

                /* en passant */
                if ((((bm_orig & (bm_move << 8)) == 0) && ((bm_orig & (bm_move >> 8)) == 0)) && (busy == 0)) {
                    get_column_from_bm(bm_move, &new_col);
                    enems[8 + new_col].bm_pos = 0;
                    enems[4].value -= 1;       
                }
            }
        }
    } else {
        if (busy <= 0)
            rule50++;
        for (i = 0; i <= 15; i++) {
            friends[i].last_double_move = 0;
        }
    }

    /* castle                                          */
    if (friends[order].type == 'k') {
        if ((bm_orig & (bm_move >> 2)) != 0)
            friends[7].bm_pos = friends[7].bm_pos >> 2; /* short castle */
        if ((bm_orig & (bm_move << 2)) != 0)
            friends[0].bm_pos = friends[0].bm_pos << 3; /* long castle */
    }
    /* see if there is a check!!!... à la fin */

    if (check(order, friends, enems) == 1)
        enems[4].under_check = 1;
    else
        enems[4].under_check = 0;

    friends[order].deja_moved = 1;

}


void            string_execute_move(char *move, piece * w, piece * b)
{
    int             row,
                    col,
                    newrow,
                    newcol,
                    i;
    char            newpiece;
    BITMASK         bm_old,
                    bm_new;

    newpiece = '-';
    col = move[0] - 97;
    row = move[1] - 49;
    newcol = move[2] - 97;
    newrow = move[3] - 49;

    if (strlen(move) > 4) {
        newpiece = move[4];     /* q, r, k, b */
    }
    conv_cases_bm(&bm_old, row, col);
    conv_cases_bm(&bm_new, newrow, newcol);

    /* attention ! */
    for (i = 0; i <= 15; i++) {
        if (w[i].bm_pos == bm_old) {
            execute_move(i, w, b, bm_new, newpiece);
            return;
        }
        if (b[i].bm_pos == bm_old) {
            execute_move(i, b, w, bm_new, newpiece);
            return;
        }
    }
}

void            list_execute_move(MOVE_LIST movelist)
{
    int             order;
    char            color,
                    newpiece;
    BITMASK         bm_new;

    order = movelist->order;
    color = movelist->color;
    newpiece = movelist->newpiece;
    bm_new = movelist->bm_move;

    if (color == 'W')
        execute_move(order, w, b, bm_new, newpiece);
    else
        execute_move(order, b, w, bm_new, newpiece);
}
