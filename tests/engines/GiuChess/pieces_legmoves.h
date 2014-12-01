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

#ifndef _PIECES_LEGMOVES_H
#define _PIECES_LEGMOVES_H

#include "main.h" 

BITMASK rook_legmovs(int, piece *, piece *);
BITMASK bishop_legmovs(int, piece *, piece *);
BITMASK queen_legmovs(int, piece *, piece *);
BITMASK knight_legmovs(int, piece *, piece *);
BITMASK white_pawn_legmovs(int, piece *, piece *);
BITMASK black_pawn_legmovs(int, piece *, piece *);
BITMASK king_legmovs(int, piece *, piece *);

#endif