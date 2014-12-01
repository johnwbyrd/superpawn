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

#ifndef _LEGAL_H
#define _LEGAL_H

#include "main.h"

int is_illegal(int, piece*, piece*);
int rook_eats_king(int order, piece*, piece*, int, int);
int bishop_eats_king(int order, piece*, piece*, int, int);
int queen_eats_king(int order, piece*, piece*, int, int);
int knight_eats_king(int order, piece*, piece*, int, int);
int white_pawn_eats_king(int order, piece*, piece*, BITMASK);
int black_pawn_eats_king(int order, piece*, piece*, BITMASK);
int king_eats_king(piece*, piece*, int, int);
int exists_legal_mov(int, piece *, piece *);
int is_legal_mov(int, piece *, piece *, BITMASK);
int is_pawn_legal_mov(int, piece *, piece *, BITMASK);

#endif
