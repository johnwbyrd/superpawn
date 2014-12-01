/* This file is a part of the GiuChess Project. */
/* */
/* Copyright (c) 2005 Giuliano Ippoliti aka JSorel (ippo@linuxmail.org) */
/* Some Correction by Dan Corbit */
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

#include "rw_thread.h"
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
void            xlog(char *fmt,...);
#endif
#define ZEITNOT 3500

int             force,
                mtime,
                otim,
                white_to_move,
                first_move_done,
                tid;

void            leggi(char *buf, size_t len)
{
    int             i = 0;
    char            c = ' ';
    int             index;

    while ((c != '\n') && (i < len)) {
        read(0, &c, 1);
        buf[i] = c;
        i++;
    }
    index = max(i - 1, 0);
    index = min(index, len - 1);
    buf[index] = '\0';
}

void            scrivi(char *buf)
{
    write(1, buf, strlen(buf));
}

int             is_move(char *buf)
{
    if (strlen(buf) < 4)
        return 0;
    if ((buf[0] >= 'a') && (buf[0] <= 'h') &&
        (buf[1] >= '1') && (buf[1] <= '8') &&
        (buf[2] >= 'a') && (buf[2] <= 'h') &&
        (buf[3] >= '1') && (buf[3] <= '8'))
        return 1;
    else
        return 0;
}

void            do_move(char *move)
{
    char            real[100];
    sprintf(real, "move %s\n", move);
    scrivi(real);
}

int             cinquanta()
{
    if (rule50 == 99) {
        rule50 = 0;
        return 1;
    }
    return 0;
}

int             insuff_material()
{
    int             i,
                    wnr = 0,
                    wnn = 0,
                    wnb = 0,
                    wnq = 0,
                    wnp = 0,
                    bnr = 0,
                    bnn = 0,
                    bnb = 0,
                    bnq = 0,
                    bnp = 0;
    int             winsuff = 0,
                    binsuff = 0;

    for (i = 0; i <= 15; i++) {
        if (i == 4)
            continue;           /* king */
        if (w[i].bm_pos != 0) {
            switch (w[i].type) {
            case 'r':
                wnr++;
                break;
            case 'n':
                wnn++;
                break;
            case 'b':
                wnb++;
                break;
            case 'q':
                wnq++;
                break;
            case 'p':
                wnp++;
                break;
            }
        }
        if (b[i].bm_pos != 0) {
            switch (b[i].type) {
            case 'r':
                bnr++;
                break;
            case 'n':
                bnn++;
                break;
            case 'b':
                bnb++;
                break;
            case 'q':
                bnq++;
                break;
            case 'p':
                bnp++;
                break;
            }
        }
    }

    if (!(wnr + wnq + wnp)) {   /* white: K+N or K+B or K */
        if ((wnn + wnb) <= 1)
            winsuff = 1;
        else
            winsuff = 0;
    }
    if (!(bnr + bnq + bnp)) {   /* black: K+N or K+B or K */
        if ((bnn + bnb) <= 1)
            binsuff = 1;
        else
            binsuff = 0;
    }
    if ((winsuff) && (binsuff))
        return 1;
    else
        return 0;
}

void            white_move()
{
    int             nlegmov;
    char            bestply2[10];
    float           tmpres;
    MOVE_LIST       movelist;

    if (insuff_material()) {
        scrivi("1/2-1/2 {insufficient material}\n");
        iniz(w, b);
        return;
    }
    if (cinquanta()) {
        scrivi("1/2-1/2 {50 moves rule}\n");
        iniz(w, b);
        return;
    }
    white_leg_mov(w, b);

    crt_list(&movelist);
    nlegmov = push_leg_mov(w, &movelist);
    if (!nlegmov) {
        if (w[4].under_check)
            scrivi("0-1\n");
        else
            scrivi("1/2-1/2 {Stalemate}\n");
        return;
    }
    if (mtime < ZEITNOT)
        depthmax = STARTDPTH - 1;
    else
        depthmax = STARTDPTH;

    tmpres = alphabeta(depthmax, -200, 200, -1, bestply2);

    string_execute_move(bestply2, w, b);
    do_move(bestply2);
    canc_list(&movelist);

}

void            black_move()
{
    int             nlegmov;
    char            bestply2[10];
    float           tmpres;
    MOVE_LIST       movelist;

    if (insuff_material()) {
        scrivi("1/2-1/2 {insufficient material}\n");
        iniz(w, b);
        return;
    }
    if (cinquanta()) {
        scrivi("1/2-1/2 {50 moves rule}\n");
        iniz(w, b);
        return;
    }
    black_leg_mov(w, b);

    crt_list(&movelist);
    nlegmov = push_leg_mov(b, &movelist);

    if (!nlegmov) {
        if (b[4].under_check)
            scrivi("1-0\n");
        else
            scrivi("1/2-1/2 {Stalemate}\n");
        return;
    }
    if (mtime < ZEITNOT)
        depthmax = STARTDPTH - 1;
    else
        depthmax = STARTDPTH;

    tmpres = alphabeta(depthmax, -200, 200, 1, bestply2);  /* 1: white did last move */

    string_execute_move(bestply2, w, b);

    do_move(bestply2);
    canc_list(&movelist);
}

void            wait_for_input()
{
    char            buf[100];

    while (1) {
        leggi(buf, sizeof(buf));
        xlog("%s\n", buf);
        if (!strcmp(buf, "xboard")) {
            scrivi("\n");
        } else if (!strcmp(buf, "protover 2")) {
            scrivi("Chess\n");
            scrivi("feature setboard=1 sigint=0 variants=\"normal\" draw=1 reuse=1 myname=\"GiuChess-1.0beta2\" done=1\n");
        } else if (!strcmp(buf, "new")) {
            first_move_done = 0;
            white_to_move = 1;
            force = 0;          /* not in force mode */
            iniz(w, b);
            cont = 0;
        } else if (!strcmp(buf, "force")) {
            force = 1;
        } else if (!strcmp(buf, "quit"))
            exit(EXIT_SUCCESS);
        /* else if (!strcmp(buf, "draw")) { scrivi("offer draw\n"); } */
        else if (strstr(buf, "time")) {
            sscanf(buf, "%*s %d", &mtime);
            llog("%d - %d\n", cont, mtime);
        } else if (strstr(buf, "otim")) {
            sscanf(buf, "%*s %d", &otim);
        } else if (!strcmp(buf, "white")) {
            white_to_move = 1;
        } else if (!strcmp(buf, "black")) {
            white_to_move = 0;
        } else if (!strcmp(buf, "go")) {
            force = 0;
            cont++;
            if (white_to_move)
                white_move();
            else
                black_move();
            first_move_done = 1;
        } else if (is_move(buf)) {
            string_execute_move(buf, w, b);
            if (!first_move_done) {
                white_to_move = 0;
                first_move_done = 1;
            }
            if (!force) {
                cont++;
                if (white_to_move)
                    white_move();
                else
                    black_move();
            }
        }
    }
}
