// ***********************************************************************
//
//  Kim-1 MicroChess (c) 1976-2005 Peter Jennings, www.benlo.com
//  6502 emulation   (c) 2005 Bill Forster
//  xboard interface (c) 2007 Andre Adrian
//
//  Runs an emulation of the Kim-1 Microchess on any standard C platform
//
// ***********************************************************************

// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES// LOSS OF USE,
// DATA, OR PROFITS// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// **********************************************************************
// *
// *  Part 1
// *  ------
// *  Create virtual 6502 platform using standard C facilities.
// *  Goal is to run Microchess on any platform supporting C.
// *
// *       Part 1 added July 2005 by Bill Forster (www.triplehappy.com)
// **********************************************************************

// Standard library include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <math.h>
#include <signal.h>

// Use <setjmp.h> macros and functions to emulate the "jump to reset
//  stack pointer then restart program" behaviour used by microchess
jmp_buf jmp_chess;
#define EXIT        -1
#define RESTART     -2
void RESTART_CHESS( void )  // start CHESS program with reset stack
{
    longjmp( jmp_chess, RESTART );
}
void EXIT_TO_SYSTEM( void ) // return to operating system
{
    longjmp( jmp_chess, EXIT );
}

// 6502 emulation memory
typedef unsigned char byte;
byte zeropage[256];
byte stack[256];
byte stack_cy[256];
byte stack_v[256];

// 6502 emulation registers
byte reg_a, reg_f, reg_x, reg_y, reg_s, reg_cy, reg_v, temp_cy;
unsigned int temp1, temp2;

// Debug stuff
#if 0
    #define DBG , register_dump()
#else
    #define DBG
#endif
void register_dump( void )
{
    printf( "A=%02x X=%02x Y=%02x S=%02x F=%02X CY=%d V=%d\n",
        reg_a, reg_x, reg_y, reg_s, reg_f, reg_cy, reg_v );
}

// 6502 emulation macros - register moves
#define T(src,dst)          reg_f = (dst) = (src)    DBG
#define A reg_a
#define S reg_s
#define X reg_x
#define Y reg_y
#define TYA                 T(Y,A)
#define TXS                 T(X,S)
#define TAX                 T(A,X)
#define TAY                 T(A,Y)
#define TSX                 T(S,X)
#define TXA                 T(X,A)

// 6502 emulation macros - branches
#define BEQ(label)          if( reg_f == 0 )     goto label
#define BNE(label)          if( reg_f != 0 )     goto label
#define BPL(label)          if( ! (reg_f&0x80) ) goto label
#define BMI(label)          if( reg_f & 0x80 )   goto label
#define BCC(label)          if( !reg_cy )        goto label
#define BCS(label)          if( reg_cy )         goto label
#define BVC(label)          if( !reg_v )         goto label
#define BVS(label)          if( reg_v )          goto label
#define BRA(label) /*extra*/ goto label

// 6502 emulation macros - call/return from functions
#define JSR(func)           func()
#define RTS                 return

// 6502 emulation macros - jump to functions, note that in
//  assembly language jumping to a function is a more efficient
//  way of calling the function then returning, so we emulate
//  that in the high level language by (actually) calling then
//  returning. There is no JEQ 6502 opcode, but it's useful to
//  us so we have made it up! (like BRA, SEV)
#define JMP(func)           if( 1 )          { func(); return; } \
                            else // else eats ';'
#define JEQ(func) /*extra*/ if( reg_f == 0 ) { func(); return; } \
                            else // else eats ';'

// 6502 emulation macros - load registers
//  Addressing conventions;
//   default addressing mode is zero page, else indicate with suffix;
//   i = immediate
//   x = indexed, zero page
//   f = indexed, not zero page (f for "far")
#define ZP(addr8)           (zeropage[ (byte) (addr8) ])
#define ZPX(addr8,idx)      (zeropage[ (byte) ((addr8)+(idx)) ])
#define LDAi(dat8)          reg_f = reg_a = dat8                  DBG
#define LDAx(addr8,idx)     reg_f = reg_a = ZPX(addr8,idx)        DBG
#define LDAf(addr16,idx)    reg_f = reg_a = (addr16)[idx]         DBG
#define LDA(addr8)          reg_f = reg_a = ZP(addr8)             DBG
#define LDXi(dat8)          reg_f = reg_x = dat8                  DBG
#define LDX(addr8)          reg_f = reg_x = ZP(addr8)             DBG
#define LDYi(dat8)          reg_f = reg_y = dat8                  DBG
#define LDY(addr8)          reg_f = reg_y = ZP(addr8)             DBG
#define LDYx(addr8,idx)     reg_f = reg_y = ZPX(addr8,idx)        DBG

// 6502 emulation macros - store registers
#define STA(addr8)          ZP(addr8)      = reg_a                DBG
#define STAx(addr8,idx)     ZPX(addr8,idx) = reg_a                DBG
#define STX(addr8)          ZP(addr8)      = reg_x                DBG
#define STY(addr8)          ZP(addr8)      = reg_y                DBG
#define STYx(addr8,idx)     ZPX(addr8,idx) = reg_y                DBG

// 6502 emulation macros - set/clear flags
#define CLD            // luckily CPU's BCD flag is cleared then never set
#define CLC                 reg_cy = 0                            DBG
#define SEC                 reg_cy = 1                            DBG
#define CLV                 reg_v  = 0                            DBG
#define SEV /*extra*/       reg_v  = 1  /*avoid problematic V emulation*/ DBG

// 6502 emulation macros - accumulator logical operations
#define ANDi(dat8)          reg_f = (reg_a &= dat8)               DBG
#define ORA(addr8)          reg_f = (reg_a |= ZP(addr8))          DBG

