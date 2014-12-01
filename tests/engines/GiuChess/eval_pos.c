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

#include "eval_pos.h"

#define INTERVAL 0.1
/*#define RULE50THR 99 */
#define RULE50THR 49

float           eval_material()
{
    return w[4].value - b[4].value;
}

float           centre(int color)
{
    float           centre = 0.0;
    int             j;

    if (color == 1) {
        for (j = 0; j <= 7; j++) {
            if (j == 4)
                continue;
            if (b[j].bm_pos == 0)
                continue;
            if ((b[j].bm_pos & PERBOR) != 0)
                centre += 0.01;
            else if ((b[j].bm_pos & PERIF) != 0)
                centre += 0.1;
            else if ((b[j].bm_pos & CENTRE) != 0)
                centre += 0.15;
        }
        for (j = 10; j <= 13; j++) {
            if (b[j].bm_pos == 0)
                continue;
            /* if ((b[j].bm_pos & PERBOR) != 0 ) centre += 0.01; */
            else if ((b[j].bm_pos & PERIF) != 0)
                centre += 0.1;
            else if ((b[j].bm_pos & CENTRE) != 0)
                centre += 0.3;
        }
    } else {
        for (j = 0; j <= 7; j++) {
            if (j == 4)
                continue;
            if (w[j].bm_pos == 0)
                continue;
            if ((w[j].bm_pos & PERBOR) != 0)
                centre += 0.01;
            else if ((w[j].bm_pos & PERIF) != 0)
                centre += 0.1;
            else if ((w[j].bm_pos & CENTRE) != 0)
                centre += 0.15;
        }
        for (j = 10; j <= 13; j++) {
            if (w[j].bm_pos == 0)
                continue;
            /* if ((w[j].bm_pos & PERBOR) != 0 ) centre += 0.01; */
            else if ((w[j].bm_pos & PERIF) != 0)
                centre += 0.1;
            else if ((w[j].bm_pos & CENTRE) != 0)
                centre += 0.3;
        }
    }
    return centre;
}


float           evaluate(int color)
{                               /* color which made the last move */
    if (color == 1) {           /* white */
        if (black_exists_leg_moves(w, b) == 0) {
            if (b[4].under_check)
                return -99.0;   /* ATTENTION: - !!!! */
            else
                return 0.0;
        } else if (rule50 >= RULE50THR)
            return 0.0;
        else
            return (b[4].value - w[4].value + centre(1));
    } else {                    /* black */
        if (white_exists_leg_moves(w, b) == 0) {
            if (w[4].under_check)
                return -99.0;
            else
                return 0.0;
        } else if (rule50 >= RULE50THR)
            return 0.0;
        else
            return (w[4].value - b[4].value + centre(-1));
    }
}

float           alphabeta(int depth, float alpha, float beta, int color, char *bestmove)
{
    float           alphaL,
                    val;
    int             nlgmv;
    MOVE_LIST       tmplist,
                    mvlst;

    piece           wbck[16],
                    bbck[16];
    char            thismove[6];
    int             rule50bck;

    if (depth == 0)
        return evaluate(color);
    alphaL = alpha;

    if (color == 1) {           /* white */
        black_leg_mov(w, b);
        crt_list(&mvlst);
        nlgmv = push_leg_mov(b, &mvlst);
        if (nlgmv == 0) {
            if (b[4].under_check) {     /* white's move mated black */
                canc_list(&mvlst);
                return -99.0 - depth;
            } else {
                canc_list(&mvlst);
                return 0.0;     /* stalemate */
            }
        } else {
            /* backup */
            memcpy(wbck, w, sizeof(w));
            memcpy(bbck, b, sizeof(b));
            rule50bck = rule50;

            tmplist = mvlst;
            while (tmplist != NULL) {
                if (depth == depthmax) {
                    conv_list_mov(thismove, tmplist);
                }
                list_execute_move(tmplist);

                val = -alphabeta(depth - 1, -beta, -alphaL, -color, bestmove);

                /* undo backup */
                memcpy(w, wbck, sizeof(w));
                memcpy(b, bbck, sizeof(b));
                rule50 = rule50bck;

                tmplist = tmplist->next;

                if (val >= beta) {
                    canc_list(&mvlst);
                    return beta;
                }
                if (val > alphaL) {
                    alphaL = val;

                    if (depth == depthmax) {
                        strcpy(bestmove, thismove);
                    }
                }
            }
            canc_list(&mvlst);
            return alphaL;
        }
    } else {
        white_leg_mov(w, b);
        crt_list(&mvlst);
        nlgmv = push_leg_mov(w, &mvlst);
        if (nlgmv == 0) {
            if (w[4].under_check) {     /* white's move mated black */
                canc_list(&mvlst);
                return -99.0 - depth;
            } else {
                canc_list(&mvlst);
                return 0.0;     /* stalemate */
            }
        } else {
            /* backup */
            memcpy(wbck, w, sizeof(w));
            memcpy(bbck, b, sizeof(b));
            rule50bck = rule50;

            tmplist = mvlst;
            while (tmplist != NULL) {
                if (depth == depthmax) {
                    conv_list_mov(thismove, tmplist);
                }
                list_execute_move(tmplist);
                val = -alphabeta(depth - 1, -beta, -alphaL, -color, bestmove);

                /* undo backup */
                memcpy(w, wbck, sizeof(w));
                memcpy(b, bbck, sizeof(b));
                rule50 = rule50bck;

                tmplist = tmplist->next;

                if (val >= beta) {
                    canc_list(&mvlst);
                    return beta;
                }
                if (val > alphaL) {
                    alphaL = val;

                    if (depth == depthmax) {
                        strcpy(bestmove, thismove);
                    }
                }
            }
            canc_list(&mvlst);
            return alphaL;
        }
    }
}

void            conv_list_mov(char *move, MOVE_LIST movelist)
{                               /* appelé 20/30 fois */
    int             order;
    char            color,
                    newpiece;
    int             oldrow,
                    oldcol,
                    newrow,
                    newcol;
    BITMASK         bm_old,
                    bm_new;

    order = movelist->order;
    color = movelist->color;
    newpiece = movelist->newpiece;
    bm_new = movelist->bm_move;

    if (color == 'W')
        bm_old = w[order].bm_pos;
    else
        bm_old = b[order].bm_pos;

    conv_bm_cases(bm_old, &oldrow, &oldcol);
    conv_bm_cases(bm_new, &newrow, &newcol);

    move[0] = 97 + oldcol;
    move[1] = 49 + oldrow;
    move[2] = 97 + newcol;
    move[3] = 49 + newrow;

    if (newpiece == '-')
        move[4] = '\0';
    else {
        move[4] = newpiece;
        move[5] = '\0';
    }
}
