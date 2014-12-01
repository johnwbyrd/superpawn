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

#ifndef _UTIL_FUNCTS_H
#define _UTIL_FUNCTS_H

#include "main.h"

void llog(char *, ...);
void log_bin(BITMASK);
void init_matrix();
void init_array();
void init_rc_arr();
void conv_bm_cases(BITMASK, int *, int *);
void get_column_from_bm(BITMASK, int *);
void conv_cases_bm(BITMASK *, int, int);
void trace_time(char*);
unsigned long getesp();

#endif