// 6502 emulation macros - shifts and rotates
#define ASL(addr8)          reg_cy = (ZP(addr8)&0x80) ? 1 : 0,  \
                            ZP(addr8) = ZP(addr8)<<1,           \
                            reg_f = ZP(addr8)                     DBG
#define ROL(addr8)          temp_cy = (ZP(addr8)&0x80) ? 1 : 0, \
                            ZP(addr8) = ZP(addr8)<<1,           \
                            ZP(addr8) |= reg_cy,                \
                            reg_cy = temp_cy,                   \
                            reg_f = ZP(addr8)                     DBG
#define LSR                 reg_cy = reg_a & 0x01,              \
                            reg_a  = reg_a>>1,                  \
                            reg_a  &= 0x7f,                     \
                            reg_f = reg_a                         DBG

// 6502 emulation macros - push and pull
#define PHA                 stack[reg_s--]  = reg_a               DBG
#define PLA                 reg_a           = stack[++reg_s]      DBG
#define PHY                 stack[reg_s--]  = reg_y               DBG
#define PLY                 reg_y           = stack[++reg_s]      DBG
#define PHP                 stack   [reg_s] = reg_f,       \
                            stack_cy[reg_s] = reg_cy,      \
                            stack_v [reg_s] = reg_v,       \
                            reg_s--                               DBG
#define PLP                 reg_s++,                       \
                            reg_f  = stack   [reg_s],      \
                            reg_cy = stack_cy[reg_s],      \
                            reg_v  = stack_v [reg_s]              DBG

// 6502 emulation macros - compare
#define cmp(reg,dat)        reg_f  = ((reg) - (dat)), \
                            reg_cy = ((reg) >= (dat) ? 1 : 0)  DBG
#define CMPi(dat8)          cmp( reg_a, dat8 )
#define CMP(addr8)          cmp( reg_a, ZP(addr8) )
#define CMPx(addr8,idx)     cmp( reg_a, ZPX(addr8,idx) )
#define CMPf(addr16,idx)    cmp( reg_a, (addr16)[idx] )
#define CPXi(dat8)          cmp( reg_x, dat8 )
#define CPXf(addr16,idx)    cmp( reg_x, (addr16)[idx] )
#define CPYi(dat8)          cmp( reg_y, dat8 )

// 6502 emulation macros - increment,decrement
#define DEX                 reg_f = --reg_x                       DBG
#define DEY                 reg_f = --reg_y                       DBG
#define DEC(addr8)          reg_f = --ZP(addr8)                   DBG
#define INX                 reg_f = ++reg_x                       DBG
#define INY                 reg_f = ++reg_y                       DBG
#define INC(addr8)          reg_f = ++ZP(addr8)                   DBG
#define INCx(addr8,idx)     reg_f = ++ZPX(addr8,idx)              DBG

// 6502 emulation macros - add
#define adc(dat)            temp1 = reg_a,                   \
                            temp2 = (dat),                   \
                            temp1 += (temp2+(reg_cy?1:0)),   \
                            reg_f = reg_a = (byte)temp1,     \
                            reg_cy = ((temp1&0xff00)?1:0)         DBG
#define ADCi(dat8)          adc( dat8 )
#define ADC(addr8)          adc( ZP(addr8) )
#define ADCx(addr8,idx)     adc( ZPX(addr8,idx) )
#define ADCf(addr16,idx)    adc( (addr16)[idx] )

// 6502 emulation macros - subtract
//   (note that both as an input and an output cy flag has opposite
//    sense to that used for adc(), seems unintuitive to me)
#define sbc(dat)            temp1 = reg_a,                   \
                            temp2 = (dat),                   \
                            temp1 -= (temp2+(reg_cy?0:1)),   \
                            reg_f = reg_a = (byte)temp1,     \
                            reg_cy = ((temp1&0xff00)?0:1)         DBG
#define SBC(addr8)          sbc( ZP(addr8) )
#define SBCx(addr8,idx)     sbc( ZPX(addr8,idx) )

// Test some of the trickier opcodes (hook this up as needed)
void test_function( void )
{
    byte hi, lo;
                LDAi    (0x33);     // 0x4444 - 0x3333 = 0x1111
                STA     (0);
                STA     (1);
                LDAi    (0x44);
                SEC;
                SBC     (0);
                lo      = reg_a;
                LDAi    (0x44);
                SBC     (1);
                hi      = reg_a;

                LDAi    (0x44);     // 0x3333 - 0x4444 = 0xeeef
                STA     (0);
                STA     (1);
                LDAi    (0x33);
                SEC;
                SBC     (0);
                lo      = reg_a;
                LDAi    (0x33);
                SBC     (1);
                hi      = reg_a;

                LDAi    (0x33);     // 0x3333 + 0x4444 = 0x7777
                STA     (0);
                STA     (1);
                LDAi    (0x44);
                CLC;
                ADC     (0);
                lo      = reg_a;
                LDAi    (0x44);
                ADC     (1);
                hi      = reg_a;
}


// **********************************************************************
// *
// *  Part 2
// *  ------
// *  Original microchess program by Peter Jennings, www.benlo.com
// *  In this form, 6502 assembly language has been minimally transformed
// *  to run with the virtual 6502 in C facilities created in part 1.
// *   (New comments by Bill Forster are identified with text (WRF))
// **********************************************************************

