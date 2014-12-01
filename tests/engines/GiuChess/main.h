// This file is a part of the GiuChess Project.
//
// Copyright (c) 2005 Giuliano Ippoliti aka JSorel (ippo@linuxmail.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef _MAIN_H
#define _MAIN_H

#undef max
#define max(x,y) ((x)>(y)?(x):(y))
#undef min
#define min(x,y) ((x)<(y)?(x):(y))

#define STARTDPTH 5
//#define STARTDPTH 4
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define RET_ERR 0xFFFFFFFFFFFFFFFF

#define BOTTOM  0x00000000000000FF
#define TOP     0xFF00000000000000
#define LEFT    0x0101010101010101
#define RIGHT   0x8080808080808080

#define TOPLEFT 0xFF01010101010101
#define TOPRIGHT 0xFF80808080808080
#define BOTTOMLEFT 0x01010101010101FF
#define BOTTOMRIGHT 0x80808080808080FF

#define BORDER  0xFF818181818181FF
#define PERIF   0x00003C24243C0000
#define PERBOR  0x007E424242427E00
#define CENTRE  0x0000001818000000

#define ROW1	0x00000000000000FF
#define ROW2	0x000000000000FF00
#define ROW3	0x0000000000FF0000
#define ROW4	0x00000000FF000000
#define ROW5	0x000000FF00000000
#define ROW6	0x0000FF0000000000
#define ROW7	0x00FF000000000000
#define ROW8	0xFF00000000000000

#define COL1    0x0101010101010101
#define COL2    0x0202020202020202
#define COL3    0x0404040404040404
#define COL4    0x0808080808080808
#define COL5    0x1010101010101010
#define COL6    0x2020202020202020
#define COL7    0x4040404040404040
#define COL8    0x8080808080808080

#define BITMASK unsigned long long int

#define PAWN_VAL 1.0
#define KNIGHT_VAL 2.9
#define BISHOP_VAL 3.1
#define ROOK_VAL 5.0
#define QUEEN_VAL 9.0
//#define KING_VAL 100.0		//special value: sum of the others !

typedef struct move_leg {
	int order;
	char color;
	char newpiece;
	BITMASK bm_move;
	float evaluation;
	struct move_leg *next;
} MOVE_LEG;

typedef MOVE_LEG* MOVE_LIST;

typedef struct move_ord {
	char move[6];
	float evaluation;
	struct move_ord *next;
} MOVE_ORD;

typedef MOVE_ORD* ORD_LIST;

typedef struct {
	char color;
	char type;
	float value;
	int last_double_move;              //for pawns (en passant...)
	int deja_moved;                    //for castle (rook and king)
	int under_check;                   //for king
	BITMASK bm_pos;
	BITMASK bm_legmov;
	} piece;
	
char last_moved_color;
piece w[16], b[16];   //w et b sont des piece*
BITMASK brow[8], bcol[8], matrix[8][8];
int arr_row[64], arr_col[64];
BITMASK array[64];
int DEBUGG;
int depthmax;

#include "check.h"
#include "iniz.h"
#include "eval_pos.h"
#include "exec_mov.h"
#include "legal.h"
#include "list.h"
#include "pieces_ex_legmoves.h"
#include "pieces_legmoves.h"
#include "rw_thread.h"
#include "util_functs.h"
#include "wblegmov.h"

#endif


