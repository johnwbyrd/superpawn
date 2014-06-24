
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Epd2Wb
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	This file is part of the Epd2Wb EPD/Winboard harness project.
//
//	Epd2Wb is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Epd2Wb is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Epd2Wb; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Parts of this module have been adapted from Tim Mann's Winboard project,
//	and are used with permission.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include <windows.h>
#include <stdio.h>

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	fTRUE	1
#define	fFALSE	0

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	coWHITE		0			// This stuff is all for the protover 1
#define	coBLACK		1			//  edit command.
#define	coMAX		2

#define	pcPAWN		0
#define	pcKNIGHT	1
#define	pcBISHOP	2
#define	pcROOK		3
#define	pcQUEEN		4
#define	pcKING		5
#define	pcMAX		6

#define	csqMAX		64

// In epd2wb.h
typedef	struct	tagPCCO {
	char	pc;
	char	co;
}	PCCO;

typedef	struct tagBD {
	PCCO argpcco[csqMAX];
	int	coMove;
}	BD, * PBD;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef struct tagLANG {
	char * szLang;
	char argbPc[pcMAX];
}	LANG, * PLANG;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

BOOL FFenToBd(char * szFen, PBD pbd);
BOOL FCheckAnswer(char * szFen, char * szAttempt, char * szAnswer,
	PLANG plangAttempt, PLANG plangAnswer);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