//
// page zero variables
//
const byte BOARD     = 0x50;	// LOCATION OF PIECES
const byte BK        = 0x60;	// OPPONENT'S PIECES
const byte PIECE     = 0xB0;	// INITIAL PIECE LOCATIONS
const byte SQUARE    = 0xB1;	// TO SQUARE OF .PIECE
const byte SP2       = 0xB2;	// STACK POINTER FOR STACK 2
const byte SP1       = 0xB3;	// STACK POINTER FOR STACK 1
const byte INCHEK    = 0xB4;	// MOVE INTO CHECK FLAG
const byte STATE     = 0xB5;	// STATE OF ANALYSIS
const byte MOVEN     = 0xB6;	// MOVE TABLE POINTER
const byte OMOVE     = 0xDC;	// OPENING POINTER
const byte WCAP0     = 0xDD;	// COMPUTER CAPTURE 0
const byte COUNT     = 0xDE;	// START OF COUNT TABLE
const byte BCAP2     = 0xDE;	// OPPONENT CAPTURE 2
const byte WCAP2     = 0xDF;	// COMPUTER CAPTURE 2
const byte BCAP1     = 0xE0;	// OPPONENT CAPTURE 1
const byte WCAP1     = 0xE1;	// COMPUTER CAPTURE 1
const byte BCAP0     = 0xE2;	// OPPONENT CAPTURE 0
const byte MOB       = 0xE3;	// MOBILITY
const byte MAXC      = 0xE4;	// MAXIMUM CAPTURE
const byte CC        = 0xE5;	// CAPTURE COUNT
const byte PCAP      = 0xE6;	// PIECE ID OF MAXC
const byte BMOB      = 0xE3;	// OPPONENT MOBILITY
const byte BMAXC     = 0xE4;	// OPPONENT MAXIMUM CAPTURE
const byte BMCC      = 0xE5;    // (BCC) OPPONENT CAPTURE COUNT
const byte BMAXP     = 0xE6;	// OPPONENT MAXP
const byte XMAXC     = 0xE8;	// CURRENT MAXIMUM CAPTURE
const byte WMOB      = 0xEB;	// COMPUTER MOBILITY
const byte WMAXC     = 0xEC;	// COMPUTER MAXIMUM CAPTURE
const byte WCC       = 0xED;	// COMPUTER CAPTURE COUNT
const byte WMAXP     = 0xEE;	// COMPUTER MAXP
const byte PMOB      = 0xEF;	// PREVIOUS COMPUTER MOB
const byte PMAXC     = 0xF0;	// PREVIOUS COMPUTER MAXC
const byte PCC       = 0xF1;	// PREVIOUS COMPUTER CC
const byte PCP       = 0xF2;	// PREVIOUS COMPUTER MAXP
const byte OLDKY     = 0xF3;	// KEY INPUT TEMPORARY
const byte BESTP     = 0xFB;	// PIECE OF BEST MOVE FOUND
const byte BESTV     = 0xFA;	// VALUE OF BEST MOVE FOUND
const byte BESTM     = 0xF9;	// TO SQUARE OF BEST MOVE
const byte DIS1      = 0xFB;	// DISPLAY POINT 1
const byte DIS2      = 0xFA;	// DISPLAY POINT 2
const byte DIS3      = 0xF9;	// DISPLAY POINT 3

// (WRF) For C version, data definitions precede code references to data
byte SETW[]   = {       0x03, 0x04, 0x00, 0x07, 0x02, 0x05, 0x01, 0x06,
                        0x10, 0x17, 0x11, 0x16, 0x12, 0x15, 0x14, 0x13,
                        0x73, 0x74, 0x70, 0x77, 0x72, 0x75, 0x71, 0x76,
                        0x60, 0x67, 0x61, 0x66, 0x62, 0x65, 0x64, 0x63
                };

byte MOVEX[]  = {       0x00, 0xF0, 0xFF, 0x01, 0x10, 0x11, 0x0F, 0xEF, 0xF1,
                        0xDF, 0xE1, 0xEE, 0xF2, 0x12, 0x0E, 0x1F, 0x21
                };

byte POINTS[] = {       0x0B, 0x0A, 0x06, 0x06, 0x04, 0x04, 0x04, 0x04,
                        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02
                };

// GIUOCO PIANO
// 1. e4, e5  2. Nf3, Nc6  3. Bc4, Bc5  4. c3, Nf6  5. d4, d4  6. d4, Bb4+  7. Nc3, Ne4  8. OO, Nc3  9. Qe1
byte OPNING[] = {       0x99, 0x25, 0x0B, 0x25, 0x01, 0x00, 0x33, 0x25,
                        0x07, 0x36, 0x34, 0x0D, 0x34, 0x34, 0x0E, 0x52,
                        0x25, 0x0D, 0x45, 0x35, 0x04, 0x55, 0x22, 0x06,
                        0x43, 0x33, 0x0F, 0xCC
                };

// (WRF) level information:
//       | Level       | addr=02F2 |  addr=018B
//       |             | (level1)  |  (level2)
//       +-------------+-----------+-------------
//       | SUPER BLITZ |    00     |    FF
//       | BLITZ       |    00     |    FB
//       | NORMAL      |    08     |    FB
static byte level1=8;
static byte level2=0xfb;

// (WRF) Forward declarations
void JANUS( void );
void INPUT( void );
void DISP( void );
void GNMZ( void );
void GNMX( void );
void GNM( void );
void RUM( void );
void STRV( void );
void SNGMV( void );
void LINE( void );
void REVERSE( void );
void CMOVE( void );
void RESET( void );
void GENRM( void );
void UMOVE( void );
void MOVE( void );
void CKMATE( void );
void GO( void );
void DISMV( void );
void STRATGY( void );

// C to emulator functions
void KIM1_OUT( void );
void CHESS( byte key );


