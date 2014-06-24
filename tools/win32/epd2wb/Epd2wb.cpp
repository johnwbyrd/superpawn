
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Epd2Wb
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//  Modifications 2002-2005 by Thomas Mayer
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

#include <iostream>
#include <ios>
#include <fstream>
#include "epd2wb.h"
// #include "answer.c"
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <winbase.h>

using namespace std;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Here's how this thing works.

//	INITIALIZATION
//
//	The engine is started, a few commands are sent ("xboard" and
//	"protover 2"), and this harness drops into "FEATURE_TIMEOUT" mode.
//	During this period, the harness is waiting for "feature" commands from the
//	engine.  It will wait a total of five seconds, while listening to whatever
//	the engine has to say.
//
//	After two seconds, the harness assumes the engine is done sending feature
//	commands, drops into "MODE_NORMAL" and sends the engine a few more initial
//	commands ("VPrepEngine").
//
//	It's possible that during the FEATURE_TIMEOUT period, the engine could
//	send a "feature done=0" command.  This is used to indicate that feature
//	commands will be coming from the engine until further notice.  If this
//	happens, the harness goes into "FEATURE_NOTIMEOUT" mode, which is like
//	the normal timeout mode except that it will stay in this mode until it
//	gets a "feature done=1" command, which is handled exactly like a normal
//	timeout.
//
//	"Feature" commands can be received at any time during execution, but will
//	most likely be received at boot.  Some of these have major influence upon
//	how the program operates.
//
//	If I am testing an old engine, it won't send any feature commands, and
//	might ignore me or send random nonsense during the intial wait period.
//	The program can handle old engines, or so I hope.
//
//	HANDLING AN EPD STRING
//
//	Once the harness has entered "NORMAL" mode, either explicitly or via
//	timeout, and has sent the initial commands to the engine, the first EPD
//	string is read from the EPD file.  The harness will process the string,
//	send a "new", put the engine into "force" mode, and depending upon
//	whether or not the engine has told us it can handle "ping":
//
//	1)	If the engine cannot handle "ping", it will sit there for a few
//		seconds (configurable from the command line via "-w"), ignoring all
//		engine output.  This gives the engine time to stop sending analysis.
//		It will then enter "TESTING" mode.  This wait is undesirable but
//		difficult to avoid.
//
//	2)	If the engine can handle "ping", we will send a ping and enter
//		"WAITING" mode.  We are going to ignore everything sent by the engine
//		until it sends a corresponding "pong", which if common sense dictates,
//		will be after the previously sent "new" command takes effect, so the
//		engine won't be sending anymore analysis.
//
//	Now, the task is to send the position.  We will use "setboard" if the
//	engine has told us it can handle it, otherwise we will send the position
//	in pieces via use of the "edit" command.  Note that in this case, castling
//	flags and en-passant square are not sent, and the engine can set them how
//	it likes.
//
//	There are enhancements to "edit" specified by Chessbase, but I won't
//	handle these for now.  These would let us set the en-passant square and
//	castling flags, which are otherwise simply set to what are hoped to be
//	sensible defaults.
//
//	If the engine can handle "analyze", we will now send that command.  If it
//	can't, we'll tell it that it has a really long time think, and put it into
//	normal search mode (with "go").
//
//	If we sent a ping, we'll now ignore all output from the engine until we
//	receive the corresponding "pong".  Once it has received the "pong", it
//	will enter "TESTING" mode and start collecting analysis.
//
//	Once the test period is complete, I'll output some results and move on to
//	the next test.
//
//	If I wasn't able to use "analyze" mode, it's possible that the engine
//	might make a move.  That's fine, although weird things might happen if
//	the engine tries to ponder, which is why "easy" is sent so often.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	I've added another "desperation" switch.  If the "-t" switch is passed,
//	"s_ivars.fUseSt" will be set to TRUE.  If this is the case, the program
//	will try to use "st" and "time" to get the engine to move.  This will be
//	the case even if the engine has "analyze" -- "-t" will override "analyze"
//	and turn it off.
//
//	This works kind of vaguely.  If you want more information, look in
//	EPD2WB.TXT.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	modeFEATURE_TIMEOUT		0
#define	modeFEATURE_NOTIMEOUT	1
#define	modeWAITING				2
#define	modeTESTING				3
#define uciDEBUG                0
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A struct containing info about a child process, in particular the engine.
//	This stuff was adapted (with permission) from similar code in Tim Mann's
//	Winboard.

typedef struct tagCHILDPROC {
	HANDLE hProcess;
	DWORD pid;
	HANDLE hTo;
	HANDLE hFrom;
}	CHILDPROC, * PCHILDPROC;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
	
typedef	unsigned long	TM;

// A struct for some search info, like last analyzed line etc.

typedef struct tagSearchInfo {
	char		 aszLastPV[512];
	int			 cLastDepth;
	int			 cSolveDepth;
	int          cLastEval;
	unsigned int cSolveTime;
} SINFO;

#define	isolMAX		1800
#define	cbINPUT_BUF	4096

//	Global variables.

typedef char Str100[100];

typedef	struct	tagIVARS {
	HANDLE	heventStdinReady;	// Events used by the asynchronous engine
	HANDLE	heventStdinAck;		// input system.
	HANDLE  heventStdinPing;    // Ping / Pong event
	char	aszEngine[256];		// Engine being used or the test.
	char    aszPath[256];       // Path to that engine
	char    aszLogFile[256];    // name of the logfile if any
	char    aszDBFile[256];     // name of the database file
	char	aszSuite[256];		// Engine being used or the test
	char    aszUCIFile[256];    // UCI-Options-File.
	char	aszFen[256];		// EPD FEN for current test.
	char	aszId[256];			// EPD "id" for current test.
	char	aszAnswer[256];		// EPD "bm" for current test, this is double
								//  null-terminated:
								//  <answer1></0><answer2></0></0>
	char	aszAvoid[256];		// EPD "am" for current test, this is double
								//  null-terminated:
								//  <answer1></0><answer2></0></0>
	char    aszOutput[cbINPUT_BUF]; // Outputstring
	Str100  aszFEN[isolMAX];	// Stores the FEN of each position for db
	Str100  aszName[isolMAX];   // Stores the name (info1) of each position
								// for db
	Str100  aszSol[isolMAX];    // stores the Solution string for each pos.
								// for db usage
	Str100  UCIoption[100];     // Stores the UCI-Option-Names
	Str100  UCIvalue[100];      // Stores the default value for each option
	int     UCIoptionstate[100];// Stores the option state
								// Bit 0: 1=value defined in uci option file
								//         0=value defined as default
								// Bit 1: option used by engine
	int UCIoptions;             // number of UCIoptions
	int	movStart;				// EPD doesn't return the full FEN, it drops
								//  the last two fields.  EPD defines fields
								//  specifically for these, and I have
								//  implemented those fields.  If they aren't
								//  present I use sensible defaults ("0 1").
	int	plyFifty;				// EPD fifty-move counter, see my comments
								//  about "movStart".
	int	mode;					// Harness mode, see definitiions above.
	CHILDPROC	cp;				// The engine.
	FILE *	pfI;				// File pointer of the EPD file.
	fstream LOGDatei;			// File stream to logfile;
	fstream DBDatei;			// File stream for database file;
	int	cPing;					// Ping counter.  I'll increment this, ping
								//  the engine with it, then wait for a
								//  corresponding pong with the same argument.
	BOOL	fCorrectAnswer;		// TRUE if the last analysis line I got for
								//  this test indicated a correct answer.
	int cPosAbs;                // absolut number of positions in the 
								// epd-file
	int cWaitAfterStart;        // seconds to wait after start
	int cPosCounter;            // counts the position where we are at the
								//  moment. Could be also a addition of the
								//  following three variables, but I think
								//  it's easier to understand this way.
	int	cSolved;				// Number of positions solved.
	int	cFailed;				// Number of positions not solved.
	int	cError;					// Number of bogus FEN's found.
	int cIPTime;                // How to interpret times given by the engine,
								// 0 = tries to figure out itself,
								// 1 = in centiseconds
								// 2 = in seconds
	int cPlys;                  // Number of plys after which the analysis
								// should stop when the solution is found
	int cScore;                 // Score to reach until the analysis stops
							    // when solution is found
	int cMultiST;               // multiplicator in "-t" - mode
	int	argsol[isolMAX];		// This is an array of the number of answers
								//  found in array index seconds, rounded
								//  down.  So element zero is the number of
						 		//  answers found in between 0 an 999 ms.
	SINFO apsInfo[isolMAX];     // This is an array where the time is saved
	                            //  that it takes to solve each position in
								//  ms. It's used for the summary.
	char	aszInBuf[			// Input buffer from the engine.
		cbINPUT_BUF];
	int	cSkip;					// Skip count, as documented in "epd2wb.txt".
	int	cLine;					// Line of the EPD file I'm processing.
	BOOL	fError;				// This is true if the EPD FEN seems to be
								//  bogus.
	BOOL	fEngineStarted;		// TRUE if I started the chess engine.
	BOOL	fPing;				// TRUE if I can use "ping".
	BOOL	fSetboard;			// TRUE if I can use "setboard".
	BOOL	fAnalyze;			// TRUE if I can use "analyze".
	BOOL	fColor;				// TRUE if I have to endure "color".
	BOOL    fPureProt1;         // TRUE if "protover 2" should not be sent.
	BOOL	fUsermove;			// TRUE if I will deal with "usermove".
	BOOL	fDump;				// TRUE if I'm going to dump everything I get
								//  input and output from/to the engine.
	BOOL	fUseSt;				// TRUE if I'm going to try to use the "st"
								//  command to tell the engine how long to
								//  think.
	BOOL    fUseUCI;            // TRUE when the engine is an UCI-Engine
	BOOL    fUCI1;				// TRUE when only UCI commands should be sent
	BOOL    fInternalTime;      // TRUE if epd2wb should use it's own time
	BOOL	fFindInfo;			// UNDONE BUG!
	BOOL    fIgnoreDone;        // TRUE when feature done should be ignored
	BOOL    fLogOn;             // TRUE if logging turned on
	BOOL    fLogOpen;			// TRUE if logfile opened
	BOOL    fDBon;				// TRUE if Database usage is on
	BOOL    fWaitAfterStart;    // TRUE when epd2wb should wait some seconds
								// before sending the first command (after
								// having started the engine)
	BOOL    fStopPly;           // TRUE when option -g is used
	BOOL    fStopScore;         // TRUE when option -h is used
	BOOL	fQuit;				// After the test is done, I send "quit" to
								//  the engine and wait two seconds.  I might
								//  try to read from the engine during this
								//  time, and get an error message.  If I do,
								//  I will just quit silently without printing
								//  it.  This variable controls that behavior
	BOOL    fStopAnalysis;      // TRUE when the Analysis of the current
								// position should be stopped
	TM	tmEnd;					// Time the current wait period is over,
								//  either the feature wait period, or the
								//  current test.
	TM	tmPerMove;				// Amount of time in milliseconds I am going
								//  to spend per test.
	TM	tmFoundIn;				// If "fCorrectAnswer", this is how many
								//  milliseconds, as reported by the engine,
								//  it took to find the correct answer.  This
								//  is "find and hold", meaning if it changes
								//  its mind later, this value is reset.
	TM	tmWaitInit;				// If the engine does not send done=x during
								//  initialization this is the time how long
								//  epd2wb waits until it goes on.
	TM  tmWaitPos;				// If the engine cannot process "ping", the
								//  utility will wait this many seconds
								//  between tests, ignoring all input from
								//  the engine.
	TM  tmStartPos;             // Time when epd2wb starts to analyze the Pos
	TM	tmStart;				// For engines that accept pings, this is the
								//  time the engine acknowledged the ping that
								//  I sent to indicate that I'm listening to
								//  the engine.  For engines that don't accept
								//  pings, this is the time I told the program
								//  to go.
	PLANG	plang;				// The language I'm using (default English).
	PLANG	plangEnglish;		// Pointer to english language table.
}	IVARS;

IVARS	s_ivars;	// The one instance of this struct.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Time is measured in milliseconds.  This returns time since system boot.

