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

#include "util_functs.h"

void            llog(char *fmt,...)
{
    FILE           *fp;
    char            buf[101];
    va_list         ap;

    buf[100] = '\0';
    va_start(ap, fmt);
    vsnprintf(buf, 100, fmt, ap);
    va_end(ap);

    fp = fopen("mylog.log", "a");
    if (fp == NULL)
        exit(6);

    fprintf(fp, "%s", buf);
    fclose(fp);
}

void            xlog(char *fmt,...)
{
    FILE           *fp;
    char            buf[101];
    va_list         ap;

    buf[100] = '\0';
    va_start(ap, fmt);
    vsnprintf(buf, 100, fmt, ap);
    va_end(ap);

    fp = fopen("xboard_cmnds.log", "a");
    if (fp == NULL)
        exit(6);

    fprintf(fp, "%s", buf);
    fclose(fp);
}

void            log_bin(BITMASK x)
{
    int             n,
                    i,
                    j;
    char            bitmask[64];
    for (n = 0; n < 64; n++) {
        if ((x & 0x1) != 0) {
            bitmask[n] = '1';
        } else {
            bitmask[n] = '0';
        }
        x = x >> 1;
    }

    for (j = 7; j >= 0; j--) {
        i = 8 * j;
        for (n = i; n <= i + 7; n++)
            llog("%c ", bitmask[n]);
        llog("\n");
    }
    llog("\n");
}

void            init_array()
{
    int             i;
    BITMASK         bm = 0x1;

    for (i = 0; i < 64; i++) {
        if (i != 0)
            bm = bm << 1;
        array[i] = bm;
    }
}

void            init_rc_arr()
{
    int             i;

    for (i = 0; i < 64; i++) {
        arr_row[i] = i / 8;
        arr_col[i] = i % 8;
    }
}

void            init_matrix()
{
    int             i,
                    j;

    brow[0] = ROW1;
    brow[1] = ROW2;
    brow[2] = ROW3;
    brow[3] = ROW4;
    brow[4] = ROW5;
    brow[5] = ROW6;
    brow[6] = ROW7;
    brow[7] = ROW8;

    bcol[0] = COL1;
    bcol[1] = COL2;
    bcol[2] = COL3;
    bcol[3] = COL4;
    bcol[4] = COL5;
    bcol[5] = COL6;
    bcol[6] = COL7;
    bcol[7] = COL8;

    for (i = 0; i <= 7; i++) {
        for (j = 0; j <= 7; j++) {
            matrix[i][j] = brow[i] & bcol[j];
        }
    }
}

void            conv_bm_cases(BITMASK bm, int *row, int *column)
{              /* ricerca dicotimca... better with hash tables */
    /* ROW */
/*chiamata un numero impressionante di volte.................................... */
    if (bm & 0x00000000FFFFFFFF) {      /* row: 1->4 */
        if (bm & 0x000000000000FFFF) {  /* row: 1->2 */
            if (bm & 0x00000000000000FF)
                *row = 0;
            else
                *row = 1;
        } else {                /* row: 3->4 */
            if (bm & 0x0000000000FF0000)
                *row = 2;
            else
                *row = 3;
        }
    } else {                    /* row: 5->8 */
        if (bm & 0x0000FFFF00000000) {
            if (bm & 0x000000FF00000000)
                *row = 4;
            else
                *row = 5;
        } else {
            if (bm & 0x00FF000000000000)
                *row = 6;
            else
                *row = 7;
        }

    }

    /* COLUMN */
    if (bm & 0x0F0F0F0F0F0F0F0F) {      /* col: 1->4 */
        if (bm & 0x0303030303030303) {  /* col: 1->2 */
            if (bm & 0x0101010101010101)
                *column = 0;
            else
                *column = 1;
        } else {                /* col: 3->4 */
            if (bm & 0x0404040404040404)
                *column = 2;
            else
                *column = 3;
        }
    } else {                    /* col: 5->8 */
        if (bm & 0x3030303030303030) {  /* col : 5 -> 6 */
            if (bm & 0x1010101010101010)
                *column = 4;
            else
                *column = 5;
        } else {
            if (bm & 0x4040404040404040)
                *column = 6;
            else
                *column = 7;
        }
    }
}

void            get_column_from_bm(BITMASK bm, int *column)
{
    if (bm & 0x0F0F0F0F0F0F0F0F) {      /* col: 1->4 */
        if (bm & 0x0303030303030303) {  /* col: 1->2 */
            if (bm & 0x0101010101010101)
                *column = 0;
            else
                *column = 1;
        } else {                /* col: 3->4 */
            if (bm & 0x0404040404040404)
                *column = 2;
            else
                *column = 3;
        }
    } else {                    /* col: 5->8 */
        if (bm & 0x3030303030303030) {  /* col : 5 -> 6 */
            if (bm & 0x1010101010101010)
                *column = 4;
            else
                *column = 5;
        } else {
            if (bm & 0x4040404040404040)
                *column = 6;
            else
                *column = 7;
        }
    }
}


void            conv_cases_bm(BITMASK * bm, int row, int column)
{                               /* chiamata quasi mai */
    *bm = matrix[row][column];
}

void            trace_time(char *text)
{
    struct timeval  tv;

    gettimeofday(&tv, NULL);
    llog("%s: %d - %d\n", text, tv.tv_sec, tv.tv_usec);
}

#ifndef WIN32
unsigned long   getesp()
{
    __asm__("movl %esp, %eax");
}
#endif