void CHESS( byte key )
{
                CLD;                        // INITIALIZE
                LDXi    (0xFF);             // TWO STACKS
                TXS;
                LDXi    (0xC8);
                STX     (SP2);
//
//       ROUTINES TO LIGHT LED
//       DISPLAY AND GET KEY
//       FROM KEYBOARD
//
//OUT:          JSR     (POUT);             // DISPLAY AND
//              JSR     (KIN);              // GET INPUT
//              CMP     (OLDKY);            // KEY IN ACC
//              BEQ     (OUT);              // (DEBOUNCE)
//              STA     (OLDKY);
//
				reg_a = key;				// GET INPUT adrian
                CMPi    (0x0C);             // [C]
                BNE     (NOSET);            // SET UP
                LDXi    (0x1F);             // BOARD
WHSET:          LDAf    (SETW,X);           // FROM
                STAx    (BOARD,X);          // SETW
                DEX;
                BPL     (WHSET);
                LDXi    (0x1B);             // *ADDED FOR OPNING BOOK
                STX     (OMOVE);            // INITS TO 0xFF
                LDAi    (0xCC);             // Display CCC
                BNE     (CLDSP);
//
NOSET:          CMPi    (0x0E);             // [E]
                BNE     (NOREV);            // REVERSE
                JSR     (REVERSE);          // BOARD IS
                LDAi    (0xEE);             //   IS
                BNE     (CLDSP);
//
NOREV:          CMPi    (0x14);             // [PC]
                BNE     (NOGO);             // PLAY CHESS
                JSR     (GO);
CLDSP:          STA     (DIS1);             // DISPLAY
                STA     (DIS2);             // ACROSS
                STA     (DIS3);             // DISPLAY
//                BNE     (CHESS_BEGIN);	// adrian
				RTS;						// adrian
//
NOGO:           CMPi    (0x0F);             // [F]
                BNE     (NOMV);             // MOVE MAN
                JSR     (MOVE);             // AS ENTERED
                JMP     (DISP);             //
NOMV:           JMP     (INPUT);            //
}

//
//       THE ROUTINE JANUS DIRECTS THE
//       ANALYSIS BY DETERMINING WHAT
//       SHOULD OCCUR AFTER EACH MOVE
//       GENERATED BY GNM
//
//
//
void JANUS( void )
{               LDX     (STATE);
                BMI     (NOCOUNT);
//
//       THIS ROUTINE COUNTS OCCURRENCES
//       IT DEPENDS UPON STATE TO INDEX
//       THE CORRECT COUNTERS
//
/*COUNTS:*/     LDA     (PIECE);
                BEQ     (OVER);             // IF STATE=8
                CPXi    (0x08);             // DO NOT COUNT
                BNE     (OVER);             // BLK MAX CAP
                CMP     (BMAXP);            // MOVES FOR
                BEQ     (XRT);              // WHITE
//
OVER:           INCx    (MOB,X);            // MOBILITY
                CMPi    (0x01);             //  + QUEEN
                BNE     (NOQ);              // FOR TWO
                INCx    (MOB,X);
//
NOQ:            BVC     (NOCAP);
                LDYi    (0x0F);             // CALCULATE
                LDA     (SQUARE);           // POINTS
ELOOP:          CMPx    (BK,Y);             // CAPTURED
                BEQ     (FOUN);             // BY THIS
                DEY;                        // MOVE
                BPL     (ELOOP);
FOUN:           LDAf    (POINTS,Y);
                CMPx    (MAXC,X);
                BCC     (LESS);             // SAVE IF
                STYx    (PCAP,X);           // BEST THIS
                STAx    (MAXC,X);           // STATE
//
LESS:           CLC;
                PHP;                        // ADD TO
                ADCx    (CC,X);             // CAPTURE
                STAx    (CC,X);             // COUNTS
                PLP;
//
NOCAP:          CPXi    (0x04);
                BEQ     (ON4);
                BMI     (TREE);             //(=00 ONLY)
XRT:            RTS;

//
//      GENERATE FURTHER MOVES FOR COUNT
//      AND ANALYSIS
//
ON4:            LDA     (XMAXC);            // SAVE ACTUAL
                STA     (WCAP0);            // CAPTURE
                LDAi    (0x00);             // STATE=0
                STA     (STATE);
                JSR     (MOVE);             // GENERATE
                JSR     (REVERSE);          // IMMEDIATE
                JSR     (GNMZ);             // REPLY MOVES
                JSR     (REVERSE);
//
                LDAi    (0x08);             // STATE=8
                STA     (STATE);            // GENERATE
                JSR     (GNM);              // CONTINUATION
                JSR     (UMOVE);            // MOVES
//
                JMP     (STRATGY);          //
NOCOUNT:        CPXi    (0xF9);
                BNE     (TREE);
//
//      DETERMINE IF THE KING CAN BE
//      TAKEN, USED BY CHKCHK
//
                LDA     (BK);               // IS KING
                CMP     (SQUARE);           // IN CHECK?
                BNE     (RETJ);             // SET INCHEK=0
                LDAi    (0x00);             // IF IT IS
                STA     (INCHEK);
RETJ:           RTS;

//
//      IF A PIECE HAS BEEN CAPTURED BY
//      A TRIAL MOVE, GENERATE REPLIES &
//      EVALUATE THE EXCHANGE GAIN/LOSS
//
TREE:           BVC     (RETJ);             // NO CAP
                LDYi    (0x07);             // (PIECES)
                LDA     (SQUARE);
LOOPX:          CMPx    (BK,Y);
                BEQ     (FOUNX);
                DEY;
                BEQ     (RETJ);             // (KING)
                BPL     (LOOPX);            // SAVE
FOUNX:          LDAf    (POINTS,Y);         // BEST CAP
                CMPx    (BCAP0,X);          // AT THIS
                BCC     (NOMAX);            // LEVEL
                STAx    (BCAP0,X);
NOMAX:          DEC     (STATE);
                LDAf    (&level2,0);        // IF STATE=FB  (WRF, was LDAi (0xFB);)
                CMP     (STATE);            // TIME TO TURN
                BEQ     (UPTREE);           // AROUND
                JSR     (GENRM);            // GENERATE FURTHER
UPTREE:         INC     (STATE);            // CAPTURES
                RTS;
}