unsigned TmNow(void)
{
	return GetTickCount();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	cbLINE	80	// Assumed width of the output console.

// Main output (and hopefully only except VUsage()) routine, handles also
// logging !
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static char const s_aszModule[] = __FILE__;

void VWriteLogAndOutput()
{
	if (s_ivars.fLogOpen) {
		s_ivars.LOGDatei.write(s_ivars.aszOutput,strlen(s_ivars.aszOutput));
		s_ivars.LOGDatei.seekp(0,ios::end);
	}
	printf("%s",s_ivars.aszOutput);
}

int	CszVectorizeEpd(char * sz, char * rgsz[], int * pibSecond);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VWriteEmpty()
{	
	strcpy(s_ivars.aszOutput,"\n");
	if (s_ivars.fLogOpen) {
		s_ivars.LOGDatei.write(s_ivars.aszOutput,strlen(s_ivars.aszOutput));
		s_ivars.LOGDatei.seekp(0,ios::end);
	}
	printf("%s",s_ivars.aszOutput);
}


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

// #include "epd2wb.h"
#include <ctype.h>

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This module tries to figure out if two strings refer to the same move.  It
//	does this without having any chess knowledge.  Instead, it knows how
//	standard algebraic notation works, and it's able to make enough assertions
//	about the form of moves as describe in algebraic notation that it is able
//	to differentiate between moves represented in various types of algebraic
//	notations.
//
//	It can also successfully compare SAN strings in English to attempted
//	solutions that are in another language, if it knows what the language is.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Remove 'x', '-', '+', '=', ':', '!', '?', and '#' from a string.  The idea
//	is that I want to make a bare-bones move from a more verbose one.  These
//	symbols don't really add any information.
//
//	So this turns Nxe4+ into Ne4, e8=Q# become e8Q, etc.

void VCompact(char * szIn, char * szOut)
{
	for (; *szIn; szIn++)
		switch (*szIn) {
		case 'x':
		case '-':
		case '+':
		case '=':
		case ':':
		case '#':
		case '?':
		case '!':
			break;
		default:
			*szOut++ = *szIn;
			break;
		}
	*szOut = '\0';
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	rnkNIL	-1				// These are used to indicate that the value
#define	filNIL	-1				//  of a rnk, fil, or pc is unknown or empty
#define	pcNIL	-1				//  or what have you.

typedef	struct	tagSQ {
	int	rnk;
	int	fil;
}	SQ, * PSQ;

typedef	struct	tagMOV {
	SQ	sqFrom;					// Source square.
	SQ	sqTo;					// Destination square.
	int	pcFrom;					// Piece that's moving.
	int	pcTo;					// pcNIL or piece promoted to.
}	MOV, * PMOV;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This figures out if "ch" is a piece, and if so which one (it returns
//	"pcPAWN", "pcKNIGHT", etc.
//
//	The function is told what language to use to do its conversion.  If you
//	pass the German language record in here, for instance, "T" becomes pcROOK.

int PcFromCh(int ch, PLANG plang, BOOL fCaseSensitive)
{
	int	pc;
	
	for (pc = pcPAWN; pc <= pcKING; pc++) {
		if (ch == plang->argbPc[pc])
			return pc;
		if ((!fCaseSensitive) && (tolower(ch) == tolower(plang->argbPc[pc])))
			return pc;
	}
	return pcNIL;
}

// This is just a function to count the total positions in the epd-file
// Thanks to Leen Ameraal !

int fGetAbsPos(char * sz)
{
   FILE *epdfile;
   int npos = 0, nslash = 0, ch;
   epdfile = fopen(sz, "r");
   do
   {  ch = getc(epdfile);
      if (ch == '/')
         nslash++;
      else
      if ((ch == '\n' || ch == EOF) && nslash >= 7)
      {  npos++;
         nslash = 0;
      }
   }while(ch != EOF);
   fclose(epdfile);
   return npos;
}

// The following function reads the pre-defined UCI-options

void ReadUCIOptions(void)
{
	FILE *ucifile;
	char ch;
	char line[256];
	char ucioption[256];
	char * argsz[256];
	int pibsecond,i,j,k;
	long slen=0;
	bool found;
	ucifile=fopen(s_ivars.aszUCIFile,"r");
	do {
		ch=getc(ucifile);
		if ((ch == '\n') || (ch == EOF)) {
			line[slen]=0;
			slen=CszVectorizeEpd(line,argsz,&pibsecond);
			if (!strcmp("option",argsz[0])) {
				strcpy(ucioption,argsz[1]);
				k=2;
				while (strcmp(argsz[k],"value")) {
					strcat(ucioption," ");
					strcat(ucioption,argsz[k]);
					k++;
				}
				i=0;
				found=false;
				while ((i<s_ivars.UCIoptions) && (!found)) {
					if (!strcmp(s_ivars.UCIoption[i],ucioption)) {
						found=true;
						sprintf(s_ivars.UCIvalue[i],"\0");
						for (j=k+1;j<slen;j++) {
							if (j>(k+1)) {
								strcat(s_ivars.UCIvalue[i]," ");
							}
							strcat(s_ivars.UCIvalue[i],argsz[j]);
						}
						s_ivars.UCIoptionstate[i]|=1;
					} else {
						i++;
					}
				}
				if (!found) {
					i=s_ivars.UCIoptions;
					strcpy(s_ivars.UCIoption[i],ucioption);
					strcpy(s_ivars.UCIvalue[i],"\0");
					for (j=k+1;j<slen;j++) {
						if (j>(k+1)) {
							strcat(s_ivars.UCIvalue[i]," ");
						}
						strcat(s_ivars.UCIvalue[i],argsz[j]);
					}
					s_ivars.UCIoptionstate[i]=1;
					s_ivars.UCIoptions++;
				}
			}
			slen=0;
		} else {
			line[slen]=ch;
			slen++;
		}
	} while(ch != EOF);
	fclose(ucifile);
}
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These functions try to get a rank, file, or full coordinate from "sz"

BOOL FGetRnk(char * sz, int * prnk)
{
	if ((sz[0] < '1') || (sz[0] > '8'))
		return fFALSE;
	*prnk = sz[0] - '1';
	return fTRUE;
}

BOOL FGetFil(char * sz, int * pfil)
{
	if ((sz[0] < 'a') || (sz[0] > 'h'))
		return fFALSE;
	*pfil = sz[0] - 'a';
	return fTRUE;
}

BOOL FGetCoord(char * sz, PSQ psq)
{
	SQ	sq;		// I use this to avoid writing crap into the output square if
				//  I get a file but not a rank.
	
	if (!FGetFil(sz, &sq.fil))
		return fFALSE;
	if (!FGetRnk(sz + 1, &sq.rnk))
		return fFALSE;
	*psq = sq;
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Converts a coordinate into an ISQ (an integer from 0..63).

int IsqFromSq(PSQ psq)
{
	return psq->rnk * 8 + psq->fil;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This derives as much information as possible from "szMov" and stuffs it
//	into "pmov".  It returns FALSE if the move made no sense.
//
//	"szMov" is either in full coordinate notation, coordinate notation
//	preceded by piece type, or SAN.  Promoted pieces can be in either upper
//	or lower case.
//
//	I use a few goto's to compact the code a little.

BOOL FDecipher(PBD pbd, PLANG plang, char * szMov, PMOV pmov)
{
	//	Check for a couple of obvious exception cases (remember, the "-" is
	//	gone by this time).
	//
	if ((!strcmp(szMov, "OO")) || (!strcmp(szMov, "oo")) || (!strcmp(szMov, "00")))
		szMov = (pbd->coMove == coWHITE) ? "e1g1" : "e8g8";
	else if ((!strcmp(szMov, "OOO")) || (!strcmp(szMov, "ooo")) || (!strcmp(szMov, "000")))
		szMov = (pbd->coMove == coWHITE) ? "e1c1" : "e8c8";
	//
	//	Set all the fields to unknown except the "from" piece, which is going
	//	to be set on the line after this.
	//
	pmov->sqFrom.rnk = pmov->sqTo.rnk = rnkNIL;
	pmov->sqFrom.fil = pmov->sqTo.fil = filNIL;
	pmov->pcTo = pcNIL;
	//
	//	The full form here is something like Nb1c3.  We can also handle
	//	b1c3, N1c3, Nbc3, or Nc3.
	//
	//	If I get something like "bc3", "b3", "b8Q", or "b8q", I'm going
	//	to assume the "from" piece must be a pawn.
	//
	//	First, check for an explicit piece.
	//
	if ((pbd->coMove==coBLACK) && (strlen(szMov)>=5) && (isdigit(szMov[2])) && (isdigit(szMov[4])))
	{
		if ((pmov->pcFrom = PcFromCh(*szMov, plang, fFALSE)) != pcNIL)	// Hagrid Bug
			szMov++;
	}
	else
	{
		if ((pmov->pcFrom = PcFromCh(*szMov, plang, fTRUE)) != pcNIL)
			szMov++;
	}
	//
	//	Next, try to get a coord.
	//
	if (FGetCoord(szMov, &pmov->sqFrom)) {
		szMov += 2;
		//
		//	If we got one coord, try to get another one.
		//
		if (FGetCoord(szMov, &pmov->sqTo)) {
			int	pcFrom;

			szMov += 2;
			//
			//	If we're here, we have two coords.  We can go to the "from"
			//	square and get the piece there.  If this conflicts with an
			//	explicit piece, bomb out, otherwise remember it.
			//
			pcFrom = pbd->argpcco[IsqFromSq(&pmov->sqFrom)].pc;
			if (pcFrom == pcNIL)	// <-- This indicates an illegal move.
				return fFALSE;
			if ((pmov->pcFrom != pcNIL) && (pcFrom != pmov->pcFrom))
				return fFALSE;
			pmov->pcFrom = pcFrom;
			//
			//	Try to get the piece promoted to if it is there.  If we don't
			//	know what the moving piece was -- surprise! -- it was a pawn,
			//	so remember that.
			//
			//	I am careful to allow "e7e8q" here, as well as "e7e8Q".
			//
lblPromote:	if ((pmov->pcTo = PcFromCh(*szMov, plang, fFALSE)) != pcNIL) {
				szMov++;
				if ((pmov->pcFrom != pcNIL) && (pmov->pcFrom != pcPAWN))
					return fFALSE;
				pmov->pcFrom = pcPAWN;
			}
			if (*szMov == '\0')
				return fTRUE;
		} else {
			//
			//	If we're here, we got one coordinate, but not a second.  I
			//	have mistakenly put this coordinate into the "from" coord.
			//	It should actually go into the "to" coord, so fix that.
			//
			//	If we don't know what the piece being moved was, it was a
			//	pawn (we've gotten something like "e4").
			//
			pmov->sqTo = pmov->sqFrom;
			pmov->sqFrom.rnk = rnkNIL;
			pmov->sqFrom.fil = filNIL;
			if (pmov->pcFrom == pcNIL)
				pmov->pcFrom = pcPAWN;
			goto lblPromote;
		}
	} else if (FGetFil(szMov, &pmov->sqFrom.fil)) {
		//
		//	If I am here, I couldn't get a coord, but I got a file.  This is
		//	fine whether or not there is an explicit piece, but if there isn't
		//	one, the piece must be a pawn.  Examples here are "Nbd7" and
		//	"ab4".
		//
		if (pmov->pcFrom == pcNIL)
			pmov->pcFrom = pcPAWN;
		//
		//	Go get the destination square and then deal with promotion.
		//
lblTo:	szMov++;
		if (FGetCoord(szMov, &pmov->sqTo)) {
			szMov += 2;
			goto lblPromote;
		}
	} else if (FGetRnk(szMov, &pmov->sqFrom.rnk)) {
		//
		//	If I'm here, I couldn't get a coord, but I got a rank.  That's
		//	fine as long as we had an explicit piece.  "N6d7" is okay, but
		//	"6d7" is not.  "P6d7" is also not allowed.
		//
		if ((pmov->pcFrom == pcNIL) || (pmov->pcFrom == pcPAWN))
			return fFALSE;
		//
		//	Go get the destination square.
		//
		goto lblTo;
	}
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This takes two moves and compares them.  It's possible that one or both
//	may be partially filled in, rather than completely filled in.

BOOL FCompare(PMOV pmovTry, PMOV pmovAnswer)
{
	//	We have a destination square for sure, so let's compare that.
	//
	if (pmovTry->sqTo.rnk != pmovAnswer->sqTo.rnk)
		return fFALSE;
	if (pmovTry->sqTo.fil != pmovAnswer->sqTo.fil)
		return fFALSE;
	//
	//	The promotion piece should also be accurate.
	//
	if (pmovTry->pcTo != pmovAnswer->pcTo)
		return fFALSE;
	//
	//	Okay, at this point we know that both moves are going to the right
	//	square, and the promotion problem has been ruled out.  Let's check
	//	to see if anything else definitely does not match.  If I'm comparing
	//	two values, both of which are known, and they aren't the same, that's
	//	an obvious failure.
	//
	if ((pmovTry->sqFrom.rnk != rnkNIL) &&
		(pmovAnswer->sqFrom.rnk != rnkNIL) &&
		(pmovTry->sqFrom.rnk != pmovAnswer->sqFrom.rnk))
		return fFALSE;
	if ((pmovTry->sqFrom.fil != filNIL) &&
		(pmovAnswer->sqFrom.fil != filNIL) &&
		(pmovTry->sqFrom.fil != pmovAnswer->sqFrom.fil))
		return fFALSE;
	if ((pmovTry->pcFrom != pcNIL) && (pmovAnswer->pcFrom != pcNIL) &&
		(pmovTry->pcFrom != pmovAnswer->pcFrom))
		return fFALSE;
	//
	//	The "to" coordinate is finished, so now I need to deal with the
	//	"from" coordinate.
	//
	//	If I am comparing two full coordinates, I am done, since the checks
	//	above test that case.
	//
	//	If I didn't get a full coordinate in both cases, and the piece being
	//	moved is a pawn, I am either capturing to the destination or moving
	//	there.  If I am moving there, the move cannot be ambiguous.  If I am
	//	capturing, I must have at least a "file" dis-ambiguator in all cases,
	//	and this would have passed the above checks.
	//
	//	If I didn't get a full coordinate, and the piece being moved is not
	//	a pawn, the move is either not ambiguous, in which case I'm fine, or
	//	the move is ambiguous and was marked as such.  That would have passed
	//	the checks above.
	//
	//	The only case I'm not catching here is when I have Ne4c3 given as
	//	Nec3 in the EPD (presumably, because that is correct), and N4c3
	//	passed in by the engine.  I can't tell if that's the same move, and I
	//	would call that a match, which it is.  But N2c3 is also a match if
	//	this knight on on e2.
	//
	//	The only reason this problem is not crushing is that N4c3 is not
	//	legal SAN if Nec3 adequately disambiguates (disambiguation via a file
	//	is preferred).
	//
	//	So:
	//
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	We want to figure out if "szTry" matches "szAnswer".  I'm expecting that
//	both are in SAN or some form of coordinate notation.  I'm going to lightly
//	assume that both moves are legal and give enough information to
//	disambiguate.  This is an advantage, and will let me figure out what is
//	going on without using any chess knowledge.

BOOL FCheckAnswer(char * szFen, char * szTry, char * szAnswer,
	PLANG plangTry, PLANG plangAnswer)
{
	char	aszAnswer[32];
	char	aszTry[32];
	MOV	movTry;
	MOV	movAnswer;
	BD	bd;

	VCompact(szTry, aszTry);
	VCompact(szAnswer, aszAnswer);
	if ((plangTry == plangAnswer) && (!strcmp(szAnswer, szTry)))
		return fTRUE;	// Identical string & language is a success.
	if (!FFenToBd(szFen, &bd))			// If the FEN is broken, the test
		return fFALSE;					//  can't have succeeded.
	//
	//	Try to interpret what I've been given from the EPD line and from the
	//	analysis line.
	//
	//	The case where the correct answer can't be deciphered is a really bad
	//	case, and it would be nice if I could report that to the user, but
	//	that's a bit complicated and I can't report it with 100% accuracy
	//	anyway.
	//
	if (!FDecipher(&bd, plangTry, aszTry, &movTry))
		return fFALSE;
	if (!FDecipher(&bd, plangAnswer, aszAnswer, &movAnswer))
		return fFALSE;
	//
	//	Compare these two deciphered moves.
	//
	if (FCompare(&movTry, &movAnswer))
		return fTRUE;
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VAssertFailed(const char * szMod, int iLine)
{
	sprintf(s_ivars.aszOutput,"Assert Failed: %s+%d\n", szMod, iLine);
	VWriteLogAndOutput();
	exit(1);
}

#define	Assert(cond)		if (!(cond)) VAssertFailed(s_aszModule, __LINE__)

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This converts the last Windows error into a text string, displays it along
//	with some harness-specific error text, and exits.

//	This code was once again adapted (with permission) from code in Tim Mann's
//	Winboard.

void VDisplayLastError(char * sz)
{
	int	len;
	char	aszBuf[512];
	DWORD	dw = GetLastError();

	if (!s_ivars.fQuit) {
		len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dw, LANG_NEUTRAL, aszBuf, sizeof(aszBuf), NULL);
		if (len > 0) {
			fprintf(stderr, "%s: %s", sz, aszBuf);
			exit(1);
		}
		fprintf(stderr, "%s: error = %ld\n", sz, dw);
	}
	exit(1);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is supposed to kill the engine, but when I call it, it hangs, so I
//	don't call it.

void DestroyChildProcess(PCHILDPROC pcp)
{
	CloseHandle(pcp->hTo);
    if (pcp->hFrom)
		CloseHandle(pcp->hFrom);
    CloseHandle(pcp->hProcess);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This code was adapted (with permission) from code in Tim Mann's Winboard.
//	It starts the engine while hooking the engine's input and output.

//	This function returns FALSE if it fails, and "GetLastError()" might
//	explain a little more about why it failed.

BOOL FStartProcess(PCHILDPROC pcp, char * szEngine, char * szPath)
{
	HANDLE	hChildStdinRd;
	HANDLE	hChildStdinWr;
	HANDLE	hChildStdoutRd;
	HANDLE	hChildStdoutWr;
	HANDLE	hChildStdinWrDup;
	HANDLE	hChildStdoutRdDup;
	PROCESS_INFORMATION	piProcInfo;
	STARTUPINFO	siStartInfo;
	SECURITY_ATTRIBUTES saAttr;
	char szEngineFull[256];

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
		return fFALSE;
	if (!DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
		GetCurrentProcess(), &hChildStdoutRdDup, 0,
		FALSE, DUPLICATE_SAME_ACCESS))
		return fFALSE;
	CloseHandle(hChildStdoutRd);
	if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
		return fFALSE;
	if (!DuplicateHandle(GetCurrentProcess(), hChildStdinWr,
		GetCurrentProcess(), &hChildStdinWrDup, 0, FALSE,
		DUPLICATE_SAME_ACCESS))
		return fFALSE;
	CloseHandle(hChildStdinWr);
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.lpReserved = NULL;
	siStartInfo.lpDesktop = NULL;
	siStartInfo.lpTitle = NULL;
	siStartInfo.dwFlags = STARTF_USESTDHANDLES;
	siStartInfo.cbReserved2 = 0;
	siStartInfo.lpReserved2 = NULL;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdError = hChildStdoutWr;
	strcpy(szEngineFull,szPath);
	strcat(szEngineFull,szEngine);
	if (strlen(szPath)==0) szPath=NULL;
	if (!CreateProcess(NULL, szEngineFull, NULL, NULL, TRUE,
		DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
		NULL, szPath, &siStartInfo, &piProcInfo))
		return fFALSE;
	CloseHandle(hChildStdinRd);
	CloseHandle(hChildStdoutWr);
	pcp->hProcess = piProcInfo.hProcess;
	pcp->pid = piProcInfo.dwProcessId;
	pcp->hFrom = hChildStdoutRdDup;
	pcp->hTo = hChildStdinWrDup;
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#if	0

//	This debug routine might come in handy.

void VDumpRaw(char * rgb, int cb)
{
	int	ib;
	char aszAddText[20];

	sprintf(s_ivars.aszOutput,"RAW: ");
	for (ib = 0; ib < cb; ib++) {
		if ((rgb[ib] < ' ') || (rgb[ib] > '~')) {
			sprintf(aszAddText,"<%02Xh>", rgb[ib]);
			strcat(s_ivars.aszOutput,aszAddText);
		}
		else
			strcat(s_ivars.aszOutput,rgb[ib]);
	}
	strcat(s_ivars.aszOutput,'\n');
	VWriteLogAndOutput();
}

#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is a thread function.  It will sit here waiting for permission to
//	send the next line.  Once it gets it, it will read a line from the engine,
//	then it will break it up into chunks (delimited by \r\n) and pass them
//	back to the main thread one chunk at a time via "s_ivars.aszInBuf".

DWORD WINAPI DwInput(void * pv)
{
	char	argb[cbINPUT_BUF];
	int	ib = 0;
	int	cb = 0;
	int	ibOut = 0;

	for (;;) {
		WaitForSingleObject(s_ivars.heventStdinAck, INFINITE);
		for (;;) {
			Assert(ib <= cb);
			if (ib == cb) {		// Empty buffer, read a new line.
				DWORD	dw;

				if (!ReadFile(s_ivars.cp.hFrom,
					argb, sizeof(argb), &dw, NULL))
					VDisplayLastError("Can't read from engine");
				ib = 0;
				cb = dw;
/*				if (s_ivars.fDump) {
					sprintf(s_ivars.aszOutput,"RAWin> %s\n", argb);
					VWriteLogAndOutput();
				} */
//				VDumpRaw(argb, cb);
				Assert(cb <= sizeof(argb));
			} else if ((argb[ib] == '\r') || (argb[ib] == '\n')) {
				if ((cb<(ib+7)) || (strncmp(&argb[ib+2],"    ",4)))	// KnightDreamer
				{
					if ((++ib < cb) && (argb[ib] == '\n'))
						ib++;
					s_ivars.aszInBuf[ibOut] = '\0';
					ibOut = 0;
					if (s_ivars.fDump) {
						sprintf(s_ivars.aszOutput,"LineCut> %s\n", s_ivars.aszInBuf);
						VWriteLogAndOutput();
					}
					// For what the hell ?
	/*				else if ((s_ivars.aszInBuf[0] == '#')
							&& (strlen(s_ivars.aszInBuf)>1)) {
						sprintf(s_ivars.aszOutput,"%s\n", s_ivars.aszInBuf);
						VWriteLogAndOutput();
					} */
					SetEvent(s_ivars.heventStdinReady);
					break;
				}
				else										// KnightDreamer
				{
					ib+=6;
					s_ivars.aszInBuf[ibOut++]=argb[ib++];
				}
			} else
				s_ivars.aszInBuf[ibOut++] = argb[ib++];
		}
	}
	return 0;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This writes a line of stuff to the engine, with a '\n' appended.

void VSendToEngine(const char * szFmt, ...)
{
	char	aszBuf[2048];
	va_list	lpArgPtr;
	int	cb;
	DWORD	dw;

	va_start(lpArgPtr, szFmt);
	vsprintf(aszBuf, szFmt, lpArgPtr);
	cb = strlen(aszBuf);
	aszBuf[cb++] = '\n';
	aszBuf[cb] = '\0';
	if (s_ivars.fDump) {
		sprintf(s_ivars.aszOutput,"<%s", aszBuf);
		VWriteLogAndOutput();
	}
	if (!WriteFile(s_ivars.cp.hTo, aszBuf, cb, &dw, NULL))
		VDisplayLastError("Can't write to engine");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This breaks a space-delimited line ("sz") up into pieces and returns a
//	count of the pieces.  The line has null characters poked into it as part
//	of the process.
//
//	"rgsz" is an array of pointers to the pieces.
//
//	"*pibSecond" is the index in "sz" of the second piece.
//
//	This function handles double-quoted matter by deleting the quotes and
//	ignoring spaces that occur within the quoted matter.  So:
//
//		id "tough position"
//
//	is turned into:
//
//		0: id
//		1: tough position

int	CszVectorizeEpd(char * sz, char * rgsz[], int * pibSecond)
{
	int	i;
	int	csz;

	for (csz = 0, i = 0; sz[i]; i++)
		if (sz[i] != ' ') {
			BOOL	fInQuote;

			if (sz[i] == '"') {
				fInQuote = fTRUE;
				i++;
			} else
				fInQuote = fFALSE;
			if (csz == 1)
				*pibSecond = i;
			rgsz[csz++] = sz + i;
			for (;; i++) {
				if ((sz[i] == ' ') && (!fInQuote))
					break;
				if ((sz[i] == '"') && (fInQuote))
					break;
				if (sz[i] == '\0')
					break;
			}
			if (sz[i] == '\0')
				break;
			sz[i] = '\0';
		}
	if (csz <= 1)
		*pibSecond = i;
	return csz;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is similar to the "CszVectorizeEpd" function, but it doesn't strip
//	quotes, and if a quote occurs in the middle of a token, that's okay.
//
//	"feature myname="Mint v2.1" playother=1"
//
//		feature
//		myname="Mint v2.1"
//		playother=1

int	CszVectorizeCmd(char * sz, char * rgsz[], int * pibSecond)
{
	int	i;
	int	csz;

	for (csz = 0, i = 0; sz[i]; i++)
		if ((sz[i] != ' ') && (sz[i] != '\t')) {
			BOOL	fInQuote;

			if (csz == 1)
				*pibSecond = i;
			rgsz[csz++] = sz + i;
			fInQuote = fFALSE;
			for (;; i++) {
				if (((sz[i] == ' ') || (sz[i] == '\t')) && (!fInQuote))
					break;
				if (sz[i] == '"')
					fInQuote = !fInQuote;
				if (sz[i] == '\0')
					break;
			}
			if (sz[i] == '\0')
				break;
			sz[i] = '\0';
		}
	return csz;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This removes the newline at the end of "sz", if it is there.

void VStrip(char * sz)
{
	int	i;

	for (i = 0; sz[i]; i++)
		if (sz[i] == '\n') {
			sz[i] = '\0';
			break;
		}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These commands are sent to the engine once at the start of the run, after
//	the engine seems to be done sending the harness "feature" commands.

void VPrepEngine(void)
{
	VSendToEngine("new");
	VSendToEngine("level 0 5 0");
	VSendToEngine("post");
	VSendToEngine("hard");
	VSendToEngine("easy");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	In a few places I am going to vectorize a string, then search for the
//	first vectorized element in a command table, then call a function with
//	the original string and the vector.  This structure facilitates this.

typedef	struct	tagCMD {
	char * sz;
	BOOL (* pfn)(char * sz, char * rgsz[], int csz);
}	CMD, * PCMD;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	EPD processing commands.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This one processes "bm".  It converts the arguments into a double null-
//	terminated string.

BOOL FCmdBm(char * sz, char * rgsz[], int csz)
{
	int	ib;
	int	i;

	ib = 0;
	for (i = 1; i < csz; i++)
		ib += sprintf(s_ivars.aszAnswer + ib, "%s", rgsz[i]) + 1;
	s_ivars.aszAnswer[ib] = '\0';
	return fTRUE;
}

//	This one processes "am".  It converts the arguments into a double null-
//	terminated string.

BOOL FCmdAm(char * sz, char * rgsz[], int csz)
{
	int	ib;
	int	i;

	ib = 0;
	for (i = 1; i < csz; i++)
		ib += sprintf(s_ivars.aszAvoid + ib, "%s", rgsz[i]) + 1;
	s_ivars.aszAvoid[ib] = '\0';
	return fTRUE;
}

//	This one processes "id".  It just remembers the argument.

BOOL FCmdId(char * sz, char * rgsz[], int csz)
{
	if (csz < 2)
		return fTRUE;
	strcpy(s_ivars.aszId, rgsz[1]);
	return fTRUE;
}

//	This one processes "fmvn", which stands for "first move number".

BOOL FCmdFmvn(char * sz, char * rgsz[], int csz)
{
	if (csz < 2)
		return fTRUE;
	s_ivars.movStart = atoi(rgsz[1]);
	if (s_ivars.movStart < 1)
		s_ivars.movStart = 1;
	return fTRUE;
}

//	This one processes "hmvc", which is the fifty-move counter (number of
//	moves since the last reversible move.

BOOL FCmdHmvc(char * sz, char * rgsz[], int csz)
{
	if (csz < 2)
		return fTRUE;
	s_ivars.plyFifty = atoi(rgsz[1]);
	if (s_ivars.plyFifty < 0)
		s_ivars.plyFifty = 0;
	return fTRUE;
}

//	EPD command table.

CMD const c_argcmdEpd[] = {
	"bm",			FCmdBm,		// Best move
	"am",			FCmdAm,		// Avoid move
	"id",			FCmdId,		// Id
	"fmvn",			FCmdFmvn,	// Full move number.
	"hmvc",			FCmdHmvc,	// "Half-move clock".  Fifty-move counter.
	NULL,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This ignores all input for the specified number of seconds.  I use this
//	if I can't use ping, to give the engine time to shut up and stop spitting
//	analysis crap at me.

void VKillTime(TM tmToWait)
{
	TM	tmEnd = TmNow() + tmToWait;

	for (;;) {
		if (WaitForSingleObject(s_ivars.heventStdinReady,
			tmEnd - TmNow()) == WAIT_TIMEOUT)
			break;
		SetEvent(s_ivars.heventStdinAck);
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VSendMoveToEngine(char * szMov)
{
	if (s_ivars.fUsermove)
		VSendToEngine("usermove %s", szMov);
	else
		VSendToEngine(szMov);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

BOOL FFenToBd(char * szFen, PBD pbd)
{
	int	isq;
	int	rnk;
	int	fil;
	int	co;
	
	for (isq = 0; isq < csqMAX; isq++)
		pbd->argpcco[isq].co = coMAX;
	rnk = 7;
	fil = 0;
	for (;; szFen++)
		if (*szFen == ' ')
			break;
		else if (*szFen == '/') {
			rnk--;
			fil = 0;
		} else if ((*szFen >= '1') && (*szFen <= '8'))
			fil += *szFen - '0';
		else if ((fil > 7) || (rnk < 0))
			return fFALSE;
		else {
			int	pc;
			
			for (pc = pcPAWN; pc <= pcKING; pc++)
				if (s_ivars.plangEnglish->argbPc[pc] == *szFen) {
					co = coWHITE;
					break;
				}
			if (pc > pcKING)
				for (pc = pcPAWN; pc <= pcKING; pc++)
					if (s_ivars.plangEnglish->argbPc[pc] ==
						toupper(*szFen)) {
						co = coBLACK;
						break;
					}
			if (pc > pcKING)
				return fFALSE;
			pbd->argpcco[rnk * 8 + fil].pc = pc;
			pbd->argpcco[rnk * 8 + fil].co = co;
			fil++;
		}
	switch (*++szFen) {
	case 'w':
		pbd->coMove = coWHITE;
		break;
	case 'b':
		pbd->coMove = coBLACK;
		break;
	default:
		return fFALSE;
	}
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is NOT guaranteed to get a correct FEN.

BOOL FSendEdit(char * szFen)
{
	BD	bd;
	int	co;

	if (!FFenToBd(szFen, &bd))
		return fFALSE;
	if (bd.coMove == coBLACK)
		VSendMoveToEngine("a2a3");
	VSendToEngine("edit");
	VSendToEngine("#");
	for (co = coWHITE; co <= coBLACK; co++) {
		int	isq;

		for (isq = 0; isq < csqMAX; isq++)
			if (bd.argpcco[isq].co == co) {
				char	asz[8];
				
				asz[0] = s_ivars.plangEnglish->argbPc[bd.argpcco[isq].pc];
				asz[1] = (isq % 8) + 'a';
				asz[2] = (isq / 8) + '1';
				asz[3] = '\0';
				VSendToEngine(asz);
			}
		VSendToEngine((co == coWHITE) ? "c" : ".");
	}
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This gets a line from the EPD file, breaks it up and processes it, gets
//	the engine ready to go, and sends a ping.

//	Later on, something else will see the "pong" from the engine, and the
//	engine will be told to analyze.

//	If there are no more EPD lines, or if this EPD line is seriously broken,
//	this returns FALSE.

BOOL FNextTest(void)
{
	char	aszBuf[256];
	char    aszAddText[15];
	int	i;
	int	cSpaces;
	int	coMove;

	for (;;) {
		//
		//	Get the line.
		//
		if (fgets(aszBuf, sizeof(aszBuf), s_ivars.pfI) != aszBuf)
			return fFALSE;
		s_ivars.cLine++;
		VStrip(aszBuf);
		if (aszBuf[0] != '\0')
			break;
	}
	//	Break the FEN out of the line.  The EPD standard doesn't include the
	//	list two fields of the FEN.  These may be added while processing
	//	the semi-colon terminated EPD fields that will probably follow.
	//
	for (i = cSpaces = 0; aszBuf[i]; i++) {
		if (aszBuf[i] == ' ') {
			if (++cSpaces == 4)
				break;
			if (cSpaces == 1)
				if (aszBuf[i + 1] == 'w')
					coMove = coWHITE;
				else
					coMove = coBLACK;
		}
		s_ivars.aszFen[i] = aszBuf[i];
	}
	if (aszBuf[i] == '\0') {	// If I didn't find the FEN I assume I'm
								//  broken.
		sprintf(s_ivars.aszOutput,"Obviously bogus FEN, line %d\n", s_ivars.cLine);
		VWriteLogAndOutput();
		return fFALSE;
	}
	s_ivars.aszFen[i] = '\0';
	//
	//	I'm sitting at the space after the FEN now.  I'm going to eat EPD
	//	semi-colon delimited fields.  First step is to zero some fields that
	//	I might not find.
	//
	s_ivars.aszId[0] = '\0';
	s_ivars.aszAnswer[0] = '\0';
	s_ivars.aszAvoid[0] = '\0';
	s_ivars.movStart = 1;
	s_ivars.plyFifty = 0;
	s_ivars.apsInfo[s_ivars.cPosCounter].aszLastPV[0]='\0';
	s_ivars.apsInfo[s_ivars.cPosCounter].cLastDepth=0;
	s_ivars.apsInfo[s_ivars.cPosCounter].cSolveDepth=0;
	s_ivars.apsInfo[s_ivars.cPosCounter].cSolveTime=-1;
	//
	//	One pass through this loop for every EPD field.
	//
	for (;;) {
		char	aszCmd[256];
		int	j;
		char *	argsz[256];
		int	ibSecond;
		int	csz;

		while (aszBuf[i] == ' ')	// Strip preceding spaces.
			i++;
		if (aszBuf[i] == '\0')		// A null here indicates no more fields.
			break;
		//
		//	Grab everything from here until the semi-colon.
		//
		for (j = 0; aszBuf[i]; i++) {
			if (aszBuf[i] == ';') {
				i++;
				break;
			}
			aszCmd[j++] = aszBuf[i];
		}
		aszCmd[j] = '\0';
		//
		//	Break the argument up, then try to process it via the command
		//	table.
		//
		csz = CszVectorizeEpd(aszCmd, argsz, &ibSecond);
		if (csz)
			for (j = 0; c_argcmdEpd[j].sz != NULL; j++)
				if (!strcmp(c_argcmdEpd[j].sz, argsz[0])) {
					(*c_argcmdEpd[j].pfn)(aszBuf + ibSecond, argsz, csz);
					break;
				}
	}
	//	At this point the entire EPD is eaten.
	//
	sprintf(s_ivars.aszFen + strlen(s_ivars.aszFen), " %d %d",
		s_ivars.plyFifty, s_ivars.movStart);
	s_ivars.fCorrectAnswer = fFALSE;
	s_ivars.tmFoundIn = -1;
	s_ivars.fError = fFALSE;
	strcpy(s_ivars.aszFEN[s_ivars.cPosCounter],s_ivars.aszFen);
	s_ivars.aszName[s_ivars.cPosCounter][0]=0;
	sprintf(s_ivars.aszOutput,"Nr:%5d (of %i)\n",s_ivars.cPosCounter+1,s_ivars.cPosAbs);
	VWriteLogAndOutput();
	strncpy(s_ivars.aszName[s_ivars.cPosCounter],s_ivars.aszId,99);
	strcat(s_ivars.aszName[s_ivars.cPosCounter],"\0");
	if (s_ivars.aszId[0] != '\0') {
		sprintf(s_ivars.aszOutput,"Id:  %s\n", s_ivars.aszId);
		VWriteLogAndOutput();
	}
	sprintf(s_ivars.aszOutput,"Fen: %s\n", s_ivars.aszFen);
	VWriteLogAndOutput();
	s_ivars.aszSol[s_ivars.cPosCounter][0]=0;
	if (s_ivars.aszAnswer[0] != '\0') {
		sprintf(s_ivars.aszOutput,"Bm: ");
		for (i = 0; s_ivars.aszAnswer[i] != '\0';) {
			sprintf(aszAddText," %s", s_ivars.aszAnswer + i);
			strcat(s_ivars.aszOutput,aszAddText);
			i += strlen(s_ivars.aszAnswer + i) + 1;
		}
		strcpy(s_ivars.aszSol[s_ivars.cPosCounter],s_ivars.aszOutput);
		strcat(s_ivars.aszOutput,"\n");
		VWriteLogAndOutput();
	}
	if (s_ivars.aszAvoid[0] != '\0') {
		sprintf(s_ivars.aszOutput,"Am: ");
		for (i = 0; s_ivars.aszAvoid[i] != '\0';) {
			sprintf(aszAddText," %s", s_ivars.aszAvoid + i);
			strcat(s_ivars.aszOutput,aszAddText);
			i += strlen(s_ivars.aszAvoid + i) + 1;
		}
		strcpy(s_ivars.aszSol[s_ivars.cPosCounter],s_ivars.aszOutput);
		strcat(s_ivars.aszOutput,"\n");
		VWriteLogAndOutput();
	}
	VWriteEmpty();
	s_ivars.fStopAnalysis=fFALSE; // Do not stop now
	//
	//	Tell the engine to do its thing.
	//
	// VSendToEngine("force"); - already sent after "exit"
	if ((s_ivars.fUseUCI) && (!s_ivars.fUCI1)) {
		VSendToEngine("ucinewgame");
	} else {
		VSendToEngine("new");
	}
	//
	//	If I can't use "analyze", I have to give the engine a time control.
	//	This is problematic.  I don't know if all engines support "st".  If
	//	they do, that's the solution.  Otherwise, I set this to do a game
	//	that's sudden death in 10000 minutes.  That's a long time and I'd hope
	//	that it will prove long enough for most purposes.
	//
	//	If you can think of a better way to do this, or if this causes you
	//	significant trouble, please email me at brucemo@seanet.com
	//
	//
	//	Send ping if I can, otherwise wait efficiently for a while.
	//
	if (s_ivars.fUseUCI) {
		VSendToEngine("isready");
		s_ivars.mode=modeWAITING;
//		WaitForSingleObject(s_ivars.heventStdinPing, 15000);
	} else {
		if (s_ivars.fPing) {
			VSendToEngine("ping %d", ++s_ivars.cPing);
			s_ivars.mode = modeWAITING;
//			WaitForSingleObject(s_ivars.heventStdinPing, 15000);
		} else {
			VKillTime(s_ivars.tmWaitPos/2);
			s_ivars.mode = modeTESTING;
		}
	}
	//
	// We try it with another row... ?!
	//
	if (!s_ivars.fUseUCI) {
		if (!s_ivars.fAnalyze)
			VSendToEngine("level 0 10000 0");
		else
			VSendToEngine("level 0 5 0");
		VSendToEngine("post");
		VSendToEngine("hard");
		VSendToEngine("easy");
		VSendToEngine("force");
	}
	//
	//	Send the position.
	//
	if (s_ivars.fUseUCI) {
		VSendToEngine("position fen %s", s_ivars.aszFen);
	}
	else {
		if (s_ivars.fSetboard) {
			VSendToEngine("setboard %s", s_ivars.aszFen);
			if ((s_ivars.fColor) && (!s_ivars.fAnalyze))
				// This line is *not* wrong.
				VSendToEngine((coMove == coWHITE) ? "black" : "white");
		} else if (!FSendEdit(s_ivars.aszFen)) {// If I couldn't turn the FEN into
			s_ivars.fError = fTRUE;				//  a valid position, I'm going to
			return fTRUE;						//  set the "Error" flag and just
		}										//  wait until the test period is
		else {									//  over.  The "Error" flag can
			// This line is *not* wrong.
			if ((s_ivars.fColor) && (!s_ivars.fAnalyze)) 
				VSendToEngine((coMove == coWHITE) ? "black" : "white");
		}										//  also be set if the engine
												//  rejects the position (this is
	}											//  handled elsewhere).
	// For those engines which support ping, we send a next ping.
	// Just to be sure that they do not loose to much time because of setting
	// up the board. And to ensure that they are ready
	// Alternatively I could implement here another waiting time, but so far
	// I am unsure if I should do that.
	if (s_ivars.fUseUCI) {
		VSendToEngine("isready");
		s_ivars.mode=modeWAITING;
//		WaitForSingleObject(s_ivars.heventStdinPing, 15000);
	} else {
		if (s_ivars.fPing) {
			VSendToEngine("ping %d", ++s_ivars.cPing);
			s_ivars.mode = modeWAITING;
//			WaitForSingleObject(s_ivars.heventStdinPing, 15000);
		} else {
			VKillTime(s_ivars.tmWaitPos/2);
			s_ivars.mode = modeTESTING;
		}
	}
	//	Figure out when to end.
	//
	s_ivars.tmEnd = TmNow() + s_ivars.tmPerMove;
	s_ivars.tmStartPos = TmNow();
	//
	//	Tell it to go.
	//
	if (s_ivars.fUseUCI) {
		VSendToEngine("go infinite");
	} else {
		if (s_ivars.fAnalyze)
			VSendToEngine("analyze");
		else {
			if (s_ivars.fUseSt) {
				VSendToEngine("st %d", s_ivars.tmPerMove / 1000);
				VSendToEngine("time %d", s_ivars.tmPerMove / 10 * s_ivars.cMultiST);
			} else {
				VSendToEngine("time 60000000");		// 10000 seconds for this.
				VSendToEngine("otim 60000000");      // Same for opponent !
			}
			if (s_ivars.fColor)
				VSendToEngine((coMove == coWHITE) ? "white" : "black");
			VSendToEngine("go");
		}
	}
	s_ivars.tmStart = TmNow();
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	When I am done processing "feature" commands from the engine, I'll do
//	this.  The funtion sends a few simple commands to the engine, then tries
//	to set up the first EPD line.

char const * c_argszNoYes[] = { "No", "Yes" };

BOOL FFeatureTimeout(void)
{
	int	i;

	sprintf(s_ivars.aszOutput,"\n");
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"Engine:         %s\n", s_ivars.aszEngine);
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"Suite:          %s\n", s_ivars.aszSuite);
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"Positions:      %i\n", s_ivars.cPosAbs);
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"Time per move:  %d second%s\n", s_ivars.tmPerMove / 1000,
		(s_ivars.tmPerMove / 1000 == 1) ? "" : "s");
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"Language:       %s (%c%c%c%c%c%c)\n", s_ivars.plang->szLang,
		s_ivars.plang->argbPc[pcPAWN],
		s_ivars.plang->argbPc[pcKNIGHT],
		s_ivars.plang->argbPc[pcBISHOP],
		s_ivars.plang->argbPc[pcROOK],
		s_ivars.plang->argbPc[pcQUEEN],
		s_ivars.plang->argbPc[pcKING]);
	VWriteLogAndOutput();
	if (s_ivars.cSkip) {
		sprintf(s_ivars.aszOutput,"Analysis skip:  %d\n", s_ivars.cSkip);
		VWriteLogAndOutput();
	}
	sprintf(s_ivars.aszOutput,"Engine will use ...\n");
	VWriteLogAndOutput();
	if (!s_ivars.fUseUCI) {
		sprintf(s_ivars.aszOutput,"    \"analyze\":  %s\n", c_argszNoYes[s_ivars.fAnalyze]);
		VWriteLogAndOutput();
		sprintf(s_ivars.aszOutput,"    \"white\" &\n");
		VWriteLogAndOutput();
		sprintf(s_ivars.aszOutput,"      \"black\":  %s\n", c_argszNoYes[s_ivars.fColor]);
		VWriteLogAndOutput();
		sprintf(s_ivars.aszOutput,"    \"ping\":     %s\n", c_argszNoYes[s_ivars.fPing]);
		VWriteLogAndOutput();
		sprintf(s_ivars.aszOutput,"    \"setboard\": %s\n", c_argszNoYes[s_ivars.fSetboard]);
		VWriteLogAndOutput();
		sprintf(s_ivars.aszOutput,"    \"usermove\": %s\n", c_argszNoYes[s_ivars.fUsermove]);
		VWriteLogAndOutput();
	} else {
		for (i=0;i<s_ivars.UCIoptions;i++) {
			if (s_ivars.UCIoptionstate[i] & 2) {
				sprintf(s_ivars.aszOutput,"setoption name %s value %s\n",s_ivars.UCIoption[i],s_ivars.UCIvalue[i]);
				if (!s_ivars.fFindInfo) {
					VSendToEngine(s_ivars.aszOutput);
				}
				VWriteLogAndOutput();
			}
		}
	}
	if (s_ivars.cIPTime) {
		if (s_ivars.cIPTime==1) 
			sprintf(s_ivars.aszOutput,"    \"time in\":  centiseconds\n");
		else if (s_ivars.cIPTime==2) sprintf(s_ivars.aszOutput,"    \"time in\":  seconds\n");
		else if (s_ivars.cIPTime==3) sprintf(s_ivars.aszOutput,"    \"time in\":  tenth of seconds\n");
		else sprintf(s_ivars.aszOutput,"    \"time in\":  milliseconds\n");
		VWriteLogAndOutput();
	}
	if ((!s_ivars.fPing) && (!s_ivars.fUseSt)) {
		sprintf(s_ivars.aszOutput,"Test delay:     %d second%s\n", s_ivars.tmWaitInit / 1000,
			(s_ivars.tmWaitInit / 1000 == 1) ? "" : "s");
		VWriteLogAndOutput();
	}
	if (s_ivars.fUseSt) {
		sprintf(s_ivars.aszOutput,"Try \"st\":       %s\n", c_argszNoYes[s_ivars.fUseSt]);
		VWriteLogAndOutput();
	}
	if (s_ivars.fFindInfo)
		return fFALSE;
	VWriteEmpty();
	s_ivars.aszOutput[0]=0;
	for (i = 0; i < cbLINE - 1; i++)
		strcat(s_ivars.aszOutput,"-");
	VWriteLogAndOutput();
	VWriteEmpty();
	VWriteEmpty();
	if (!s_ivars.fUseUCI) VPrepEngine();
	return FNextTest();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Breaks "done=1", etc., into two parts.

BOOL FBreakFeature(char * szCmd, char * szName, int * piValue)
{
	for (;;) {
		if ((*szCmd == '\0') || (*szCmd == '='))
			break;
		*szName++ = *szCmd++;
	}
	*szName = '\0';
	if (*szCmd++ != '=')
		return fFALSE;
	if (*szCmd == '"') {    // I'm going to handle string features by
		*piValue = 0;		//  interpreting strings as zero.  This is fine
		return fTRUE;		//  for now but may need changing later.
	}
	if (!isdigit(*szCmd))
		return fFALSE;
	*piValue = atoi(szCmd);
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Engine commands.

//	These functions are called in reaction to command received from the
//	engine.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	"feature" command.  All I care about are "feature done=0", which tells the
//	engine to ignore the two-second timeout, and "feature done=1", which is
//	treated as a feature timeout.

BOOL FCmdFeature(char * sz, char * rgsz[], int csz)
{
	int	isz;
	DWORD i;

	for (isz = 1; isz < csz; isz++) {
		char	aszName[256];
		int	iValue;

		if (!FBreakFeature(rgsz[isz], aszName, &iValue))
			return fTRUE;	// Ignore that which I don't understand.
		if ((!strcmp(aszName, "done")) && (!s_ivars.fIgnoreDone)) {
			if (iValue == 0) {
				if (s_ivars.mode == modeFEATURE_TIMEOUT)
					s_ivars.mode = modeFEATURE_NOTIMEOUT;
			} else {
				if ((s_ivars.mode == modeFEATURE_NOTIMEOUT) ||
					(s_ivars.mode == modeFEATURE_TIMEOUT))
					if (!FFeatureTimeout())
						return fFALSE;
			}
		} else if (!strcmp(aszName, "ping"))
			s_ivars.fPing = (iValue) ? fTRUE : fFALSE;
		else if (!strcmp(aszName, "setboard"))
			s_ivars.fSetboard = (iValue) ? fTRUE : fFALSE;
		else if (!strcmp(aszName, "myname"))
		{
			for (i=1;i<strlen(rgsz[isz]);i++)
			{
				if (rgsz[isz][i]=='"')
				{
					strcpy(s_ivars.aszEngine,&rgsz[isz][i+1]);
					s_ivars.aszEngine[strlen(s_ivars.aszEngine)-1]=0;
					break;
				}
			}
		}
		else if (!strcmp(aszName, "analyze")) {
			s_ivars.fAnalyze = (iValue) ? fTRUE : fFALSE;
			if (s_ivars.fUseSt)		// "-t" switch turns off analyze.
				s_ivars.fAnalyze = fFALSE;
		} else if (!strcmp(aszName, "colors"))
			s_ivars.fColor = (iValue) ? fTRUE : fFALSE;
		else if (!strcmp(aszName, "usermove"))
			s_ivars.fUsermove = (iValue) ? fTRUE : fFALSE;
	}
	return fTRUE;
}

// UCI-Engine has sent it's ID... let's take a look wether it is
// the name or the author

BOOL FCmdUciId(char * sz, char * rgsz[], int csz)
{
	long i;
	if (!strcmp("name",rgsz[1])) {
		strcpy(s_ivars.aszEngine,"\0");
		for (i=2;i<csz;i++) {
			if (i>2) strcat(s_ivars.aszEngine," ");
			strcat(s_ivars.aszEngine,rgsz[i]);
		}
	}
	return fTRUE;
}

BOOL FCmdUciOk(char * sz, char * rgsz[], int csz)
{
	if (!FFeatureTimeout())
		return fFALSE;
	return fTRUE;
}

BOOL FCmdOption(char * sz, char * rgsz[], int csz)
{
	char UciOption[256];
	int i,k;
	bool found;
	if (!strcmp("name",rgsz[1])) {
		strcpy(UciOption,rgsz[2]);
		k=3;
		while (strcmp(rgsz[k],"type")) {
			strcat(UciOption," ");
			strcat(UciOption,rgsz[k]);
			k++;
		}
		found=false;
		for (i=0;i<s_ivars.UCIoptions;i++)
		{
			if (!strcmp(UciOption,s_ivars.UCIoption[i])) {
				found=true;
				s_ivars.UCIoptionstate[i]|=2;
			}
		}
		if (!found) {
			strcpy(s_ivars.UCIoption[s_ivars.UCIoptions],UciOption);
			found=false;
			for (i=k;i<csz;i++) {
				if (!strcmp(rgsz[i],"default")) {
					strcpy(s_ivars.UCIvalue[s_ivars.UCIoptions],rgsz[i+1]);
					found=true;
				}
			}
			s_ivars.UCIoptionstate[s_ivars.UCIoptions]=2;
			if (found) s_ivars.UCIoptions++;
		}
	}
	return fTRUE;
}

//	A "pong" indicates that the engine is listening to me and is ready to
//	start analyzing the current position, so I change the mode to TESTING.

//	In some cases I need to keep track of time taken by the program.  I start
//	the clock when I tell the program to go and think, but I also send a ping,
//	and if I get a pong back, I reset the clock.

BOOL FCmdPong(char * sz, char * rgsz[], int csz)
{
	if (csz == 0)
		return fTRUE;
	if (atoi(rgsz[1]) == s_ivars.cPing) {
		s_ivars.mode = modeTESTING;
		s_ivars.tmStart = TmNow();
		s_ivars.tmEnd = s_ivars.tmStart + s_ivars.tmPerMove;
		SetEvent(s_ivars.heventStdinPing);
	}
	return fTRUE;
}

BOOL FCmdReadyOk(char * sz, char * rgsz[], int csz)
{
	s_ivars.mode = modeTESTING;
	s_ivars.tmStart = TmNow();
	s_ivars.tmEnd = s_ivars.tmStart + s_ivars.tmPerMove;
	SetEvent(s_ivars.heventStdinPing);
	return fTRUE;
}

void VAnalysisLine(char * rgsz[], int csz);

void VEngineMoved(char * sz)
{
	char	asz[256];
	char *	argsz[256];
	int	ibSecond;
	int	csz;

	s_ivars.tmEnd = TmNow();
	if (!s_ivars.fUseUCI) {
		sprintf(asz, "-1 0 %lu 0 %s", (s_ivars.tmEnd - s_ivars.tmStart) / 10, sz);
	} else {
		sprintf(asz, "info time %lu nodes 0 score cp 0 depth -1 pv %s",(s_ivars.tmEnd - s_ivars.tmStart),sz);
	}
	csz = CszVectorizeCmd(asz, argsz, &ibSecond);
	VAnalysisLine(argsz, csz);
}

BOOL FCmdMove(char * sz, char * rgsz[], int csz)
{
	if (csz != 2)
		return fTRUE;
	if (s_ivars.mode != modeTESTING)
		return fTRUE;
	VEngineMoved(rgsz[1]);
	return fTRUE;
}

//	This will be received if I send a bogus FEN to the engine.  I'm not going
//	to look at the error message at all.  It is supposed to be
//	"Illegal position", but I am not going to count on that.

BOOL FCmdTellusererror(char * sz, char * rgsz[], int csz)
{
	if (s_ivars.mode != modeTESTING)
		return fTRUE;
	s_ivars.fError = fTRUE;
	return fTRUE;
}

//	Engine command table.

CMD const c_argcmdEngine[] = {
	"feature",			FCmdFeature,
	"pong",				FCmdPong,
	"move",				FCmdMove,
	"tellusererror",	FCmdTellusererror,
	"id",				FCmdUciId,
	"option",			FCmdOption,
	"uciok",            FCmdUciOk,
	"readyok",			FCmdReadyOk,
	NULL,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

BOOL FIsInteger(char * sz, BOOL * pfNonBlankTrailer)
{
	if ((*sz == '-') || (*sz == '+'))
		sz++;
	if (sz[strlen(sz)-1]=='d')	// Gaviota - strip of last "d"
		sz[strlen(sz)-1]='\0';
	if (sz[strlen(sz)-1]=='!')	// Horizon - strip of last "!"
		sz[strlen(sz)-1]='\0';
	if (sz[strlen(sz)-1]=='?')	// Horizon - strip of last "?"
		sz[strlen(sz)-1]='\0';
	if (!isdigit(*sz++))	// Must have at least one digit.
		return fFALSE;
	while (isdigit(*sz))
		sz++;
	if (*sz != '\0') {		// I'm going to allow one non-blank trailer.
		*pfNonBlankTrailer = fTRUE;
		sz++;
	} else
		*pfNonBlankTrailer = fFALSE;
	return (*sz == '\0');
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This checks to see if "sz" is a correct answer to the current test
//	position.

BOOL FCorrectAnswer(char * sz)
{
	int	i;

	while (isdigit(*sz))
		sz++;
	while (*sz == '.')
		sz++;
	if (*sz == '(')
		sz++;							// Monik
	if (*sz == '+')
		sz++;							// KnightDreamer
	if (*sz == '-')
		sz++;							// KnightDreamer
	if ((strlen(sz)>4) && (sz[strlen(sz)-3]=='('))
		sz[strlen(sz)-3]=0;				// Freyr
	if ((strlen(sz)>3) && ((!strncmp(&sz[strlen(sz)-2],"FH",2)) || (!strncmp(&sz[strlen(sz)-2],"FL",2))))
		sz[strlen(sz)-2]=0;				// La Petite
	if (s_ivars.aszAnswer[0] != '\0') {
		for (i = 0; s_ivars.aszAnswer[i] != '\0';) {
			if (FCheckAnswer(s_ivars.aszFen, sz, s_ivars.aszAnswer + i,
				s_ivars.plang, s_ivars.plangEnglish))
				return fTRUE;
			i += strlen(s_ivars.aszAnswer + i) + 1;
		}
		return fFALSE;
	}
	if (s_ivars.aszAvoid[0] != '\0') {
		for (i = 0; s_ivars.aszAvoid[i] != '\0';) {
			if (FCheckAnswer(s_ivars.aszFen, sz, s_ivars.aszAvoid + i,
				s_ivars.plang, s_ivars.plangEnglish))
				return fFALSE;
			i += strlen(s_ivars.aszAvoid + i) + 1;
		}
		return fTRUE;
	}
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

BOOL FIsMove(char * sz)
{
	if ((strlen(sz) == 4) && (sz[0]=='[') && (sz[3]==']'))
		return fFALSE;							// King of Kings
	if (!strcmp(sz,"MAT")) return fFALSE;		// Capture
	if (!strcmp(sz,"EN")) return fFALSE;		// Capture
	if (!strcmp(sz,"COUPS")) return fFALSE;		// Capture
	if (!strcmp(sz,"MAT!")) return fFALSE;		// Capture
	if (!strncmp(sz,"a8a8",4)) return fFALSE;   // Skaki
	while (isdigit(*sz))
		sz++;
	while (*sz == '.')
		sz++;
	if (*sz == '(')
		sz++;									// Monik-Bug
	if (*sz == ')')
		sz++;									// Hagrid
	while (*sz == '+')
		sz++;									// KnightDreamer
	while (*sz == '-')
		sz++;									// KnightDreamer
	if ((*sz != '\0') && (*sz != '/'))          // '/' BSC
		return fTRUE;
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This function handles a line of analysis nonsense sent by the engine.  At
//	the start of the function I don't know for sure that it's actually a line
//	of analysis.  I do some dummy checking to see if it starts with four
//	integers, and if so I assume it's an analysis line.

void VAnalysisLine(char * rgsz[], int csz)
{
	int		plyDepth;
	int		valScore;
	TM		tm;
	long	nodes;
	int		i;
	char	aszResult[16],hstring[60];
	char	uciline[1024];
	int		cb;
	int		cbStats;
	BOOL	fTrailer;
	int	cSkip;
	
	if (!s_ivars.fUseUCI) {
		// Do we need that ?
		if ((csz == 1) && ((!strcmp(rgsz[0], "++")) ||
			(!strcmp(rgsz[0], "--")))) {
			sprintf(s_ivars.aszOutput,"%40s\n", rgsz[0]);
			VWriteLogAndOutput();
		}
		strcpy(s_ivars.aszOutput,"Analysis tokens:");
		hstring[0]=0;
		if (s_ivars.fDump) {
			for (i=0;i<__min(csz,6);i++)
			{
				sprintf(hstring,"%d/%s ",i,rgsz[i]);
				strcat(s_ivars.aszOutput,hstring);
			}
			strcat(s_ivars.aszOutput,"\n");
			VWriteLogAndOutput();
		}
		if (csz < 4)
		{
			if (s_ivars.fDump) {
				strcpy(s_ivars.aszOutput,"Analysis: Not enough parameters\n");
				VWriteLogAndOutput();
			}
			return;
		}
		//
		//	An analysis line starts with four integers.  If the first one has a
		//	trailing non-blank, that's okay but it puts us into Gnuchess mode,
		//	in which case the third parameter is a value in seconds, not in 1/100
		//	of a second.
		//
		for (i = 0; i < 4; i++) {
			BOOL	f;
			if (!FIsInteger(rgsz[i], &f))
			{
				if ((i==1) && (!strncmp(rgsz[i],"Mat",3)))
					f=false;
				else 
				{
					if (s_ivars.fDump) {
						sprintf(s_ivars.aszOutput,"Analysis: parameter %d is no integer !\n",i);
						VWriteLogAndOutput();
					}
					return;
				}
			}
			if (i == 0)
				fTrailer = f;
			else if (f)
			{
					if (s_ivars.fDump) {
						sprintf(s_ivars.aszOutput,"Analysis: in the 4 first parameters is one no integer !\n",i);
						VWriteLogAndOutput();
					}
				return;
			}
		}
	} else {
		// Let's take a look if it is an UCI PV
		if (!strcmp(rgsz[0],"info")) {
			char ucidepth[50];
			char uciscore[50];
			char ucinodes[50];
			char ucitime[50];
			char ucipv[1024];
			bool uciwrong,pvstarted;
			int j;
			strcpy(ucipv,"");
			strcpy(ucidepth,"0");
			strcpy(uciscore,"0");
			strcpy(ucinodes,"0");
			strcpy(ucitime,"0");
			i=1;
			uciwrong=false;
			pvstarted=false;
			while ((i<csz) && (!uciwrong )) {
				if (!strcmp(rgsz[i],"pv")) {
					pvstarted=true;
				} else {
					if (!strcmp(rgsz[i],"currmove")) {
						uciwrong=true;
					} else {
						if (!strcmp(rgsz[i],"currmovenumber")) {
							uciwrong=true;
						} else {
							if (!strcmp(rgsz[i],"currline")) {
								uciwrong=true;
							} else {
								if (!strcmp(rgsz[i],"refutation")) {
									uciwrong=true;
								} else {
									if (!strcmp(rgsz[i],"depth")) {
										pvstarted=false;
										strcpy(ucidepth,rgsz[i+1]);
#if uciDEBUG
										sprintf(s_ivars.aszOutput,"Depth: %s\n",ucidepth);
										VWriteLogAndOutput();
#endif
										i++;
									} else {
										if (!strcmp(rgsz[i],"seldepth")) {
											pvstarted=false;
											i++;
										} else {
											if (!strcmp(rgsz[i],"time")) {
												pvstarted=false;
												strcpy(ucitime,rgsz[i+1]);
												i++;
#if uciDEBUG
												sprintf(s_ivars.aszOutput,"Time: %s\n",ucitime);
												VWriteLogAndOutput();
#endif
											} else {
												if (!strcmp(rgsz[i],"nodes")) {
													pvstarted=false;
													strcpy(ucinodes,rgsz[i+1]);
													i++;
#if uciDEBUG
													sprintf(s_ivars.aszOutput,"Nodes: %s\n",ucinodes);
													VWriteLogAndOutput();
#endif
												} else {
													if (!strcmp(rgsz[i],"multipv")) {
														pvstarted=false;
														i++;
													} else {
														if (!strcmp(rgsz[i],"score")) {
															pvstarted=false;
															if (!strcmp(rgsz[i+1],"mate")) {
																j=atoi(rgsz[i+2]);
																if (j>0) {
																	sprintf(uciscore,"%i",32767-(j*2));
																} else {
																	sprintf(uciscore,"%i",-32768+(j*2));
																}
															} else {
																strcpy(uciscore,rgsz[i+2]);
															}
															i+=2;
															/*
															if ((!strcmp(rgsz[i+1],"lowerbound")) || (!strcmp(rgsz[i+1],"upperbound"))) {
																i++;
															}
															*/
#if uciDEBUG
															sprintf(s_ivars.aszOutput,"Score: %s\n",uciscore);
															VWriteLogAndOutput();
#endif
														} else {
															if (!strcmp(rgsz[i],"hashfull")) {
																pvstarted=false;
																i++;
															} else {
																if (!strcmp(rgsz[i],"nps")) {
																	pvstarted=false;
																	i++;
																} else {
																	if (!strcmp(rgsz[i],"tbhits")) {
																		pvstarted=false;
																		i++;
																	} else {
																		if (!strcmp(rgsz[i],"cpuload")) {
																			pvstarted=false;
																			i++;
																		} else {
																			if (!strcmp(rgsz[i],"string")) {
																				uciwrong=true;
																			} else {
																				if (pvstarted) {
																					if (strlen(ucipv)>1) {
																						strcat(ucipv," ");
																					}
																					strcat(ucipv,rgsz[i]);
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				i++;
			}
			if ((uciwrong) || (strlen(ucipv)<2)) return;
			sprintf(uciline,"%s %s %s %s %s",ucidepth,uciscore,ucitime,ucinodes,ucipv);
			// just for debug
#if uciDEBUG
			strcpy(s_ivars.aszOutput,uciline);
			strcat(s_ivars.aszOutput,"\n");
			VWriteLogAndOutput();
#endif
			csz=CszVectorizeCmd(uciline,rgsz,&i);
		}
		else {
			return;
		}
	}
	//	It had four integers, break them out.
	//
	plyDepth = atoi(rgsz[0]);
	cSkip = (plyDepth < 0) ? 0 : s_ivars.cSkip;
	valScore = atoi(rgsz[1]);
	if (s_ivars.cIPTime != 4) tm = atol(rgsz[2]) * 10;
	else tm = atol(rgsz[2]);
	if (s_ivars.cIPTime == 3) tm *= 10;
	else if (((fTrailer) || (s_ivars.cIPTime==2)) 
		&& (s_ivars.cIPTime != 1) && (s_ivars.cIPTime != 4))
		tm *= 100;
	if (s_ivars.fInternalTime) tm = TmNow() - s_ivars.tmStartPos;
	if (tm > s_ivars.tmPerMove) return;
	nodes = atol(rgsz[3]);
	//
	//	Check to see if the answer is correct.  If so, record the time, if
	//	not, clear the time.
	//
	//	This loop is a little gross, because I might decide that the first
	//	"move" or two aren't really moves (they might be move numbers or some
	//	symbol that indicates that black is on move), so I'll skip them.
	//
	for (i = 4 + cSkip;; i++)
		if (i >= csz) {
			// Diep - fix
			if (valScore == 49999)
			{
				s_ivars.fCorrectAnswer = fTRUE;
				strcpy(aszResult,"yes");
				if (s_ivars.tmFoundIn==-1)
				{
					s_ivars.tmFoundIn=tm;
					s_ivars.apsInfo[s_ivars.cPosCounter].cSolveDepth=plyDepth;
				}
				break;
			}
			// End Diep - fix
lblWrong:	s_ivars.fCorrectAnswer = fFALSE;
			strcpy(aszResult, "no");
			s_ivars.tmFoundIn = -1;
			break;
		} else 
			{
				// Aristarch - fix
				if (!strncmp(rgsz[i],"TBHits=",7)) i++;		// TBHits=xxxxx
				// BSC - fix
				if ((rgsz[i][0]=='(') && (rgsz[i][strlen(rgsz[i])-1]==')')) i++;	// (g1f3)
				// Armageddon - fix
				if (rgsz[i][0]==',') i++;											// ,
				if ((rgsz[i][0]=='(') && (rgsz[i+1][strlen(rgsz[i+1])-1]==',')) i+=2; // (g1f3 b8c6),
				if ((rgsz[i][0]=='(') && (rgsz[i+1][strlen(rgsz[i+1])-1]==')')) i+=2; // (g1f3 b8c6)
				// usual move check
				if (FIsMove(rgsz[i])) {
				if (!FCorrectAnswer(rgsz[i]))
					goto lblWrong;
				s_ivars.fCorrectAnswer = fTRUE;
				strcpy(aszResult, "yes");
				if (s_ivars.tmFoundIn == -1) {
					s_ivars.tmFoundIn = tm;
					s_ivars.apsInfo[s_ivars.cPosCounter].cSolveDepth=plyDepth;
				}
				if (s_ivars.fStopPly) {
					if ((plyDepth-s_ivars.apsInfo[s_ivars.cPosCounter].cSolveDepth)>=s_ivars.cPlys) {
						s_ivars.fStopAnalysis=fTRUE;
					}
				}
				if (s_ivars.fStopScore) {
					if (valScore>=s_ivars.cScore) {
						s_ivars.fStopAnalysis=fTRUE;
					}
				}
				break;
			}
		}
	//
	//	Output the line in a somewhat prettied up format.
	//
	if (plyDepth < 0)
		cb = cbStats = sprintf(s_ivars.aszOutput,"%-3s ??? %9ld (moved)           ???",
			aszResult, tm);
	else {
		if (s_ivars.fDump) {
			cb = cbStats = sprintf(s_ivars.aszOutput,"%-3s %3d %9ld:%d %+8d %12ld",
				aszResult, plyDepth, tm, TmNow() - s_ivars.tmStartPos,valScore, nodes);
			s_ivars.apsInfo[s_ivars.cPosCounter].cLastDepth=plyDepth;
			s_ivars.apsInfo[s_ivars.cPosCounter].cLastEval=valScore;
			if (csz>4+cSkip) s_ivars.apsInfo[s_ivars.cPosCounter].aszLastPV[0]='\0';
		}
		else
		{
			cb = cbStats = sprintf(s_ivars.aszOutput,"%-3s %3d %9ld %+8d %12ld",
				aszResult, plyDepth, tm,valScore, nodes);
			s_ivars.apsInfo[s_ivars.cPosCounter].cLastDepth=plyDepth;
			s_ivars.apsInfo[s_ivars.cPosCounter].cLastEval=valScore;
			if (csz>4+cSkip) s_ivars.apsInfo[s_ivars.cPosCounter].aszLastPV[0]='\0';
		}
	}
	for (i = 4 + cSkip; i < csz; i++) {
		int	cbCur = strlen(rgsz[i]) + 1;

		if (strlen(s_ivars.apsInfo[s_ivars.cPosCounter].aszLastPV)+strlen(rgsz[i])+1<512) {
			strcat(s_ivars.apsInfo[s_ivars.cPosCounter].aszLastPV," ");
			strcat(s_ivars.apsInfo[s_ivars.cPosCounter].aszLastPV,rgsz[i]);
		}
		if (cb + cbCur > cbLINE - 1) {
			VWriteLogAndOutput();
			VWriteEmpty();
			s_ivars.aszOutput[0]=0;
			for (cb = 0; cb < cbStats; cb++)
				strcat(s_ivars.aszOutput," ");
		}
		strcat(s_ivars.aszOutput," ");
		strcat(s_ivars.aszOutput,rgsz[i]);
		cb += cbCur;
	}
	VWriteLogAndOutput();
	VWriteEmpty();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	uHOUR_FROM_MILLI(tm)	(unsigned)((TM)(tm) / 3600000)
#define	uMIN_FROM_MILLI(tm)		(unsigned)(((TM)(tm) / 60000) % 60)
#define	uSEC_FROM_MILLI(tm)		(unsigned)(((TM)(tm) / 1000) % 60)
#define	uMILLI_FROM_MILLI(tm)	(unsigned)((TM)(tm) % 1000)

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called at the end, and it just dumps out the number solved, and
//	a breakdown of how many problems were solved in less than N seconds.

void VDumpResults(void)
{
	int	isolHi;
	int i,j,k,r,cb,cbCur,csz;
	unsigned int l;
	DWORD dwa;
	char aszAddText[20];
	char * argsz[256];
	
	sprintf (s_ivars.aszOutput,"Position results:\n\n");
	VWriteLogAndOutput();
	sprintf (s_ivars.aszOutput,"PosNr solved ply      time     last pv\n");
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"----- ------ --- ------------- ");
	for (cb=strlen(s_ivars.aszOutput);cb<cbLINE - 1;cb++)
		strcat(s_ivars.aszOutput,"-");
	VWriteLogAndOutput();
	VWriteEmpty();
	for (i = 0; i < s_ivars.cPosCounter;i++) {
		if (s_ivars.apsInfo[i].cSolveTime<=s_ivars.tmPerMove)
			sprintf(s_ivars.aszOutput,"%5d    yes %3d [%02d:%02d:%02d,%02d] ",i+1,
				s_ivars.apsInfo[i].cSolveDepth,
				uHOUR_FROM_MILLI(s_ivars.apsInfo[i].cSolveTime),
				uMIN_FROM_MILLI(s_ivars.apsInfo[i].cSolveTime),
				uSEC_FROM_MILLI(s_ivars.apsInfo[i].cSolveTime),
				uMILLI_FROM_MILLI(s_ivars.apsInfo[i].cSolveTime)/10);
		else sprintf(s_ivars.aszOutput,"%5d     no     [--:--:--,--] ",i+1);
		cb=strlen(s_ivars.aszOutput);
		if (s_ivars.apsInfo[i].cLastDepth>0) {
			sprintf(aszAddText,"[%d] %+.2f ",s_ivars.apsInfo[i].cLastDepth,
					(float) s_ivars.apsInfo[i].cLastEval/100.0);
		}
		else aszAddText[0]=0;
		cbCur=strlen(aszAddText);
		if (cbCur>0) strcat(s_ivars.aszOutput,aszAddText);
		csz=CszVectorizeCmd(s_ivars.apsInfo[i].aszLastPV,argsz,&r);
		for (j=0; j<csz; j++) {
			if (cb+cbCur+strlen(argsz[j])<cbLINE) {
				cbCur+=strlen(argsz[j])+1;
				strcat(s_ivars.aszOutput,argsz[j]);
				strcat(s_ivars.aszOutput," ");
			}
			else
			{
				VWriteLogAndOutput();
				VWriteEmpty();
				s_ivars.aszOutput[0]=0;
				for (k=0; k<cb; k++)
					strcat(s_ivars.aszOutput," ");
				strcat(s_ivars.aszOutput,argsz[j]);
				strcat(s_ivars.aszOutput," ");
				cbCur=strlen(argsz[j])+1;
			}
		}
		VWriteLogAndOutput();
		VWriteEmpty();
	}
	s_ivars.aszOutput[0]=0;
	for (i = 0; i < cbLINE - 1; i++)
		strcat(s_ivars.aszOutput,"-");
	VWriteLogAndOutput();
	VWriteEmpty();
	VWriteEmpty();
	for (isolHi = isolMAX - 1; isolHi >= 0; isolHi--)
		if (s_ivars.argsol[isolHi])
			break;
	sprintf(s_ivars.aszOutput,"Results:\n\n");
	VWriteLogAndOutput();
	if (s_ivars.cSolved) {
		if (isolHi < 0) {
			sprintf(s_ivars.aszOutput,"No problems solved in < %d seconds\n", isolMAX);
			VWriteLogAndOutput();
		}
		else {
			int	cTotal = 0;

			sprintf(s_ivars.aszOutput,"<=Sec Solved Total  PosNr\n");
			VWriteLogAndOutput();
			strcpy(s_ivars.aszOutput,"----- ------ ------ ");
			for (cb=strlen(s_ivars.aszOutput);cb<cbLINE - 1;cb++)
				strcat(s_ivars.aszOutput,"-");
			VWriteLogAndOutput();
			VWriteEmpty();
			for (i = 0; i <= isolHi; i++) {
				if (s_ivars.argsol[i]) {
					cTotal += s_ivars.argsol[i];
					k=sprintf(s_ivars.aszOutput,"%5d %6d %6d", i + 1, s_ivars.argsol[i], cTotal);
					cb=strlen(s_ivars.aszOutput);
					cbCur=0;
					for (j=0;j<s_ivars.cPosCounter;j++) {
						if (((s_ivars.apsInfo[j].cSolveTime<=(TM)(i+1)*1000)
							&& (s_ivars.apsInfo[j].cSolveTime>(TM)i*1000)) 
							|| ((i==0) && (s_ivars.apsInfo[j].cSolveTime==0))) {
							if (s_ivars.cPosCounter<10) {
								k=sprintf(aszAddText,"%2d",j+1);
							}
							else if (s_ivars.cPosCounter<100) {
								k=sprintf(aszAddText,"%3d",j+1);
							}
							else if (s_ivars.cPosCounter<1000) {
								k=sprintf(aszAddText,"%4d",j+1);
							}
							else k=sprintf(aszAddText,"%5d",j+1);
							if (cb+cbCur+strlen(aszAddText)<cbLINE) {
								cbCur+=strlen(aszAddText);
								strcat(s_ivars.aszOutput,aszAddText);
							}
							else {
								VWriteLogAndOutput();
								VWriteEmpty();
								cbCur=strlen(aszAddText);
								s_ivars.aszOutput[0]=0;
								for (k=0;k<cb;k++)
									strcat(s_ivars.aszOutput," ");
								strcat(s_ivars.aszOutput,aszAddText);
							}
						}
					}
					VWriteLogAndOutput();
					VWriteEmpty();
				}
			}
			strcpy(s_ivars.aszOutput,"Failure");
			for (i=strlen(s_ivars.aszOutput);i<cb;i++)
				strcat(s_ivars.aszOutput," ");
			cbCur=0;
			for (i=0;i<s_ivars.cPosCounter;i++) {
				if (s_ivars.apsInfo[i].cSolveTime>=s_ivars.tmPerMove) {
					if (s_ivars.cPosCounter<10) {
						k=sprintf(aszAddText,"%2d",i+1);
					}
					else if (s_ivars.cPosCounter<100) {
						k=sprintf(aszAddText,"%3d",i+1);
					}
					else if (s_ivars.cPosCounter<1000) {
						k=sprintf(aszAddText,"%4d",i+1);
					}
					else k=sprintf(aszAddText,"%5d",i+1);
					if (cb+cbCur+strlen(aszAddText)<cbLINE) {
						cbCur+=strlen(aszAddText);
						strcat(s_ivars.aszOutput,aszAddText);
					}
					else {
						VWriteLogAndOutput();
						VWriteEmpty();
						cbCur=strlen(aszAddText);
						s_ivars.aszOutput[0]=0;
						for (k=0;k<cb;k++)
							strcat(s_ivars.aszOutput," ");
						strcat(s_ivars.aszOutput,aszAddText);
					}
				}
			}
			VWriteLogAndOutput();
			VWriteEmpty();
		}
		VWriteEmpty();
	}
	sprintf(s_ivars.aszOutput,"%d problem%s solved.\n", s_ivars.cSolved,
		(s_ivars.cSolved == 1) ? "" : "s");
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"%d problem%s unsolved.\n", s_ivars.cFailed,
		(s_ivars.cFailed == 1) ? "" : "s");
	VWriteLogAndOutput();
	if (s_ivars.cError) {
		sprintf(s_ivars.aszOutput,"%d error%s found!\n",
			s_ivars.cError, (s_ivars.cError == 1) ? "" : "s");
		VWriteLogAndOutput();
	}
	VWriteEmpty();
	if (s_ivars.fDBon)
	{
		FILE * OldFile;
		char aszOutput[5000];
		DWORD solvetime,solved;
		solved=0;
		solvetime=0;
		dwa=GetFileAttributes(s_ivars.aszDBFile);
		if (dwa==(DWORD)(-1))
		{
			s_ivars.DBDatei.open(s_ivars.aszDBFile,ios::trunc | ios::out | ios::in);
			s_ivars.DBDatei.seekp(0,ios::beg);
			sprintf(aszOutput,"\"%s\";;;;\"%s\";\n",s_ivars.aszSuite,s_ivars.aszEngine);
			s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
			s_ivars.DBDatei.seekp(0,ios::end);
			strcpy(aszOutput,"\"Nr.\";\"Name\";\"FEN\";\"Keymove\";\"time\";\"depth\"\n");
			s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
			s_ivars.DBDatei.seekp(0,ios::end);
			for (i = 0; i < s_ivars.cPosCounter;i++) 
			{
				sprintf(aszOutput,"\"%i\";\"%s\";\"%s\";\"%s\";",i+1,s_ivars.aszName[i],s_ivars.aszFEN[i],s_ivars.aszSol[i]);
				if (s_ivars.apsInfo[i].cSolveTime<=s_ivars.tmPerMove)
				{
					solved++;
					solvetime+=s_ivars.apsInfo[i].cSolveTime/1000;
					sprintf(aszOutput,"%s\"%i\";\"%i\"\n",aszOutput,s_ivars.apsInfo[i].cSolveTime/1000,s_ivars.apsInfo[i].cSolveDepth);
				}
				else
				{
					solvetime+=s_ivars.tmPerMove/1000;
					sprintf(aszOutput,"%s\"9999\";\"%i\"\n",aszOutput,s_ivars.apsInfo[i].cLastDepth);
				}
				s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
				s_ivars.DBDatei.seekp(0,ios::end);
			}
			sprintf(aszOutput,";;;\"Solved:\";\"%i\";\n",solved);
			s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
			s_ivars.DBDatei.seekp(0,ios::end);
			sprintf(aszOutput,";;;\"Solve-Time:\";\"%i\";\n",solvetime);
			s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
			s_ivars.DBDatei.seekp(0,ios::end);
			s_ivars.DBDatei.close();
		}
		else
		{
			strcpy(aszOutput,s_ivars.aszDBFile);
			strcat(aszOutput,".bak");
			dwa=GetFileAttributes(aszOutput);
			if (!(dwa==(DWORD)(-1)))
			{
				remove(aszOutput);
			}
			rename(s_ivars.aszDBFile,aszOutput);
			if( (OldFile = fopen( aszOutput, "r" )) != NULL )
			{
				fgets(aszOutput,5000,OldFile);
				j=0;
				for (l=0;l<strlen(aszOutput);l++) {
					if (aszOutput[l]==';') {
						j++;
					}
				}
				if (j>250) {
					fclose(OldFile);
					strcpy(aszOutput,s_ivars.aszDBFile);
					strcat(aszOutput,".001");
					dwa=GetFileAttributes(aszOutput);
					if (!(dwa==(DWORD)(-1)))
					{
						remove(aszOutput);
					}
					rename(s_ivars.aszDBFile,aszOutput);
					dwa=GetFileAttributes(s_ivars.aszDBFile);
					if (dwa==(DWORD)(-1))
					{
						s_ivars.DBDatei.open(s_ivars.aszDBFile,ios::trunc | ios::out | ios::in);
						s_ivars.DBDatei.seekp(0,ios::beg);
						sprintf(aszOutput,"\"%s\";;;;\"%s\";\n",s_ivars.aszSuite,s_ivars.aszEngine);
						s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
						s_ivars.DBDatei.seekp(0,ios::end);
						strcpy(aszOutput,"\"Nr.\";\"Name\";\"FEN\";\"Keymove\";\"time\";\"depth\"\n");
						s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
						s_ivars.DBDatei.seekp(0,ios::end);
						for (i = 0; i < s_ivars.cPosCounter;i++) 
						{
							sprintf(aszOutput,"\"%i\";\"%s\";\"%s\";\"%s\";",i+1,s_ivars.aszName[i],s_ivars.aszFEN[i],s_ivars.aszSol[i]);
							if (s_ivars.apsInfo[i].cSolveTime<=s_ivars.tmPerMove)
							{
								solved++;
								solvetime+=s_ivars.apsInfo[i].cSolveTime/1000;
								sprintf(aszOutput,"%s\"%i\";\"%i\"\n",aszOutput,s_ivars.apsInfo[i].cSolveTime/1000,s_ivars.apsInfo[i].cSolveDepth);
							}
							else
							{
								solvetime+=s_ivars.tmPerMove/1000;
								sprintf(aszOutput,"%s\"9999\";\"%i\"\n",aszOutput,s_ivars.apsInfo[i].cLastDepth);
							}
							s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
							s_ivars.DBDatei.seekp(0,ios::end);
						}
						sprintf(aszOutput,";;;\"Solved:\";\"%i\";\n",solved);
						s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
						s_ivars.DBDatei.seekp(0,ios::end);
						sprintf(aszOutput,";;;\"Solve-Time:\";\"%i\";\n",solvetime);
						s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
						s_ivars.DBDatei.seekp(0,ios::end);
						s_ivars.DBDatei.close();
					}
				} else {
					VStrip(aszOutput);
					sprintf(aszOutput,"%s;\"%s\";\n",aszOutput,s_ivars.aszEngine);
					s_ivars.DBDatei.open(s_ivars.aszDBFile,ios::trunc | ios::out | ios::in);
					s_ivars.DBDatei.seekp(0,ios::beg);
					s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
					s_ivars.DBDatei.seekp(0,ios::end);
					fgets(aszOutput,5000,OldFile);
					VStrip(aszOutput);
					sprintf(aszOutput,"%s;\"time\";\"depth\"\n",aszOutput);
					s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
					s_ivars.DBDatei.seekp(0,ios::end);
					for (i = 0; i < s_ivars.cPosCounter;i++) 
					{
						fgets(aszOutput,5000,OldFile);
						VStrip(aszOutput);
						if (s_ivars.apsInfo[i].cSolveTime<=s_ivars.tmPerMove)
						{
							solved++;
							solvetime+=s_ivars.apsInfo[i].cSolveTime/1000;
							sprintf(aszOutput,"%s;\"%i\";\"%i\"\n",aszOutput,s_ivars.apsInfo[i].cSolveTime/1000,s_ivars.apsInfo[i].cSolveDepth);
						}
						else
						{
							solvetime+=s_ivars.tmPerMove/1000;
							sprintf(aszOutput,"%s;\"9999\";\"%i\"\n",aszOutput,s_ivars.apsInfo[i].cLastDepth);
						}
						s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
						s_ivars.DBDatei.seekp(0,ios::end);
					}
					fgets(aszOutput,5000,OldFile);
					VStrip(aszOutput);
					sprintf(aszOutput,"%s;\"%i\";\n",aszOutput,solved);
					s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
					s_ivars.DBDatei.seekp(0,ios::end);
					fgets(aszOutput,5000,OldFile);
					VStrip(aszOutput);
					sprintf(aszOutput,"%s;\"%i\";\n",aszOutput,solvetime);
					s_ivars.DBDatei.write(aszOutput,strlen(aszOutput));
					s_ivars.DBDatei.seekp(0,ios::end);
					s_ivars.DBDatei.close();
					fclose(OldFile);
				}
			}
		}
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-


BOOL FSummary(void)
{
	int	i;
	char hilfe[80];
	
	if (!s_ivars.fUseUCI) {
		if (s_ivars.fAnalyze)
			VSendToEngine("exit");
		else 
		{
			VSendToEngine("?");
		}
		VSendToEngine("force");
	} else {
		VSendToEngine("stop");
	}
	sprintf(s_ivars.aszOutput,"\nResult:   ");
	if (s_ivars.fError) {
		strcat(s_ivars.aszOutput,"ERROR!!");
		s_ivars.cError++;
	} else if (s_ivars.fCorrectAnswer) {
		strcat(s_ivars.aszOutput,"Success");
		s_ivars.cSolved++;
	} else {
		strcat(s_ivars.aszOutput,"Failure");
		s_ivars.cFailed++;
	}
	sprintf(hilfe,"   (%i of %i solved so far - %i",s_ivars.cSolved,s_ivars.cPosCounter+1,s_ivars.cSolved*100/(s_ivars.cPosCounter+1));
	strcat(s_ivars.aszOutput,hilfe);
	strcat(s_ivars.aszOutput,"%)");
	VWriteLogAndOutput();
	VWriteEmpty();
	if (s_ivars.fCorrectAnswer) {
		int	isol;
		s_ivars.apsInfo[s_ivars.cPosCounter].cSolveTime=s_ivars.tmFoundIn;
		sprintf(s_ivars.aszOutput,"Found in: %ld ms (%02d:%02d:%02d.%03d)\n",
			s_ivars.tmFoundIn,
			uHOUR_FROM_MILLI(s_ivars.tmFoundIn),
			uMIN_FROM_MILLI(s_ivars.tmFoundIn),
			uSEC_FROM_MILLI(s_ivars.tmFoundIn),
			uMILLI_FROM_MILLI(s_ivars.tmFoundIn));
		VWriteLogAndOutput();
		isol = s_ivars.tmFoundIn / 1000;
		if ((isol>0) && (s_ivars.tmFoundIn % 1000 == 0)) isol--;
		if (isol < isolMAX)
			s_ivars.argsol[isol]++;
	}
	else s_ivars.apsInfo[s_ivars.cPosCounter].cSolveTime=s_ivars.tmPerMove+1;
	s_ivars.cPosCounter++;
	VWriteEmpty();
	s_ivars.aszOutput[0]=0;
	for (i = 0; i < cbLINE - 1; i++)
		strcat(s_ivars.aszOutput,"-");
	VWriteLogAndOutput();
	VWriteEmpty();
	VWriteEmpty();
	return FNextTest();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is this program's main loop.

void VProcess(void)
{
	int	i;

	//	The function starts in FEATURE_TIMEOUT mode.  It's going to sit here
	//	and collect feature commands for two seconds or perhaps longer or
	//	shorter if directed by the feature commands.
	//
	//	Change in Release 01:  The engine now waits five seconds, since Tim
	//	has said that he's going to make this change in protover 3 anyway.
	//
	sprintf(s_ivars.aszOutput,"\nEpd2Wb, Release 02/TM\n");
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"original version by Bruce Moreland with some modifications by Thomas Mayer\n");
	VWriteLogAndOutput();
	sprintf(s_ivars.aszOutput,"epd2wb comes with ABSOLUTELY NO WARRANTY - released under GPL\n\n");
	VWriteLogAndOutput();
	if (s_ivars.fUseUCI==fFALSE) {
		VSendToEngine("xboard");
		if (!s_ivars.fPureProt1) VSendToEngine("protover 2");
		s_ivars.mode = modeFEATURE_TIMEOUT;
	} else {
		VSendToEngine("uci");
		s_ivars.mode = modeFEATURE_NOTIMEOUT;
	}
	s_ivars.tmEnd = TmNow() + 5000;
	//
	//	Tell the input thread that I'm ready for a line.
	//
	SetEvent(s_ivars.heventStdinAck);
	//
	//	Forever loop.  If this exits, it will be because the EPD file is
	//	exhausted.
	//
	for (;;) {
		int	csz;
		char	aszBuf[1024];
		char	aszVec[1024];
		char *	argsz[256];
		int	ibSecond;

		if (s_ivars.mode == modeFEATURE_TIMEOUT) {
			TM	tm;

			//	In FEATURE_TIMEOUT mode I'll wait some small amount of time
			//	for a line, and if this expires, the features are done and
			//	I start processing the EPD.
			//
			tm = TmNow();
			if ((tm >= s_ivars.tmEnd) ||
				(WaitForSingleObject(s_ivars.heventStdinReady,
				s_ivars.tmEnd - tm) == WAIT_TIMEOUT)) {
				if (!FFeatureTimeout())
					return;
				continue;	// There's no command, so back to the top of the
							//  loop.
			}
		} else if (s_ivars.mode == modeTESTING) {
			TM	tm;
			DWORD	dwTimeout;

			//	In TESTING mode, I'm running a test, so I'll wait for input
			//	only until the text time has expired.
			//
			//	If I run out of time I'll write some output then try to start
			//	a new test.
			//
			tm = TmNow();
			dwTimeout = (s_ivars.fUseSt) ?
				INFINITE : s_ivars.tmEnd - tm;
			if ((tm >= s_ivars.tmEnd) || (s_ivars.fStopAnalysis) || 
				(WaitForSingleObject(s_ivars.heventStdinReady,
				dwTimeout) == WAIT_TIMEOUT)) {
				if (!FSummary())
					return;
				continue;	// There's no command, so back to the top of the
							//  loop.
			}
		} else
			//	In any other mode I'll wait forever for input.
			//
			WaitForSingleObject(s_ivars.heventStdinReady, INFINITE);
		//
		//	If I'm here, it's because the input request did not time out, so
		//	I received an input line, which is presumed to contain a command
		//	or some analysis.
		//
		//	Vectorize the line and try to process it as a command.
		//
/*		if (s_ivars.fDump) {
			sprintf(s_ivars.aszOutput,"Analysis Line:%s\n", s_ivars.aszInBuf);
			VWriteLogAndOutput();
		} */
		strcpy(aszBuf, s_ivars.aszInBuf);
		strcpy(aszVec, s_ivars.aszInBuf);
		csz = CszVectorizeCmd(aszVec, argsz, &ibSecond);
		if (csz) {
			if ((!s_ivars.fUseUCI) || (strcmp(argsz[0],"info"))) {
				for (i = 0; c_argcmdEngine[i].sz != NULL; i++)
					if (!strcmp(c_argcmdEngine[i].sz, argsz[0])) {
						if (!(*c_argcmdEngine[i].pfn)(aszBuf + ibSecond,
							argsz, csz))
							return;
						break;
					}
			}
			if (s_ivars.mode == modeTESTING)
				if (((csz == 3) && (!strcmp(argsz[1], "..."))) && (!s_ivars.fUseUCI)) {
					VEngineMoved(argsz[2]); 
				}
				else {
					if ((s_ivars.fUseUCI) && (!strcmp(argsz[0],"bestmove"))) {
						VEngineMoved(argsz[1]);
					} else {
						VAnalysisLine(argsz, csz);
					}
				}
		}
		//	Tell the input thread that I'm ready for another line of stuff.
		//
		SetEvent(s_ivars.heventStdinAck);
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void ShutdownSignalHandler(int signr)
{
	s_ivars.fQuit = fTRUE;
	if (s_ivars.fUseUCI) {
		VSendToEngine("stop");
	}
	if (s_ivars.fEngineStarted)
		VSendToEngine("quit");
    return;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

LANG c_arglang[] = {
	"Czech",		'P',	'J',	'S',	'V',	'D',	'K',
	"Danish",		'B',	'S',	'L',	'T',	'D',	'K',
	"Dutch",		'O',	'P',	'L',	'T',	'D',	'K',
	"English",		'P',	'N',	'B',	'R',	'Q',	'K',
	"Estonian",		'P',	'R',	'O',	'V',	'L',	'K',
	"Finnish",		'P',	'R',	'L',	'T',	'D',	'K',
	"French",		'P',	'C',	'F',	'T',	'D',	'R',
	"German",		'B',	'S',	'L',	'T',	'D',	'K',
	"Hungarian",	'G',	'H',	'F',	'B',	'V',	'K',
	"Icelandic",	'P',	'R',	'B',	'H',	'D',	'K',
	"Italian",		'P',	'C',	'A',	'T',	'D',	'R',
	"Norwegian",	'B',	'S',	'L',	'T',	'D',	'K',
	"Polish",		'P',	'S',	'G',	'W',	'H',	'K',
	"Portuguese",	'P',	'C',	'B',	'T',	'D',	'R',
	"Romanian",		'P',	'C',	'N',	'T',	'D',	'R',
	"Spanish",		'P',	'C',	'A',	'T',	'D',	'R',
	"Swedish",		'B',	'S',	'L',	'T',	'D',	'K',
	NULL,
};

PLANG PlangFindLang(char * szLang)
{
	int	i;
	
	for (i = 0; c_arglang[i].szLang != NULL; i++)
		if (!strcmpi(c_arglang[i].szLang, szLang))
			return &c_arglang[i];
	return NULL;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

char * const c_aszUsage[] = {
	"Usage: epd2wb <engine> <EPD> <seconds> [flags]",
	"  -?      Usage",
	"  -a      Turns usage of analyze on by default (Prot. 1 - engines with analyze)",
	"  -b<B>   Name of the database file it should use - default=<none>",
	"  -c<E>   For UCI-engines, the file sets the parameters",
	"  -d      Dumps all input and output to the console",
	"  -e<S>   waits <S> econds after starting the engine before sending the first\n          command to it",
	"  -f<F>   Name of the logfile - else no logging !",
	"  -g<x>   Specifies that the analysis should stop when the solution is correct\n          for <x> ply's",
	"  -h<y>   Specifies that the analysis should stop when the solution is correct\n          and the score is above <y>",
	"  -i      Outputs engine information then stops",
	"  -j      Use UCI-1 commands ONLY !",
	"  -l<L>   Choose analysis input language",
	"  -n<x>   multiplys the time sent in st mode with x",
	"  -o      Skip protover for pure wb-prot 1",
	"  -p<P>   Path to the engine",
	"  -s<D>   Skip D analysis fields",
	"  -t      Uses Winboard \"st\" command to try to get old engines to work.",
	"  -u      Turns usage of setboard on by default (even Prot. 1 engines are used\n          then with setboard",
	"  -v<T>   Force to interpret time in c=centiseconds, s=seconds, t=tenthseconds,\n          m=milliseconds",
	"  -w<S>   Protover 1 for initialization wait period (in seconds) default 4",
	"  -x<T>   Protover 1 between test wait period (in seconds) default=1",
	"  -y      Use internal time control instead what the engine sends",
	"  -z      Ignore feature done=0 & feature done=1",
	NULL,
};

void VUsage(void)
{
	int	i;
	
	for (i = 0; c_aszUsage[i] != NULL; i++)
		fprintf(stderr, "%s\n", c_aszUsage[i]);
	printf("\nLanguages:\n\n");
	for (i = 0; c_arglang[i].szLang != NULL; i++) {
		if ((i % 4 == 3) || (c_arglang[i + 1].szLang == NULL))
			fprintf(stderr, "%s\n", c_arglang[i].szLang);
		else
			fprintf(stderr, "%-20s", c_arglang[i].szLang);
	}
	if (s_ivars.fEngineStarted)
		VSendToEngine("quit");
	exit(1);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This initializes some static variables and processes command-line
//	arguments.

int main(int argc, char * argv[])
{
	DWORD	dw;
	int	iszArg;
	int	isz,i;
	char	aszLanguage[256];

    signal(SIGINT, ShutdownSignalHandler);          // CTRL-C
    signal(SIGBREAK, ShutdownSignalHandler); 
    signal(SIGTERM, ShutdownSignalHandler);
	s_ivars.fDump = fFALSE;
	s_ivars.cSkip = 0;
	s_ivars.fEngineStarted = fFALSE;
	s_ivars.fPing = fFALSE;
	s_ivars.fSetboard = fFALSE;
	s_ivars.fAnalyze = fFALSE;
	s_ivars.fColor = fTRUE;
	s_ivars.fPureProt1 = fFALSE;
	s_ivars.fInternalTime = fFALSE;
	s_ivars.fUseUCI = fFALSE;
	s_ivars.fUCI1 = fFALSE;
	s_ivars.cPosCounter = 0;
	s_ivars.cIPTime = 0;
	s_ivars.aszEngine[0] = '\0';
	s_ivars.aszPath[0] = '\0';
	s_ivars.aszSuite[0] = '\0';
	s_ivars.aszDBFile[0] = '\0';
	s_ivars.aszUCIFile[0] = '\0';
	s_ivars.fDBon = fFALSE;
	s_ivars.fUseSt = fFALSE;
	s_ivars.tmWaitInit = 4000;		// 4000 milliseconds.
	s_ivars.tmWaitPos = 1000;		// 1000 milliseconds;
	s_ivars.fFindInfo = fFALSE;
	s_ivars.fLogOn = fFALSE;
	s_ivars.fIgnoreDone = fFALSE;
	s_ivars.fQuit = fFALSE;
	s_ivars.fWaitAfterStart = fFALSE;
	s_ivars.cMultiST=1;
	s_ivars.fStopPly=fFALSE;
	s_ivars.fStopScore=fFALSE;
	s_ivars.fStopAnalysis=fFALSE;
	strcpy(aszLanguage, "English");
	for (isz = 1, iszArg = 0; isz < argc; isz++) {
		char * sz = argv[isz];

		switch (*sz++) {
		case '/':
		case '-':
			switch (*sz++) {
			case 'a':
				s_ivars.fAnalyze = fTRUE;
				break;
			case 'u':
				s_ivars.fSetboard = fTRUE;
				break;
			case 'd':
				s_ivars.fDump = fTRUE;
				break;
			case 'i':
				s_ivars.fFindInfo = fTRUE;
				break;
			case 'l':
				if (*sz != '\0')
					strcpy(aszLanguage, sz);
				else if (isz + 1 == argc)
					VUsage();
				else if ((argv[++isz][0] != '-') &&
					(argv[isz][0] != '/'))
					strcpy(aszLanguage, argv[isz]);
				else
					VUsage();
				break;
			case 's':
				if (*sz != '\0') {
					if (!isdigit(*sz))
						VUsage();
					s_ivars.cSkip = atoi(sz);
				} else if (isz + 1 == argc)
					VUsage();
				else if (isdigit(argv[++isz][0]))
					s_ivars.cSkip = atoi(argv[isz]);
				else
					VUsage();
				break;
			case 't':
				s_ivars.fUseSt = fTRUE;
				break;
			case 'w':
				if (*sz != '\0') {
					if (!isdigit(*sz))
						VUsage();
					s_ivars.tmWaitInit = atoi(sz) * 1000;
				} else if (isz + 1 == argc)
					VUsage();
				else if (isdigit(argv[++isz][0]))
					s_ivars.tmWaitInit = atoi(argv[isz]) * 1000;
				else
					VUsage();
				break;
			case 'e':
				if (*sz != '\0') {
					if (!isdigit(*sz))
						VUsage();
					s_ivars.fWaitAfterStart = fTRUE;
					s_ivars.cWaitAfterStart = atoi(sz) * 1000;
				} else if (isz + 1 == argc)
					VUsage();
				else if (isdigit(argv[++isz][0])) {
					s_ivars.fWaitAfterStart = fTRUE;
					s_ivars.cWaitAfterStart = atoi(argv[isz]) * 1000;
				}
				else
					VUsage();
				break;
			case 'g':
				if (*sz != '\0') {
					if (!isdigit(*sz))
						VUsage();
					s_ivars.cPlys = atoi(sz);
					if (s_ivars.cPlys>0) {
						s_ivars.fStopPly = fTRUE;
					}
				} else if (isz + 1 == argc)
					VUsage();
				else if (isdigit(argv[++isz][0])) {
					s_ivars.cPlys = atoi(argv[isz]);
					if (s_ivars.cPlys>0) {
						s_ivars.fStopPly = fTRUE;
					}
				}
				else
					VUsage();
				break;
			case 'h':
				if (*sz != '\0') {
					if (!isdigit(*sz))
						VUsage();
					s_ivars.cScore = atoi(sz);
					if (s_ivars.cScore>0) {
						s_ivars.fStopScore = fTRUE;
					}
				} else if (isz + 1 == argc)
					VUsage();
				else if (isdigit(argv[++isz][0])) {
					s_ivars.cScore = atoi(argv[isz]);
					if (s_ivars.cScore>0) {
						s_ivars.fStopScore = fTRUE;
					}
				}
				else
					VUsage();
				break;
			case 'n':
				if (*sz != '\0') {
					if (!isdigit(*sz))
						VUsage();
					s_ivars.cMultiST = atoi(sz) * 1000;
				} else if (isz + 1 == argc)
					VUsage();
				else if (isdigit(argv[++isz][0]))
					s_ivars.cMultiST = atoi(argv[isz]) * 1000;
				else
					VUsage();
				break;
			case 'x':
				if (*sz != '\0') {
					if (!isdigit(*sz))
						VUsage();
					s_ivars.tmWaitPos = atoi(sz) * 1000;
				} else if (isz + 1 == argc)
					VUsage();
				else if (isdigit(argv[++isz][0]))
					s_ivars.tmWaitPos = atoi(argv[isz]) * 1000;
				else
					VUsage();
				break;
			case 'p':
				if (*sz != '\0') {
					strcpy(s_ivars.aszPath,sz);
				}
				else {
					if (isz + 1 == argc) {
						VUsage();
					}
					else {
						if ((argv[++isz][0] != '-') && (argv[isz][0] != '/')) {
							strcpy(s_ivars.aszPath,argv[isz]);
						}
						else {
							VUsage();
						}
					}
				}
				i=strlen(s_ivars.aszPath);
				if ((s_ivars.aszPath[i]) != 92) {
					s_ivars.aszPath[i]=92;
					s_ivars.aszPath[i+1]=0;
				}
				break;
			case 'c':
				if (*sz != '\0') {
					strcpy(s_ivars.aszUCIFile,sz);
				}
				else {
					if (isz + 1 == argc) {
						VUsage();
					}
					else {
						if ((argv[++isz][0] != '-') && (argv[isz][0] != '/')) {
							strcpy(s_ivars.aszUCIFile,argv[isz]);
						}
						else {
							VUsage();
						}
					}
				}
				s_ivars.fUseUCI=fTRUE;
				s_ivars.UCIoptions=0;
				s_ivars.cIPTime=4;
				ReadUCIOptions();
				break;
			case 'f':
				if (*sz != '\0') {
					strcpy(s_ivars.aszLogFile,sz);
				}
				else {
					if (isz + 1 == argc) {
						VUsage();
					}
					else {
						if ((argv[++isz][0] != '-') && (argv[isz][0] != '/')) {
							strcpy(s_ivars.aszLogFile,argv[isz]);
						}
						else {
							VUsage();
						}
					}
				}
				s_ivars.fLogOn=fTRUE;
				break;
			case 'b':
				if (*sz != '\0') {
					strcpy(s_ivars.aszDBFile,sz);
				}
				else {
					if (isz + 1 == argc) {
						VUsage();
					}
					else {
						if ((argv[++isz][0] != '-') && (argv[isz][0] != '/')) {
							strcpy(s_ivars.aszDBFile,argv[isz]);
						}
						else {
							VUsage();
						}
					}
				}
				s_ivars.fDBon=fTRUE;
				break;
			case 'y':
				s_ivars.fInternalTime = fTRUE;
				break;
			case 'j':
				s_ivars.fUCI1 = fTRUE;
				break;
			case 'v':
				if (*sz != '\0') {
					if ((sz[1] != 'c') && (sz[1] != 's') && (sz[01] != 't') && (sz[1] != 'm')) {
						VUsage();
						break;
					}
					if (sz[1]=='c') s_ivars.cIPTime=1;
					else if (sz[1]=='s') s_ivars.cIPTime=2;
					else if (sz[1]=='m') s_ivars.cIPTime=4;
					else s_ivars.cIPTime=3;
				} else if (isz + 1 == argc)
					VUsage();
				else {
					isz++;
					sz=argv[isz];
					if ((sz[0] != 'c') && (sz[0] != 's') && (sz[0] != 't') && (sz[0] != 'm')) {
						VUsage();
						break;
					}
					if (sz[0]=='c') s_ivars.cIPTime=1;
					else if (sz[0]=='s') s_ivars.cIPTime=2;
					else if (sz[0]=='m') s_ivars.cIPTime=4;
					else s_ivars.cIPTime=3;
				}
				break;
			case 'o':
				s_ivars.fPureProt1=fTRUE;
				break;
			case 'z':
				s_ivars.fIgnoreDone=fTRUE;
				break;
			default:
				VUsage();
			}
			break;
		default:
			switch (iszArg++) {
			case 0:
				strcpy(s_ivars.aszEngine, argv[isz]);
				break;
			case 1:
				if ((s_ivars.pfI = fopen(argv[isz], "r")) == NULL) {
					perror(argv[isz]);
					exit(1);
				}
				s_ivars.cPosAbs=fGetAbsPos(argv[isz]);
				strcpy(s_ivars.aszSuite, argv[isz]);
				break;
			case 2:
				s_ivars.tmPerMove = atoi(argv[isz]) * 1000;
				break;
			default:
				VUsage();
			}
			break;
		}
	}
	if (iszArg < 3)
		VUsage();
	if ((s_ivars.plang = PlangFindLang(aszLanguage)) == NULL)
		VUsage();
	if (!FStartProcess(&s_ivars.cp, s_ivars.aszEngine, s_ivars.aszPath))
		VDisplayLastError("Can't start engine");
	else s_ivars.fEngineStarted = fTRUE;
	if (s_ivars.fUseSt)			// "-t" switch eliminates "tmWaitInit".
		s_ivars.tmWaitInit = 0;
	if (s_ivars.fLogOn) {
		s_ivars.LOGDatei.open(s_ivars.aszLogFile,ios::trunc | ios::out | ios::in);
		s_ivars.LOGDatei.seekp(0,ios::beg);
		s_ivars.fLogOpen=fTRUE;
	}
	s_ivars.plangEnglish = PlangFindLang("English");
	Assert(s_ivars.plangEnglish != NULL);
	s_ivars.heventStdinReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	s_ivars.heventStdinAck = CreateEvent(NULL, FALSE, FALSE, NULL);
	s_ivars.heventStdinPing = CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThread(NULL, 0, DwInput, NULL, 0, &dw);
	if (s_ivars.fWaitAfterStart) {
		Sleep(s_ivars.cWaitAfterStart);
	}
	VProcess();
	if (!s_ivars.fFindInfo)
		VDumpResults();
	VSendToEngine("quit");
	s_ivars.fQuit = fTRUE;
	VKillTime(2000);
	if (s_ivars.fLogOpen) s_ivars.LOGDatei.close();
//	DestroyChildProcess(&s_ivars.cp);	// This hangs so it's commented out.
	return 1;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