//
//      THE PLAYER'S MOVE IS INPUT
//
void INPUT( void )
{
                CMPi    (0x08);             // NOT A LEGAL
                BCS     (ERROR);            // SQUARE #
                JSR     (DISMV);
                JMP     (DISP);             // fall through
ERROR:          JMP     (RESTART_CHESS);
}

void DISP( void )
{
                LDXi    (0x1F);
SEARCH:         LDAx    (BOARD,X);
                CMP     (DIS2);
                BEQ     (HERE);             // DISPLAY
                DEX;                        // PIECE AT
                BPL     (SEARCH);           // FROM
HERE:           STX     (DIS1);             // SQUARE
                STX     (PIECE);
                JMP     (RESTART_CHESS);
}

//
//      GENERATE ALL MOVES FOR ONE
//      SIDE, CALL JANUS AFTER EACH
//      ONE FOR NEXT STEP
//
//
void GNMZ( void )
{
                LDXi    (0x10);             // CLEAR
                JMP     (GNMX);             // fall through
}

void GNMX( void )
{
                LDAi    (0x00);             // COUNTERS
CLEAR:          STAx    (COUNT,X);
                DEX;
                BPL     (CLEAR);
                JMP     (GNM);              // fall though
}

void GNM( void )
{
                LDAi    (0x10);             // SET UP
                STA     (PIECE);            // PIECE
NEWP:           DEC     (PIECE);            // NEW PIECE
                BPL     (NEX);              // ALL DONE?
                RTS;                        //    -YES
//
NEX:            JSR     (RESET);            // READY
                LDY     (PIECE);            // GET PIECE
                LDXi    (0x08);
                STX     (MOVEN);            // COMMON START
                CPYi    (0x08);             // WHAT IS IT?
                BPL     (PAWN);             // PAWN
                CPYi    (0x06);
                BPL     (KNIGHT);           // KNIGHT
                CPYi    (0x04);
                BPL     (BISHOP);           // BISHOP
                CPYi    (0x01);
                BEQ     (QUEEN);            // QUEEN
                BPL     (ROOK);             // ROOK
//
KING:           JSR     (SNGMV);            // MUST BE KING!
                BNE     (KING);             // MOVES
                BEQ     (NEWP);             // 8 TO 1
QUEEN:          JSR     (LINE);
                BNE     (QUEEN);            // MOVES
                BEQ     (NEWP);             // 8 TO 1
//
ROOK:           LDXi    (0x04);
                STX     (MOVEN);            // MOVES
AGNR:           JSR     (LINE);             // 4 TO 1
                BNE     (AGNR);
                BEQ     (NEWP);
//
BISHOP:         JSR     (LINE);
                LDA     (MOVEN);            // MOVES
                CMPi    (0x04);             // 8 TO 5
                BNE     (BISHOP);
                BEQ     (NEWP);
//
KNIGHT:         LDXi    (0x10);
                STX     (MOVEN);            // MOVES
AGNN:           JSR     (SNGMV);            // 16 TO 9
                LDA     (MOVEN);
                CMPi    (0x08);
                BNE     (AGNN);
                BEQ     (NEWP);
//
PAWN:           LDXi    (0x06);
                STX     (MOVEN);
P1:             JSR     (CMOVE);            // RIGHT CAP?
                BVC     (P2);
                BMI     (P2);
                JSR     (JANUS);            // YES
P2:             JSR     (RESET);
                DEC     (MOVEN);            // LEFT CAP?
                LDA     (MOVEN);
                CMPi    (0x05);
                BEQ     (P1);
P3:             JSR     (CMOVE);            // AHEAD
                BVS     (NEWP);             // ILLEGAL
                BMI     (NEWP);
                JSR     (JANUS);
                LDA     (SQUARE);           // GETS TO
                ANDi    (0xF0);             // 3RD RANK?
                CMPi    (0x20);
                BEQ     (P3);               // DO DOUBLE
                BRA     (NEWP);             // JMP (NEWP);
}

//
//      CALCULATE SINGLE STEP MOVES
//      FOR K,N
//
void SNGMV( void )
{
                JSR     (CMOVE);            // CALC MOVE
                BMI     (ILL1);             // -IF LEGAL
                JSR     (JANUS);            // -EVALUATE
ILL1:           JSR     (RESET);
                DEC     (MOVEN);
                RTS;
}

//
//     CALCULATE ALL MOVES DOWN A
//     STRAIGHT LINE FOR Q,B,R
//
void LINE( void )
{
LINE:           JSR     (CMOVE);            // CALC MOVE
                BCC     (OVL);              // NO CHK
                BVC     (LINE);             // NOCAP
OVL:            BMI     (ILL);              // RETURN
                PHP;
                JSR     (JANUS);            // EVALUATE POSN
                PLP;
                BVC     (LINE);             // NOT A CAP
ILL:            JSR     (RESET);            // LINE STOPPED
                DEC     (MOVEN);            // NEXT DIR
                RTS;
}

//
//      EXCHANGE SIDES FOR REPLY
//      ANALYSIS
//
void REVERSE( void )
{
                LDXi    (0x0F);
ETC:            SEC;
                LDYx    (BK,X);             // SUBTRACT
                LDAi    (0x77);             // POSITION
                SBCx    (BOARD,X);          // FROM 77
                STAx    (BK,X);
                STYx    (BOARD,X);          // AND
                SEC;
                LDAi    (0x77);             // EXCHANGE
                SBCx    (BOARD,X);          // PIECES
                STAx    (BOARD,X);
                DEX;
                BPL     (ETC);
                RTS;
}
//
//        CMOVE CALCULATES THE TO SQUARE
//        USING SQUARE AND THE MOVE
//       TABLE  FLAGS SET AS FOLLOWS:
//       N - ILLEGAL MOVE
//       V - CAPTURE (LEGAL UNLESS IN CH)
//       C - ILLEGAL BECAUSE OF CHECK
//       [MY THANKS TO JIM BUTTERFIELD
//        WHO WROTE THIS MORE EFFICIENT
//        VERSION OF CMOVE]
//
void CMOVE( void )
{
    byte src;
                LDA     (SQUARE);           // GET SQUARE
                src     = reg_a;
                LDX     (MOVEN);            // MOVE POINTER
                CLC;
                ADCf    (MOVEX,X);          // MOVE LIST
                STA     (SQUARE);           // NEW POS'N
                ANDi    (0x88);
                BNE     (ILLEGAL);          // OFF BOARD
                LDA     (SQUARE);
//
                LDXi    (0x20);
LOOP:           DEX;                        // IS TO
                BMI     (NO);               // SQUARE
                CMPx    (BOARD,X);          // OCCUPIED?
                BNE     (LOOP);
//
                CPXi    (0x10);             // BY SELF?
                BMI     (ILLEGAL);
//
             // LDAi    (0x7F);             // MUST BE CAP!
             // ADCi    (0x01);             // SET V FLAG
                SEV;    LDAi(0x80);         // Avoid problematic V emulation
                BVS     (SPX);              // (JMP)
//
NO:             CLV;                        // NO CAPTURE
//
SPX:            LDA     (STATE);            // SHOULD WE
                BMI     (RETL);             // DO THE
                CMPf    (&level1,0);        // CHECK CHECK? (WRF: was CMPi (0x08);)
                BPL     (RETL);
//
//        CHKCHK REVERSES SIDES
//       AND LOOKS FOR A KING
//       CAPTURE TO INDICATE
//       ILLEGAL MOVE BECAUSE OF
//       CHECK  SINCE THIS IS
//       TIME CONSUMING, IT IS NOT
//       ALWAYS DONE
//
/*CHKCHK:*/     PHA;                        // STATE
                PHP;
                LDAi    (0xF9);
                STA     (STATE);            // GENERATE
                STA     (INCHEK);           // ALL REPLY
                JSR     (MOVE);             // MOVES TO
                JSR     (REVERSE);          // SEE IF KING
                JSR     (GNM);              // IS IN
                JSR     (RUM);              // CHECK
                PLP;
                PLA;
                STA     (STATE);
                LDA     (INCHEK);
                BMI     (RETL);             // NO - SAFE
                SEC;                        // YES - IN CHK
                LDAi    (0xFF);
                RTS;
//
RETL:           CLC;                        // LEGAL
                LDAi    (0x00);             // RETURN
                RTS;
//
ILLEGAL:        LDAi    (0xFF);
                CLC;                        // ILLEGAL
                CLV;                        // RETURN
                RTS;
}

//
//       REPLACE PIECE ON CORRECT SQUARE
//
void RESET( void )
{
                LDX     (PIECE);            // GET LOGAT
                LDAx    (BOARD,X);          // FOR PIECE
                STA     (SQUARE);           // FROM BOARD
                RTS;
}


//
//
//
void GENRM( void )
{
                JSR     (MOVE);             // MAKE MOVE
/*GENR2:*/      JSR     (REVERSE);          // REVERSE BOARD
                JSR     (GNM);              // GENERATE MOVES
                JMP     (RUM);              // fall through
}

void RUM( void )
{
                JSR     (REVERSE);          // REVERSE BACK
                JMP     (UMOVE);            // fall through
}

//
//       ROUTINE TO UNMAKE A MOVE MADE BY
//         MOVE
//
void UMOVE( void )
{
                TSX;                        // UNMAKE MOVE
                STX     (SP1);
                LDX     (SP2);              // EXCHANGE
                TXS;                        // STACKS
                PLA;                        // MOVEN
                STA     (MOVEN);
                PLA;                        // CAPTURED
                STA     (PIECE);            // PIECE
                TAX;
                PLA;                        // FROM SQUARE
                STAx    (BOARD,X);
                PLA;                        // PIECE
                TAX;
                PLA;                        // TO SOUARE
                STA     (SQUARE);
                STAx    (BOARD,X);
                JMP     (STRV);
}

//
//       THIS ROUTINE MOVES PIECE
//       TO SQUARE, PARAMETERS
//       ARE SAVED IN A STACK TO UNMAKE
//       THE MOVE LATER
//
void MOVE( void )
{               TSX;
                STX     (SP1);              // SWITCH
                LDX     (SP2);              // STACKS
                TXS;
                LDA     (SQUARE);
                PHA;                        // TO SQUARE
                TAY;
                LDXi    (0x1F);
CHECK:          CMPx    (BOARD,X);          // CHECK FOR
                BEQ     (TAKE);             // CAPTURE
                DEX;
                BPL     (CHECK);
TAKE:           LDAi    (0xCC);
                STAx    (BOARD,X);
                TXA;                        // CAPTURED
                PHA;                        // PIECE
                LDX     (PIECE);
                LDAx    (BOARD,X);
                STYx    (BOARD,X);          // FROM
                PHA;                        // SQUARE
                TXA;
                PHA;                        // PIECE
                LDA     (MOVEN);
                PHA;                        // MOVEN
                JMP     (STRV);             // fall through
}

// (WRF) Fortunately when we swap stacks we jump here and swap back before
//  returning. So we aren't swapping stacks to do threading (if we were we
//  would need to enhance 6502 stack emulation to incorporate our
//  subroutine mechanism, instead we simply use the native C stack for
//  subroutine return addresses).
void STRV( void )
{
                TSX;
                STX     (SP2);              // SWITCH
                LDX     (SP1);              // STACKS
                TXS;                        // BACK
                RTS;
}

//
//       CONTINUATION OF SUB STRATGY
//       -CHECKS FOR CHECK OR CHECKMATE
//       AND ASSIGNS VALUE TO MOVE
//
void CKMATE( void )
{
                LDX     (BMAXC);            // CAN BLK CAP
                CPXf    (POINTS,0);         // MY KING?
                BNE     (NOCHEK);
                LDAi    (0x00);             // GULP!
                BEQ     (RETV);             // DUMB MOVE!
//
NOCHEK:         LDX     (BMOB);             // IS BLACK
                BNE     (RETV);             // UNABLE TO
                LDX     (WMAXP);            // MOVE AND
                BNE     (RETV);             // KING IN CH?
                LDAi    (0xFF);             // YES! MATE
//
RETV:           LDXi    (0x04);             // RESTORE
                STX     (STATE);            // STATE=4
//
//       THE VALUE OF THE MOVE (IN ACCU)
//       IS COMPARED TO THE BEST MOVE AND
//       REPLACES IT IF IT IS BETTER
//
/*PUSH:*/       CMP     (BESTV);            // IS THIS BEST
                BCC     (RETP);             // MOVE SO FAR?
                BEQ     (RETP);
                STA     (BESTV);            // YES!
                LDA     (PIECE);            // SAVE IT
                STA     (BESTP);
                LDA     (SQUARE);
                STA     (BESTM);            // FLASH DISPLAY
RETP:           RTS;						// adrian
}

//
//       MAIN PROGRAM TO PLAY CHESS
//       PLAY FROM OPENING OR THINK
//
void GO( void )
{
                LDX     (OMOVE);            // OPENING?
                BMI     (NOOPEN);           // -NO   *ADD CHANGE FROM BPL
                LDA     (DIS3);             // -YES WAS
                CMPf    (OPNING,X);         // OPPONENT'S
                BNE     (END);              // MOVE OK?
                DEX;
                LDAf    (OPNING,X);         // GET NEXT
                STA     (DIS1);             // CANNED
                DEX;                        // OPENING MOVE
                LDAf    (OPNING,X);
                STA     (DIS3);             // DISPLAY IT
                DEX;
                STX     (OMOVE);            // MOVE IT
                BNE     (MV2);              // (JMP)
//
END:            LDAi    (0xFF);             // *ADD - STOP CANNED MOVES
                STA     (OMOVE);            // FLAG OPENING
NOOPEN:         LDXi    (0x0C);             // FINISHED
                STX     (STATE);            // STATE=C
                STX     (BESTV);            // CLEAR BESTV
                LDXi    (0x14);             // GENERATE P
                JSR     (GNMX);             // MOVES
//
                LDXi    (0x04);             // STATE=4
                STX     (STATE);            // GENERATE AND
                JSR     (GNMZ);             // TEST AVAILABLE
//                                             MOVES
//
                LDX     (BESTV);            // GET BEST MOVE
                CPXi    (0x0F);             // IF NONE
                BCC     (MATE);             // OH OH!
//
MV2:            LDX     (BESTP);            // MOVE
                LDAx    (BOARD,X);          // THE
                STA     (BESTV);            // BEST
                STX     (PIECE);            // MOVE
                LDA     (BESTM);
                STA     (SQUARE);           // AND DISPLAY
                JSR     (MOVE);             // IT
                JMP     (RESTART_CHESS);
//
MATE:           LDAi    (0xFF);             // RESIGN
                RTS;                        // OR STALEMATE
}

//
//       SUBROUTINE TO ENTER THE
//       PLAYER'S MOVE
//
void DISMV( void )
{
                LDXi    (0x04);             // ROTATE
DROL:           ASL     (DIS3);             // KEY
                ROL     (DIS2);             // INTO
                DEX;                        // DISPLAY
                BNE     (DROL);             //
                ORA     (DIS3);
                STA     (DIS3);
                STA     (SQUARE);
                RTS;
}

//
//       THE FOLLOWING SUBROUTINE ASSIGNS
//       A VALUE TO THE MOVE UNDER
//       CONSIDERATION AND RETURNS IT IN
//       THE ACCUMULATOR
//
void STRATGY( void )
{
                CLC;
                LDAi    (0x80);
                ADC     (WMOB);             // PARAMETERS
                ADC     (WMAXC);            // WITH WEIGHT
                ADC     (WCC);              // OF O.25
                ADC     (WCAP1);
                ADC     (WCAP2);
                SEC;
                SBC     (PMAXC);
                SBC     (PCC);
                SBC     (BCAP0);
                SBC     (BCAP1);
                SBC     (BCAP2);
                SBC     (PMOB);
                SBC     (BMOB);
                BCS     (POS);              // UNDERFLOW
                LDAi    (0x00);             // PREVENTION
POS:            LSR;
                CLC;                        // **************
                ADCi    (0x40);
                ADC     (WMAXC);            // PARAMETERS
                ADC     (WCC);              // WITH WEIGHT
                SEC;                        // OF 0.5
                SBC     (BMAXC);
                LSR;                        // **************
                CLC;
                ADCi    (0x90);
                ADC     (WCAP0);            // PARAMETERS
                ADC     (WCAP0);            // WITH WEIGHT
                ADC     (WCAP0);            // OF 1.0
                ADC     (WCAP0);
                ADC     (WCAP1);
                SEC;                        // [UNDER OR OVER-
                SBC     (BMAXC);            // FLOW MAY OCCUR
                SBC     (BMAXC);            // FROM THIS
                SBC     (BMCC);             // SECTION]
                SBC     (BMCC);
                SBC     (BCAP1);
                LDX     (SQUARE);           // ***************
                CPXi    (0x33);
                BEQ     (POSN);             // POSITION
                CPXi    (0x34);             // BONUS FOR
                BEQ     (POSN);             // MOVE TO
                CPXi    (0x22);             // CENTRE
                BEQ     (POSN);             // OR
                CPXi    (0x25);             // OUT OF
                BEQ     (POSN);             // BACK RANK
                LDX     (PIECE);
                BEQ     (NOPOSN);
                LDYx    (BOARD,X);
                CPYi    (0x10);
                BPL     (NOPOSN);
POSN:           CLC;
                ADCi    (0x02);
NOPOSN:         JMP     (CKMATE);           // CONTINUE
}

// **********************************************************************
// *
// *  Part 3
// *  ------
// *  xboard/WinBoard interface with chess notation
// *
// *       Part 3 added by Andre Adrian
// **********************************************************************
// 15dec2008 compatible to xboard (signal())

int xst = 0;                    // Exchange state: 0=computer moves white, 1=computer moves black

// Output in chess notation
void KIM1_OUT(void)
{
  char *col = "hgfedcba";
  char *row = "12345678";
  char *xcol = "abcdefgh";
  char *xrow = "87654321";

  if (0xff == ZP(DIS1) && 0xff == ZP(DIS2) && 0xff == ZP(DIS3)) {
    if (xst) {
      printf("1-0 {White mates}\n");
    } else {
      printf("0-1 {Black mates}\n");
    }
  } else if ((ZP(DIS2) & 0x88) | (ZP(DIS3) & 0x88)) {
    // printf("%02X%02X %02X\n", ZP(DIS1), ZP(DIS2), ZP(DIS3));
  } else {
    if (xst) {
      printf("move %c%c%c%c\n",
             xcol[ZP(DIS2) & 0x7], xrow[ZP(DIS2) / 0x10],
             xcol[ZP(DIS3) & 0x7], xrow[ZP(DIS3) / 0x10]);
    } else {
      printf("move %c%c%c%c\n",
             col[ZP(DIS2) & 0x7], row[ZP(DIS2) / 0x10],
             col[ZP(DIS3) & 0x7], row[ZP(DIS3) / 0x10]);
    }
  }
}


char *gets_s(char *s, int size)
{
  char *rv = fgets(s, size, stdin);
  if('\n' == s[strlen(s)-1]) s[strlen(s)-1]=0;
  return rv;
}

int main(int argc, char *argv[])
{
  int st = 0;
  int color = 1;                // computer colors: 0=white, 1=black

  signal(SIGINT, SIG_IGN);      // xboard needs this !

/* To input moves in chess notation (e7e5 instead of 6343) I put
 * a state machine around the Microchess state machine. The
 * main loop is now the for(;;) statement below. The state 0 gives 
 * Microchess the C (clear, new game) command. In state 1 the human 
 * input is read. The states 2 to 6 feed the human move in microchess 
 * notation to the CHESS routine. States 7 and 8 perform the Microchess 
 * move and emit result.
 * Tested with xboard/WinBoard version 4.2.7
 * Microchess does understand the commands: xboard, new, white, black, 
 * go, quit and move in e2e4 form.
 * Microchess emits Error, move in e7e5 form and result.
 */

  for (;;) {
    if (EXIT != setjmp(jmp_chess)) {
      char s[100];

      switch (st) {
      case 0:
        xst = 0;
        color = 1;
        ++st;
        CHESS(0x0C);            // KIM-1 KEY C clear board
        break;
      case 1:
        gets_s(s, sizeof(s));
        if (0 == strcmp(s, "xboard")) {
          printf
              ("feature myname=\"Microchess\" variants=\"nocastle\" done=1\n");
        } else if (0 == strcmp(s, "new")) {
          st = 0;
        } else if (0 == strcmp(s, "white")) {
          color = 0;            // xboard tells the computer color here
        } else if (0 == strcmp(s, "black")) {
          color = 1;
        } else if (0 == strcmp(s, "go")) {
          if (0 == color) {
            st = 7;
          }                     // play the first move
          if (color != xst) {
            xst = 1 - xst;
            CHESS(0x0E);        // KIM-1 KEY E computer plays other side
          }
        } else if (0 == strcmp(s, "quit")) {
          exit(0);
        } else if (4 == strlen(s)) {
          if (s[0] < 'a' || s[0] > 'h')
            break;
          if (s[1] < '1' || s[1] > '8')
            break;
          if (s[2] < 'a' || s[2] > 'h')
            break;
          if (s[3] < '1' || s[3] > '8')
            break;
          ++st;
          if (color != xst) {
            xst = 1 - xst;
            CHESS(0x0E);        // KIM-1 KEY E computer plays other side
          }
        } else {
          printf("Error (unknown command): %s\n", s);
        }
        break;
      case 2:
        ++st;
        if (xst)
          CHESS(7 - (s[1] - '1'));
        else
          CHESS(s[1] - '1');
        break;
      case 3:
        ++st;
        if (xst)
          CHESS(s[0] - 'a');
        else
          CHESS(7 - (s[0] - 'a'));
        break;
      case 4:
        ++st;
        if (xst)
          CHESS(7 - (s[3] - '1'));
        else
          CHESS(s[3] - '1');
        break;
      case 5:
        ++st;
        if (xst)
          CHESS(s[2] - 'a');
        else
          CHESS(7 - (s[2] - 'a'));
        break;
      case 6:
        ++st;
        CHESS(0xF);             // KIM-1 KEY F accept human move
        break;
      case 7:
        ++st;
        CHESS(0x14);            // KIM-1 KEY PC computer move
        break;
      case 8:
        st = 1;
        KIM1_OUT();             // DISPLAY computer move
        break;
      }
      fflush(stdout);
    }
  }
  return (0);
}
