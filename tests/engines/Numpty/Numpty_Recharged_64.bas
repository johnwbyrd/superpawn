
'*******************************************************
'                    N U M P T Y
'               version: Recharged (Oct 14)
'
'               a weak chess program
'            by Chris Tatham (c)2005-14
'*******************************************************


'NB This is internal version numpty201014


' Note:
' This version uses the previous NumptyDD code base. It has benefited from the translation of the
' code-base from FB 0.15b (dating from 2005) into the latest version FB 0.90. This mammouth
' undertaking carried out by 'frisian' has included some code refactoring/improvement. My role as
' author in this work was simply helping to get it to work properly by fixing a few bugs. The
' translation is functionally identical in providing exactly the same output. Frisian's notes of 
' this work have been retained and included below - there is a full audit trail of the changes 
' made - whilst this undoubtedly adds complexity to the source code, it is felt important to 
' retain the full record and incremental nature of Numpty's development. 

' Chris Tatham (Oct 2014)



'----------------------------------------------------------------------------------------------

' make nomega run under freebasic 0.23/0.24 started 5/3/2012 by frisian
' idea was to compile it with FB 0.23 so things are a bit messy
' don't know if I (possibly) messed things up or if they work differenly in fb 0.15 and 0.23-0.90.1
'
' there is a problem when there is castling possible / winboard commucation does not work ???
' display move's as a1,a8 instead of 21 or 28
' rewrote display SUB,
' replaced lots of GOTO's with CONTINUE FOR and EXIT FOR or tweaked the IF THEN statements
' save didn't save some settings, and load didn't load them either
' minor tweaks / boundry check
' some goto's/label's remain
' made some changes to speedup's some routine's
' gen_move only generate castling moves if they are allowed
' most winboard stuff is not altered.

' march 2014
' got the winboard protocol working
' replaced the data for WP_PST(Y) and WP_end_PST(Y) with the data from Numpty_DD
' changed a line back to the original code in order to see some information in Arena (oops)

' include the data for book move from Numpty_DD
' corrected some errors due to the convertion
' moved some code from in user_entry to commandloop
' replaced code in incheck() and attack() by much simpler code
' added code in loadgame to find wkloc and bkloc

' cleaned the listing by removing some obsolete/replaced code
' compiling with GCC 64x gave two warning of which one turned out to be a bug (movefrom) = -99 needed to be b(movefrom) = -99

' 18/3/2014 still work in progress
' a GCC compiled version crashes when used with winboard/arena
' user_move code needs some work
' written my own code to get the castling problem under controle (looks ok now)
' replaced "score = -9000" with "score = -10000" (FB_Numpty)

' 23/4/2014 work in progress
' will compile with GCC(64) and run under Arena
' change the test order in gen_move for castling, simple tests first then the more time consuming attack and incheck routine's
' incorperated make_move and unmake_move routine's in ab_search (small time gain, had hoped for a bigger gain)
' tweaked the time control statements
' made size of multi dimensioned array's equal to powers of 2, gives FB the chance to use shl instead of *


'Open "error"+Str(Int(Timer))+".txt" For Append As #99 'used for debug file 3514

#Include Once "windows.bi"

#Define NULL 0
#define TRUE 1
#define FALSE 0
'#Define bad_score -10000 * 2

#Macro move_2_str(num)
(Chr(num Mod 10 + 96) + Chr(num \ 10 + 47))
#EndMacro




'*******************************************************
'   The core winboard interface code is written by
'   Tom McBurney and is used here with his permission.
'   Its integration within NOmega is my own and any
'   resulting errors are my responsibility
'*******************************************************

'Option NoKeyword Time

'The following functions are used for communicating with Winboard
Declare Function GetCommand As String
Declare Function SendCommand(sWBCmd As String) As Integer
'Print #99,"$$";swbcmd;"$$"

Declare Sub WaitForWinboard

' Declare the API functions we will be using for communicating to Winboard
'Dim KernelLib As Any Ptr
'Dim Shared WriteFile As Function(ByVal hFile As Integer, lpBuffer As Any Ptr, ByVal nNumberOfBytesToWrite As Integer, lpNumberOfBytesWritten As Integer Ptr, lpOverlapped As Any Ptr) As Integer
'Dim Shared GetStdHandle As Function(ByVal nStdHandle As Integer) As Integer
'Dim Shared ReadFile As Function(ByVal hFile As Integer, lpBuffer As Any Ptr, ByVal nNumberOfBytesToRead As Integer, lpNumberOfBytesRead As Integer, lpOverlapped As Any Ptr) As Integer
'Dim Shared PeekNamedPipe As Function(ByVal hNamedPipe As Integer, lpBuffer As Any Ptr, ByVal nBufferSize As Integer, lpBytesRead As Integer, lpTotalBytesAvail As Integer, lpBytesLeftThisMessage As Integer) As Integer
'Dim Shared CloseHandle As Function(ByVal As Integer) As UInteger

' Link KernelLib to kernel32.dll
'KernelLib = DylibLoad( "KERNEL32")

' Link the functions to the appropriate Kernel32 library.
'WriteFile = DyLibSymbol(KernelLib, "WriteFile")
'GetStdHandle = DyLibSymbol(KernelLib, "GetStdHandle")
'ReadFile = DyLibSymbol(KernelLib, "ReadFile")
'PeekNamedPipe = DyLibSymbol(KernelLib, "PeekNamedPipe")
'CloseHandle = DyLibSymbol(kernellib, "closehandle")
'Dim Shared As Integer STD_OUTPUT_HANDLE = -11l
'Dim Shared As Integer STD_INPUT_HANDLE = -10l

'Dim Shared As UInteger Ptr OutputHandle
'Dim Shared As UInteger Ptr InputHandle

'Dim Shared As String sTemp
'Dim Shared As String sWBCmd

Dim Shared As Integer WinboardMode
Dim Shared As Integer ForceMode

'WinboardMode = 0 '0 = console mode; 1 = winboard mode '### no need, DIM set's them to 0
'ForceMode = 0

'Initialise:
'/
'***********************************
' Initialise key variables
'***********************************
Dim Shared As Integer B(-1 To 119)                   'Array for 10x12 board
'Dim Shared As String u_str(0 To 98) 'Array to display the board
', EV
Dim Shared As Integer best_score, score, Alpha, beta, eval
'Dim shared AS single Alpha, beta   '### double declared

'Dim Shared As UByte Pseu_Moves(100) ' ### for a max depth of 100 ???
Const c_depth = 15
Dim Shared As UInteger Pseu_Moves(c_depth)
Dim Shared As UInteger max_depth = c_depth           ' max_search_depth
Dim Shared As UInteger depth = c_depth               ' depth

Dim Shared As UInteger temp_val1(c_depth)
Dim Shared As Integer epsqr(c_depth)                 'ep square
'-=-=-=-=-=-=-=-=-=-=-=-=-=
Declare Sub CommandLoop
'-=-=-=-=-=-=-=-=-=-=-=-=-=

Declare Sub gen_move(side As Integer, depth As UInteger)
Declare Function incheck(side As Integer) As Integer
'Declare Function make_move(side As Integer, Depth As Integer, Pseu_Moves() As Integer, y As Integer, fmove() As Integer) As Integer
'Declare Sub unmake_move(side As Integer, Depth As Integer, Pseu_Moves() As Integer, y As Integer, temp_val1() As Integer, fmove() As Integer)
Declare Function attack(side As Integer, ps As Integer) As Integer
Declare Sub display
Declare Sub user_entry
Declare Function user_move(MoveFrom As UInteger, MoveTo As UInteger) As Integer
Declare Sub computer_move
Declare Function ab_search(side As Integer, depth As Integer, Alpha As Integer, beta As Integer, fmove() As Integer) As Integer
Declare Function evaluate(side As Integer) As Integer
Declare Function assessboard(side As Integer, best_score As Integer) As Integer
Declare Function QS(Vo As Integer, side As Integer) As Integer
Declare Sub ending
Declare Function open_book As Integer
Declare Sub help
Declare Sub SaveGame
Declare Sub LoadGame
Declare Function rep_check As Integer
Declare Sub clear_board
Declare Sub fen(o_str As String)

'declare function undo() 'not yet implemented

Dim Shared As String B_str(0 To 20)              'Book string
B_str(1) = "P" : B_str(2) = "N" : B_str(3) = "B" : B_str(4) = "R" : B_str(6) = "Q" : B_str(7) = "K"
B_str(19) = "p" : B_str(18) = "n" : B_str(17) = "b" : B_str(16) = "r" : B_str(14) = "q" : B_str(13) = "k"
B_str(0) = "-"                                   ' ### for use in the sub display

Dim Shared As Integer perft
Dim Shared As UInteger MoveFrom, MoveTo
Dim Shared As Integer act_epsq = -1 ' actual ep-square value (ie post-move)
Dim Shared As String o_str                       ' string to hold input for fen parsing
Dim Shared As ULongInt nodes_root_move
Dim Shared As Integer search_depth

Const M_N = 127, MT_F_SM = 3                     'M_N = maximum move numbers; MT_F_SM = special moves- ie to record promotion & capture
' array to hold all generated moves
Dim Shared As Integer Move_list(c_Depth, M_N, MT_F_SM)
' array used to bublle sort root moves
Dim Shared As Integer Move_list_root(c_Depth, M_N, MT_F_SM)
'Dim Shared As Byte Move_list_temp( c_Depth, M_N, MT_F_SM) ' temp array used for bubble sort of root moves
Dim Shared As ULongInt nodes

Dim Shared As Integer wkloc, bkloc        ' nb will need to move this and adjust for board rotation with colour
' move-offset arrays to reflect all possible move combinations
Dim Shared As Integer Move_Offset(16), Pawn_Offset(4)

' ***** piece values for board representation *****
'Dim Shared As Byte King = 7, Queen = 6, Rook = 4, Bishop = 3, Knight = 2, Pawn = 1
Const king As Integer = 7 : Const queen As Integer = 6 : Const rook As Integer = 4
Const bishop As Integer = 3 : Const knight As Integer = 2 : Const pawn As Integer = 1
'***** piece values for evaluation *****
'Dim Shared As Integer Pawn_Value = 100, Knight_Value = 300, Bishop_Value = 325, Rook_Value = 500, Queen_Value = 950
' ### value's don't change
Const Pawn_Value As Integer = 100 : Const Bishop_Value As Integer = 325 : Const Queen_Value As Integer = 950
' ### value's change during the games
Dim Shared As Integer Knight_Value = 300, Rook_Value = 500
'***** arrays for piece square tables (PST)
Dim Shared As Integer WP_PST(100), BP_PST(100), WKt_PST(100), BKt_PST(100)
Dim Shared As Integer WBp_PST(100), BBp_PST(100), WRk_PST(100), BRk_PST(100), WQn_PST(100), BQn_PST(100)
Dim Shared As Integer WKg_PST(100), BKg_PST(100), BKg_end_PST(100), WKg_end_PST(100)
Dim Shared As Integer K_r_end_PST(100), WP_end_PST(100), BP_end_PST(100)

'***** other variables *****
'Dim Shared As Byte White = 1, Black = -1
Const White As Integer = 1 : Const Black As Integer = -1
Dim Shared As Integer computer = 1, opponent = -1
Dim Shared As Integer STM                           ' side to move flag
Dim Shared As Integer wprom(c_depth), bprom(c_depth)
Dim Shared As Integer w_cas(2), b_cas(2)            ' black/white castling status - actual values
'w_cas(1) = 0:w_cas(2) = 0:b_cas(1) = 0:b_cas(2) = 0 ' 1 = king-side; 2 = queen-side; 0 = castling possible (default val)

Dim Shared As Integer w_casflags(c_depth,1 To 2), b_casflags(c_depth, 1 To 2)
' white & black castle flags in search  '1 = O-O; 2 = O-O-O
'                             vals: -1 no castle, 0 - available, 1 - done!

'w_casflags(depth,1) = 0:w_casflags(depth,2) = 0
'b_casflags(depth,1) = 0:b_casflags(depth,2) = 0

Dim Shared As String pr_str                      '- computer promotion piece
Dim Shared As Integer cmpclr = -1                  ' 1 - comp = white' -1 - comp = black
' cmpclr = -1  ' therefore computer is black (ie -1); computer playing white = 1
' Dim Shared As Byte Flipboard(120) ' array used for flipping board values when colours switched
Dim Shared As UInteger MoveNo
Dim Shared As Integer No_Pieces = 32
Dim Shared As Integer WQ = 1, BQ = 1, wr = 2, br = 2' No queens/rooks (used for endgame eval testing)
Dim Shared As Integer Bbp, Wbp, Bkn, Wkn            ' vars used for insufficient material test (Bbp = Black bishop etc)
Dim Shared As Integer side = white                  'assume playing white
Dim Shared As Integer IM                            'insufficient material flag
Dim Shared As UInteger fifty_move                   '= 0
Dim Shared As UInteger fmove(c_depth)               ' keep track of 50 move increment during search
Dim Shared As UInteger move_hist(255, 1 To 4)            'max number moves 200 store movefrom - to for both colours ### 250
'hold board position for each move for black & white (repetition detection)
Dim Shared As String Pos_State_W_str(250), Pos_State_B_str(250)
Dim Shared As Integer post                          ' assume no posting of PV for winboard unless conditions met later

Dim Shared As Double t1, t2, accum               'variables to keep track of time used
' vars to handle time management
Dim Shared As Double start_time, move_control, game_time, time_left, think_time, inc, t_move, break_move, av_think_time
Dim Shared As UInteger timed_out
Dim Shared As Integer conv_clock
' dim shared As Single start_time '### made double, not shared
Dim Shared As Integer conv_clock_move_no         ' variable to keep track of moves for timing purposes where increment = 0
Dim Shared As Integer fix_depth                     ' flag to indicate if fixed depth search should be carried out
Dim Shared As Integer bookhit = TRUE                ' 1 = bookmove found ; 0 = not found!
Dim Shared As ULongInt display_node              ' var for showing nodes searched  if post variable is set
Dim Shared As Integer init_swp_val                  ' initial swap off value - set in make move
Dim Shared As Integer cap_sq                        ' initial capture square for swap off - set in make move
Dim Shared As Integer temp_root_move_val            'temp to test
Dim Shared As Integer stand_pat                  'eval used in swap off calculation, to determine whether capture is worthwhile
Dim Shared As Integer ENDGAME                       ' 0 = not endgame; 1 = endgame
' Dim Shared As Byte prom_depth ' ### not used ???
Dim Shared As Integer Mat_left                   ' count of material on board - to determine endgame phase
' Mat_left = ((Bishop_Value + Knight_Value) * 4) + (Rook_Value * 4) + (Queen_Value * 2) ' total material @ game start
' total material @ game start
Mat_left = ((Bishop_Value + Knight_Value + Rook_Value) * 4) + (Queen_Value * 2)
Dim Shared As Integer orig_root                     ' number moves at root - used for in search time checks
Dim Shared As Integer it_depth                      ' iterative search depth
'-=-=-=-=-=-=-=-=-=-=-=-=-=
Dim Shared As Integer valid_move
'-=-=-=-=-=-=-=-=-=-=-=-=-=
Dim shared AS Integer bestscore(250)     ' array for recent move history of computer's best score - to determine whether to resign  CHT 5/8/14
Dim shared AS UInteger GIVEUP = FALSE            ' Resign flag 

'-------------------------------------------------------------------------------

'**********************************************************
' Define ChessBoard with initial position
'
' board is represented by standard 10 x 12 array
' initial position is populated via Read/Data
' dummy (-99) is used to represent positions off the board
'**********************************************************
Dim As Integer x, y, tmp
Dim As UInteger hitspace

Restore

For x = 0 To 119
  B(x) = -99
Next x

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read tmp
    B(Y) = tmp
    If tmp = 7 Then wkloc = y 'temp line to allow non-fen position set-up
    If tmp = -7 Then bkloc = y
  Next Y
Next X

' initial position values

Data  -4, -2, -3, -6, -7, -3, -2, -4
Data  -1, -1, -1, -1, -1, -1, -1, -1
Data   0,  0,  0,  0,  0,  0,  0,  0
Data   0,  0,  0,  0,  0,  0,  0,  0
Data   0,  0,  0,  0,  0,  0,  0,  0
Data   0,  0,  0,  0,  0,  0,  0,  0
Data   1,  1,  1,  1,  1,  1,  1,  1
Data   4,  2,  3,  6,  7,  3,  2,  4


'***********************************************************
' establish move offsets - ie movement directions on board
'   Data 1 - 4  = sliders (diagonal)
'   Data 5 - 8  = slider (horizontal/vertical)
'   Data 9 - 16 = jumper (ie knight moves)
'
'   pawn offsets are for non-capture moves
'***********************************************************

For x = 1 To 16
  Read Move_Offset(x)
Next x

Data 11,9,-11,-9,-10,-1,1,10,-21,-19,-8,-12,8,12,19,21

For x = 1 To 4
  Read Pawn_Offset(x)
Next x

Data 20,10,-20,-10


'***********************************************************
' establish piece square table values for each piece, using
' separate values for the endgame
'
' values with further refinement and further development
' are based on those given on
' http://chessprogramming.wikispaces.com/
'***********************************************************

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WP_PST(Y)
  Next Y
Next X

Data  0,  0,  0,  0,  0,  0,  0,  0
Data 20, 25, 35, 50, 50, 35, 25, 20
Data  8, 12, 20, 38, 38, 20, 12,  8
Data  5,  5, 10, 30, 30, 10,  5,  5
Data  0,  0,  5, 25, 25,  5,  0,  0
Data  0, -5,-10,  5,  5,-10, -5,  0
Data  5, 10, 10,-20,-20, 10, 10,  5
Data  0,  0,  0,  0,  0,  0,  0,  0

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WP_end_PST(Y)
  Next Y
Next X

Data   0,  0,  0,  0,  0,  0,  0,  0
Data  55, 42, 35, 30, 30, 35, 42, 55
Data  35, 27, 22, 20, 20, 22, 27, 35
Data  20, 15, 11, 10, 10, 11, 15, 20
Data  10,  8,  5,  5,  5,  5,  8, 10
Data   0,  0,  0,  0,  0,  0,  0,  0
Data   0,  0,  0,  0,  0,  0,  0,  0
Data   0,  0,  0,  0,  0,  0,  0,  0

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WKt_PST(Y)
  Next Y
Next X

Data -40,-30,-20,-20,-20,-20,-30,-40
Data -35,-20,  0,  5,  5,  0,-20,-35
Data -25,  0, 10, 15, 15, 10,  0,-25
Data -25,  5, 15, 20, 20, 15,  5,-25
Data -25,  0, 15, 20, 20, 15,  0,-25
Data -25,  5, 10, 15, 15, 10,  5,-25
Data -35,-20,  0,  5,  5,  0,-20,-35
Data -60,-40,-30,-30,-30,-30,-40,-60

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WBp_PST(Y)
  Next Y
Next X

Data -20,-10,-10,-10,-10,-10,-10,-20
Data  -5,  0,  0,  0,  0,  0,  0, -5
Data  -5,  0,  5, 10, 10,  5,  0, -5
Data  -5,  5,  5, 10, 10,  5,  5, -5
Data  -5,  0, 10, 10, 10, 10,  0, -5
Data  -5, 10, 10, 10, 10, 10, 10, -5
Data  -5,  5,  0,  0,  0,  0,  5, -5
Data -20,-15,-15,-15,-15,-15,-15,-20


For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WRk_PST(Y)
  Next Y
Next X

Data   5,  5,  5,  5,  5,  5,  5,  5
Data  10, 15, 15, 15, 15, 15, 15, 10
Data  -5,  0,  0,  0,  0,  0,  0, -5
Data  -5,  0,  0,  0,  0,  0,  0, -5
Data  -5,  0,  0,  0,  0,  0,  0, -5
Data  -5,  0,  0,  0,  0,  0,  0, -5
Data  -5,  0,  0,  0,  0,  0,  0, -5
Data   0,  0,  2,  5,  5,  2,  0,  0

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WQn_PST(Y)
  Next Y
Next X

Data -15,-10,-10, -5, -5,-10,-10,-15
Data -10,  0,  0,  0,  0,  0,  0,-10
Data -10,  0,  5,  5,  5,  5,  0,-10
Data  -5,  0,  5,  5,  5,  5,  0, -5
Data  -5,  0,  5,  5,  5,  5,  0, -5
Data -10,  5,  5,  5,  5,  5,  0,-10
Data -10,  0,  5,  0,  0,  0,  0,-10
Data -20,-10,-10, -5, -5,-10,-10,-20

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WKg_PST(Y)
  Next Y
Next X

Data -30,-40,-40,-50,-50,-40,-40,-30
Data -30,-40,-40,-50,-50,-40,-40,-30
Data -30,-40,-40,-50,-50,-40,-40,-30
Data -30,-40,-40,-50,-50,-40,-40,-30
Data -20,-30,-30,-40,-40,-30,-30,-20
Data -15,-20,-20,-20,-20,-20,-20,-15
Data  15, 15,  0,  0,  0,  0, 15, 15
Data  20, 35, 10,  0,  0, 10, 35, 20

For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read WKg_end_PST(Y)
  Next Y
Next X

Data -40,-35,-30,-20,-20,-30,-35,-40
Data -30,-20,  0,  0,  0,  0,-20,-30
Data -30,-10, 20, 30, 30, 20,-10,-30
Data -30,-10, 30, 40, 40, 30,-10,-30
Data -30,-10, 30, 40, 40, 30,-10,-30
Data -30,-10, 20, 30, 30, 20,-10,-30
Data -30,-30,  0,  0,  0,  0,-30,-30
Data -55,-30,-30,-30,-30,-30,-30,-55

'*** Special PST for king in KvR/Q endgames
For X = 90 To 20 Step -10
  For Y = X+1 To X+8
    Read K_r_end_PST(Y)
  Next Y
Next X

Data  50, 70, 100, 100, 100, 100, 70,  50
Data  70, 40,  40,  40,  40,  40, 40,  70
Data 100, 40,  20,  20,  20,  20, 40, 100
Data 100, 40,  20, -20, -20,  20, 40, 100
Data 100, 40,  20, -20, -20,  20, 40, 100
Data 100, 40,  20,  20,  20,  20, 40, 100
Data  70, 40,  40,  40,  40,  40, 40,  70
Data  50, 70, 100, 100, 100, 100, 70,  50

For X = 20 To 90 Step 10                         ' reverse weights for black
  Var tmp = 110 - X
  For Y = 1 To 8
    Var tmp_s = X + Y
    Var tmp_d = tmp + Y
    BP_PST(tmp_s) = WP_PST(tmp_d)
    BP_end_PST(tmp_s) = WP_end_PST(tmp_d)
    BKt_PST(tmp_s) = WKt_PST(tmp_d)
    BBp_PST(tmp_s) = WBp_PST(tmp_d)
    BRk_PST(tmp_s) = WRk_PST(tmp_d)
    BQn_PST(tmp_s) = WQn_PST(tmp_d)
    BKg_PST(tmp_s) = WKg_PST(tmp_d)
    BKg_end_PST(tmp_s) = WKg_end_PST(tmp_d)
  Next Y
Next X
'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

'*************************************
' test if using GUI or console mode
'*************************************

WaitForWinboard
If WinboardMode = 1 Then
  CommandLoop
  End
End If

fix_depth = 1                                    ' set fix depth flag for console mode

max_depth = 4                                    ' set default fixed depth search 4 ply for console mode  **

'**************************
' display start screen
'**************************
Cls
'Screen 21,1,1,1
ScreenRes 720, 840
Width 720 \ 8, 840 \ 14

Print "           *************************************************"
Print "           *     N U M P T Y  -  c h e s s  e n g i n e    *"
Print "           *                                               *"
Print "           *       v e r s i o n - R e c h a r g e d       *"
Print "           *                                               *"
Print "           *         by  C h r i s  T a t h a m            *"
Print "           *                                               *"
Print "           *                                               *"
Print "           *             help - option list                *"
Print "           *                                               *"
Print "           *             (W)hite or (B)lack?               *"
Print "           *                                               *"
Print "           *     all entries must be made in lowercase     *"
Print "           *                                               *"
Print "           *************************************************"

Do
  hitspace = GetKey
Loop Until hitspace = 66 Or hitspace = 98 Or hitspace = 87 Or hitspace = 119
Cls

'if press white then computer is black
If hitspace = 87 Or hitspace = 119 Then cmpclr = -1 Else cmpclr = 1


If Side = cmpclr = -1 Then                      'Black
  display
  GoTo mn2
End If

'------------------------------------------------------------------------------

'**************************
' main control program
'**************************

'main:
Do
  display

  'mn1:
  Do

    user_entry
    valid_move = user_move(Movefrom, MoveTo)

    'If valid_Move = FALSE Then GoTo mn1
  Loop Until valid_move = TRUE

  If assessboard(side, best_score) = TRUE Then ending

  Side = -Side
  mn2:

  computer_move

  If assessboard(side, best_score) = TRUE Then ending

  Side = -side

  'GoTo main
Loop
End

'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
Sub user_entry
  ' ### a select case would be better
  Dim As Integer er
  Dim As UInteger j
  'Dim As String l1_str, m1_str, l2_str, m2_str
  ' Dim As UInteger l1, m1, l2, m2, f, t, l

  If WinboardMode = 0 Then                       ' GoTo ue2 ' no need for text entry if using GUI!

    'ue1:
    Do
      Do                                         '### empty keyboard buffer
      Loop Until InKey = ""
      Input "move/option"; o_str
      o_str = Trim(o_str)   '### trim spaces, make LCase is not a good idea
      If Left(o_str, 9) = "setboard " Then fen(o_str)
      If Left(o_str, 5) = "perft" Then
        Input "depth"; depth
        If depth < 1 Then
          depth = 1
          Print
          Print "depth ="; depth
        End If
        t1 = Timer
        perft = TRUE
        computer_move
        t2 = Timer
        accum = (t2 - t1)
        Print
        Print "Total node count = "; nodes
        Print "Time = "; accum; "s"
        Print "Average nodes/sec = "; Int(nodes / accum)
        Sleep
        perft = FALSE
        display
      End If
      If o_str = "black" Then
        side = black
        cmpclr = 1                               'NOmega plays white
        display
      End If
      If o_str = "white" Then
        side = white
        cmpclr = -1                             'NOmega plays black
        display
      End If
      If o_str = "go" Then
        If side = white Then cmpclr = 1
        If side = black Then cmpclr = -1
        computer_move
        display
        If assessboard(side, best_score) = TRUE Then ending
        side = -side
      End If
      If o_str = "listgame" Then
        For j = 1 To MoveNo
          Print j; " "; move_2_str(move_hist(j, 1)); "-"; move_2_str(move_hist(j, 2)); "   ";
          If move_hist(j, 3) = 0 Then Print : Exit For
          Print move_2_str(move_hist(j, 3)); "-"; move_2_str(move_hist(j, 4))
        Next j
        Print "hit any key"
        Sleep
        display
      End If
      If o_str = "level" Then
        Input "depth"; er
        If er > c_depth Then                     ' ### max_depth can not exceed c_depth nor be below 1
          er = c_depth
        ElseIf er < 1 Then
          er = 1
        End If
        max_depth = er
        fix_depth = 1
      End If
      If o_str = "help" Then
        help
        display
        o_str = ""                           'GoTo ue1
      End If
      If o_str = "save" Then
        SaveGame
        display
        o_str = ""                           'GoTo ue1
      End If
      If o_str = "load" Then
        LoadGame
        display
        o_str = ""                           'GoTo ue1
      End If
      If o_str = "end" Then End

      'If Len(o_str) <> 4 then GoTo ue1
    Loop While Len(o_str) <> 4

    MoveFrom = o_str[0] - 96 + 10 * (o_str[1] - 47)
    MoveTo = o_str[2] - 96 + 10 * (o_str[3] - 47)

  End If
  '========================================
  ' removed code, code is now in commandloop
  '========================================
  'ue2:

End Sub


'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
Function user_move(MoveFrom As UInteger, MoveTo As UInteger) As Integer

  Dim As Integer w, ps    ', thru_check
  Dim As Integer temp_user_var                      ', Check_root
  Dim As String promotion_str

  If side = white Then MoveNo += 1

  ' increment move counter used for time management 'replaced from dev6 16/2/10
  If WinboardMode = 1 And ForceMode = 1 And ((side = white And cmpclr = 1) Or (side = black And cmpclr = -1)) Then conv_clock_move_no += 1

  depth = 1
  ' ### changed that a little
  b_casflags(0, 1) = b_cas(1) : b_casflags(0, 2) = b_cas(2)    ' need to reset search flags for ply 3 before generating moves to test legality
  w_casflags(0, 1) = w_cas(1) : w_casflags(0, 2) = w_cas(2)    ' only need depth = 3 as genmove tests casflags at previous move - ie depth = 3

  If act_epsq <> - 1 Then epsqr(depth) = act_epsq'set ep sqr if computer last moved pawn 2 squares

  gen_move(side, depth)

  valid_move = FALSE

  For w = 1 To Pseu_Moves(depth)                 'run through valid moves - if move entry is found then move is valid
    '  Print Move_List(depth,w,1);"-";Move_List(depth,w,2)
    If MoveFrom = Move_List(depth, w, 1) And MoveTo = Move_List(depth, w, 2) Then
      valid_move = TRUE
      'w = Pseu_Moves(depth)
      Exit For                                   ' ###
    End If
  Next w

  'handle special moves
  'If side = black Then GoTo um1
  If side = white Then
    If MoveFrom = 25 And MoveTo = 27 And B(25) = 7 And w_cas(1) = 0 Then
      '===================================
      ' removed code, test now in gen_move
      '===================================
      B(28) = 0
      B(26) = 4                                  'move rook
      'w_cas(1) = 1                               'set white king castle flag
      ' End If
    End If

    If MoveFrom = 25 And MoveTo = 23 And B(25) = 7 And w_cas(2) = 0 Then
      '===================================
      ' removed code, test now in gen_move
      '===================================
      B(21) = 0
      B(24) = 4                                  'move rook
      'w_cas(2) = 1                               'set white king castle flag (queen side)
      ' End If
    End If

    'negate castling option if piece move is previously unmoved rook or king
    'If MoveFrom = 21 And B(MoveFrom) = 4 Then w_cas(2) = -1
    'If MoveFrom = 28 And B(MoveFrom) = 4 Then w_cas(1) = -1
    'If MoveFrom = 25 And B(MoveFrom) = 7 Then    ''''And (w_cas(1) <> 1 Or w_cas(2) <> 1) Then
    'w_cas(1) = -1
    'w_cas(2) = -1
    'End If

    ' set ep sqr if relevant
    If B(MoveFrom) = 1 And (MoveTo - MoveFrom = 20) Then act_epsq = MoveTo - 10 Else act_epsq = -1

    ' GoTo um2

    ' um1:
  Else
    If MoveFrom = 95 And MoveTo = 97 And B(95) = -7 And b_cas(1) = 0 Then
      '===================================
      ' removed code, test now in gen_move
      '===================================

      B(98) = 0
      B(96) = -4                                'move rook
      ' b_cas(1) = 1                               'set black king castle flag
      ' End If
    End If

    If MoveFrom = 95 And MoveTo = 93 And B(95) = -7 And b_cas(2) = 0 Then
      '===================================
      ' removed code, test now in gen_move
      '===================================

      B(91) = 0
      B(94) = -4                                'move rook
      'b_cas(2) = 1                               'set black king castle flag (queen side)
      ' End If
    End If

    'If MoveFrom = 91 And B(MoveFrom) = -4 Then b_cas(2) = -1
    'If MoveFrom = 98 And B(MoveFrom) = -4 Then b_cas(1) = -1
    'If MoveFrom = 95 And B(MoveFrom) = -7 Then  ''''And (b_cas(1) <> 1 Or b_cas(2) <> 1) Then
    '  b_cas(1) = -1
    ' b_cas(2) = -1
    'End If

    'If B(MoveFrom) = -1 And (MoveTo - MoveFrom = -20) Then act_epsq = MoveTo + 10 Else act_epsq = -1
    ' ### rearranged test GCC 64x (warning 36(0): Mixing signed/unsigned operands)
    If B(MoveFrom) = -1 And (MoveFrom -20 = MoveTo) Then act_epsq = MoveTo + 10 Else act_epsq = -1
    'make move....
    'um2:
  End If
  temp_user_var = B(MoveTo)
  B(MoveTo) = B(MoveFrom)
  B(MoveFrom) = 0

  'keep record of king location

  If side = white Then
    If B(MoveTo) = 7 Then wkloc = MoveTo
    If wkloc <> 25 Or b(28) <> 4 Then w_cas(1) = -1
    If wkloc <> 25 Or b(21) <> 4 Then w_cas(2) = -1
  Else
    If B(MoveTo) = -7 Then bkloc = MoveTo
    If bkloc <> 95 Or b(98) <> -4 Then b_cas(1) = -1
    If bkloc <> 95 Or b(91) <> -4 Then b_cas(2) = -1
  End If

  'If B(MoveTo) = 7 Then wkloc = MoveTo Else If B(MoveTo) = -7 Then bkloc = MoveTo

  Dim As Integer Check_root = incheck(side)         ' test to see if move results in check

  If Check_root = TRUE Then valid_move = FALSE   ' if so then illegal and take-back move

  'valid move if get out check by PxPep!
  If (Check_root = TRUE And B(MoveTo) = 1 And MoveTo = epsqr(depth)) Or (Check_root = TRUE And B(MoveTo) = -1 And MoveTo = epsqr(depth)) Then valid_move = TRUE

  If valid_move = FALSE And WinboardMode = 0 Then
    MoveNo = MoveNo - 1
    Print "invalid entry.."
    B(MoveFrom) = B(MoveTo)
    B(MoveTo) = temp_user_var
    ' If B(MoveFrom) = 7 Then wkloc = MoveFrom
    ' If B(MoveFrom) = -7 Then bkloc = MoveFrom
    ' ### undo castling moves
    If side = white Then
      If b(movefrom) = 7 Then
        wkloc = movefrom
        If wkloc = 25 Then
          If moveto = 27 Then
            b(26) = 0 : b(28) = 4
          ElseIf moveto = 23 Then
            b(24) = 0 : b(21) = 4
          End If
        End If
        w_cas(1) = w_casflags(0, 1) ' ### restore original castling flags
        w_cas(2) = w_casflags(0, 1)
      End If
    Else
      If b(movefrom) = -7 Then
        bkloc = movefrom
        If bkloc = 95 Then
          If moveto = 97 Then
            b(96) = 0 : b(98) = -4
          ElseIf moveto = 93 Then
            b(94) = 0 : b(91) = -4
          End If
        End If
        b_cas(1) = b_casflags(0, 1)
        b_cas(2) = b_casflags(0, 2)  ' ### restore original castling flags
      End If
    End If
    Return valid_move                            ' GoTo um3
  End If

  If valid_move = FALSE And WinboardMode = 1 Then
    MoveNo = MoveNo - 1
    B(MoveFrom) = B(MoveTo)
    B(MoveTo) = temp_user_var
    If B(MoveFrom) = 7 Then wkloc = MoveFrom
    If B(MoveFrom) = -7 Then bkloc = MoveFrom
    SendCommand( "Illegal move: " + o_str)
    Exit Function
  End If

  If B(MoveTo) = 1 Or B(MoveTo) = -1 Or temp_user_var <> 0 Then fifty_move = 0 Else fifty_move += 1

  If temp_user_var <> 0 Then No_Pieces -= 1      'if piece is taken then reduce number of pieces on board

  ' reduce material value depending on which piece has been taken...
  If Abs(temp_user_var) = 6 Then
    Mat_left -= Queen_Value
    If temp_user_var = 6 Then WQ = 0 Else BQ = 0 ' ### should be -= 1 there can be more then 1 queen
  End If

  If Abs(temp_user_var) = 4 Then
    Mat_left -= Rook_Value
    If temp_user_var = 4 Then wr -= 1 Else br -= 1
  End If

  If Abs(temp_user_var) = 3 Then Mat_left -= Bishop_Value
  If Abs(temp_user_var) = 2 Then Mat_left -= Knight_Value

  ' ...in winboard mode
  If WinboardMode = 1 And B(MoveTo) = 1 And MoveTo > 90 Then
    If pr_str = "q" or pr_str = "Q" Then B(MoveTo) = 6
    If pr_str = "r" or pr_str = "R" Then B(MoveTo) = 4
    If pr_str = "b" or pr_str = "B" Then B(MoveTo) = 3
    If pr_str = "n" or pr_str = "N" Then B(MoveTo) = 2
  End If

  If WinboardMode = 1 And B(MoveTo) = -1 And MoveTo < 29 Then
    If pr_str = "q" or pr_str = "Q" Then B(MoveTo) = -6
    If pr_str = "r" or pr_str = "R" Then B(MoveTo) = -4
    If pr_str = "b" or pr_str = "B" Then B(MoveTo) = -3
    If pr_str = "n" or pr_str = "N" Then B(MoveTo) = -2
  End If

  ' ...in console mode  ' ### what if, user does not type q/r/b/n
  If B(MoveTo) = 1 And MoveTo > 90 Then
    Input "Promotion (q/r/b/n)"; promotion_str
    If promotion_str = "q" Then B(MoveTo) = 6
    If promotion_str = "r" Then B(MoveTo) = 4
    If promotion_str = "b" Then B(MoveTo) = 3
    If promotion_str = "n" Then B(MoveTo) = 2
  End If

  If B(MoveTo) = -1 And MoveTo < 29 Then
    Input "Promotion (q/r/b/n)"; promotion_str
    If promotion_str = "q" Then B(MoveTo) = -6
    If promotion_str = "r" Then B(MoveTo) = -4
    If promotion_str = "b" Then B(MoveTo) = -3
    If promotion_str = "n" Then B(MoveTo) = -2
  End If

  ' ### need to update mat_left and other piece counters to reflect new piece
  ' ### if pawn removed then alter knight and rook value

  ' remove black pawn if white move is pxp ep
  If B(MoveTo) = 1 And MoveTo = epsqr(depth) Then B(MoveTo - 10) = 0

  ' remove white pawn if black move is pxp ep
  If B(MoveTo) = -1 And MoveTo = epsqr(depth) Then B(MoveTo + 10) = 0


  If side = white Then                           ' update move history with current move
    move_hist(MoveNo, 1) = MoveFrom
    move_hist(MoveNo, 2) = MoveTo
  ElseIf side = black Then
    move_hist(MoveNo, 3) = MoveFrom
    move_hist(MoveNo, 4) = MoveTo
  End If

  STM = computer

  If winboardmode = 0 Then display

  If winboardmode = 1 And ForceMode = 0 Then
    If assessboard(side, best_score) = TRUE Then Exit Function
    side = -side
    ' ### check now made in computer_move
    'If MoveNo > 8 Then                           ' max book length is 8 moves
    computer_move
    'ElseIf open_book = FALSE Then                ' make computer move if no book move is found
    'computer_move(side)
    'End If
    If assessboard(side, best_score) = TRUE Then Exit Function
    side = -side
  End If

  If winboardmode = 1 And ForceMode = 1 Then
      side = -side
      epsqr(depth) = 0
      end if

  'um3:
  Return valid_move

End Function

'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
Sub computer_move

  If MoveNo < 9 And perft = FALSE And open_book = TRUE Then Exit Sub

  Dim As Double ply_end_time, ply_start_time, test_time  ', elp_time, accum1
  Dim As Integer root_sort(100)                  ', r1

  Dim As ULongInt total_nodes
  ' Dim As Byte Weg_condition1, Beg_condition1
  Dim As Integer prom_last_ply, Search_Condition1
  Dim As Integer root, Check_Root, prom_piece ', thru_check
  ' Dim As Byte check_rep
  Dim As Integer B_M_From, B_M_To, unsorted
  ' Dim As String j11_str,  k11_str,  j21_str,  j31_str, k21_str, k31_str
  'Dim As String BMFROM_str, BMTO_str  ' ### no need for these any more
  Dim As Integer B_M_To_easy_move, B_M_From_easy_move
  Dim As Integer t, v, w, ps, pe
  Dim As String Prom_Piece_str

  If perft = TRUE Then                           ' ### Freebasic doesn't like jumping in a FOR NEXT loop
    'it_depth = 1
    v = -99
    GoTo cmhelper
  End If

  If side = white Then MoveNo += 1

  If forcemode = 1 Then Exit Sub

  'If fix_depth = 0 Then ply_start_time = Timer
  ply_start_time = Timer  ' ### always

  start_time = ply_start_time                    ' Int(ply_start_time)

  timed_out = 0                                  ' flag to cut search if used too much time

  If endgame = 1 And fix_depth = 0 Then          ' in K+Q/R v K limit search to 5 ply unless opponent king is on board edge
    ' Weg_condition1 = (No_pieces = 3 And (wr = 1 Or WQ = 1))
    ' Beg_condition1 = (No_pieces = 3 And (br = 1 Or BQ = 1))
    ' If side = black And Beg_condition1 And K_r_end_PST(wkloc) < 45 Then max_depth = 5 Else max_depth = 15
    ' If side = white And Weg_condition1 And K_r_end_PST(bkloc) < 45 Then max_depth = 5 Else max_depth = 15
    If No_pieces = 3 Then                        ' ###
      If side = black And (br = 1 Or BQ = 1) And K_r_end_PST(wkloc) < 45 Then max_depth = 5 Else max_depth = 15
      If side = white And (wr = 1 Or WQ = 1) And K_r_end_PST(bkloc) < 45 Then max_depth = 5 Else max_depth = 15
    End If                                       ' ###
  End If

  cmhelper:

  For it_depth = 1 To max_depth

    If v = -99 Then GoTo cm    ' ### it_depth is made 1, jump to label cm

    'prom_depth = 0 ' reset variable to store depth of promotion ' ### not used ???

    prom_last_ply = 0

    depth = it_depth

    search_depth = depth
    cm:
    ' best_score = -9000                          '-10000 ### -10000 triggers a early breakoff
    best_score = - 10000 ' ### FB_numpty

    w_casflags(0, 1) = w_cas(1)          '###
    w_casflags(0, 2) = w_cas(2)          '###
    b_casflags(0, 1) = b_cas(1)          '###
    b_casflags(0, 2) = b_cas(2)          '###

    epsqr(depth) = act_epsq                      'update epsqr in search with results of last move

    Search_Condition1 = (perft = FALSE And it_depth = 1)
    ' generate moves at root

    If (Search_Condition1 Or perft = TRUE) Then gen_move(side, depth) Else Pseu_Moves(depth) = root

    root = 0
    nodes = 0                                    ' re-set node counter

    IM = 0                                       ' insufficient material flag

    For w = 1 To Pseu_Moves(depth)               ' loop through root moves

      'thru_check = FALSE                         ' added 18/2/10 - flag is needed to prevent castle when in check at root

      Check_Root = FALSE                         ' 14/12 - need this line elseif  in check at root only best move at ply 1 is chosen
      fmove(depth) = fifty_move

      Alpha = -9999
      beta = 9999

      prom_piece = 0                             'reset promotion flag (re-set for each root move therefore will pick up each different promotion piece)

      For t = 1 To depth

        wprom(t) = 0 : bprom(t) = 0              ' rest w/b promotion flags
        '#######################
        'For v = 1 To 2
        'w_casflags(depth,v) = w_cas(v):b_casflags(depth,v) = b_cas(v)   ' reset castle flags to original values
        ' w_casflags(depth,v) = w_cas(v)   ' ###
        ' b_casflags(depth,v) = b_cas(v)   ' ###
        ' Next v

        w_casflags(t, 1) = w_cas(1)          ' ###
        w_casflags(t, 2) = w_cas(2)          ' ###
        b_casflags(t, 1) = b_cas(1)          ' ###
        b_casflags(t, 2) = b_cas(2)          ' ###

        'if last opponent move has generated epsq then set this for search else reset ep_square
        If act_epsq <> - 1 And t = depth Then epsqr(t) = act_epsq Else epsqr(t) = -1

      Next t

      nodes_root_move = nodes                    ' var to calc nodes per individual root move

      ' EP at root

      ' set ep field for root move - if white
      If (Move_List(depth, w, 2) - Move_List(depth, w, 1) = 20) And B(Move_List(depth, w, 1)) = 1 And (B(Move_List(depth, w, 2) - 1) = -1 Or B(Move_List(depth, w, 2) + 1) = -1) Then
        'epsqr will be set for ply 2
        epsqr(depth - 1) = Move_List(depth, w, 1) + 10
      End If

      ' set ep field for root move - if black
      If (Move_List(depth, w, 2) - Move_List(depth, w, 1) = -20) And B(Move_List(depth, w, 1)) = -1 And (B(Move_List(depth, w, 2) - 1) = 1 Or B(Move_List(depth, w, 2) + 1) = 1) Then
        'epsqr will be set for ply 2
        epsqr(depth - 1) = Move_List(depth, w, 1) - 10
      End If

      'hold contents of root move move to sq
      temp_root_move_val = B(Move_List(depth, w, 2))
      ' make move at root
      B(Move_List(depth, w, 2)) = B(Move_List(depth, w, 1))
      B(Move_List(depth, w, 1)) = 0              ' empty move from sq

      cap_sq = Move_List(depth, w, 2)            ' hold root capture sq for swap-off
      temp_val1(1) = temp_root_move_val          ' hold root capture piece for swap-off
      init_swp_val = B(Move_List(depth, w, 2))

      ' code below reduces material and piece count remaining if piece has been captured at root

      If temp_root_move_val <> 0 Then No_Pieces -= 1

      If Abs(temp_root_move_val) = 6 Then
        Mat_left -= Queen_Value
        If temp_root_move_val = 6 Then WQ = 0 Else BQ = 0
      End If

      If Abs(temp_root_move_val) = 4 Then Mat_left -= Rook_Value
      If Abs(temp_root_move_val) = 3 Then Mat_left -= Bishop_Value
      If Abs(temp_root_move_val) = 2 Then Mat_left -= Knight_Value
      If Abs(temp_root_move_val) = 1 Then
        Knight_Value -= 1                        'Kn loses value as number of pawns reduce
        Rook_Value += 1                          'Rk value increases as pawns reduce
      End If

      If temp_root_move_val <> 0 Or B(Move_List(depth, w, 2)) = 1 Or B(Move_List(depth, w, 2)) = -1 Then
        fmove(depth) = 0
      Else
        fmove(depth) = fifty_move + 1
      End If

      ' take off white pawn after black pxp ep
      If Move_List(depth, w, 2) = act_epsq And B(Move_List(depth, w, 2)) = -1 Then B(Move_List(depth, w, 2) + 10) = 0
      ' take off pawn after white pxp ep
      If Move_List(depth, w, 2) = act_epsq And B(Move_List(depth, w, 2)) = 1 Then B(Move_List(depth, w, 2) - 10) = 0

      ' check for castling at root
      'If side = black Then GoTo cm1
      If side = white Then
        If B(Move_List(depth, w, 2)) = 7 Then wkloc = Move_List(depth, w, 2)
        ' test for white O-O
        If Move_List(depth, w, 1) = 25 And Move_List(depth, w, 2) = 27 And B(27) = 7 Then
          B(28) = 0 : B(26) = 4                  ' move rook
          ' w_casflags(depth, 1) = 1               ' set white king castle flag
        End If

        ' test for white O-O-O
        If Move_List(depth, w, 1) = 25 And Move_List(depth, w, 2) = 23 And B(23) = 7 Then
          B(21) = 0 : B(24) = 4
          ' w_casflags(depth, 2) = 1               ' set white queen castle flag
        End If

        ' If w_casflags(depth, 1) <> 1 And
        If (B(25) <> 7 Or B(28) <> 4) Then w_cas(1) = -1
        ' If w_casflags(depth, 2) <> 1 And
        If (B(25) <> 7 Or B(21) <> 4) Then w_cas(2) = -1

        If Move_List(depth, w, 2) > 90 And B(Move_List(depth, w, 2)) = 1 Then
          wprom(depth) = 1                       'set promotion flag
          If Move_List(depth, w, 3) = 6 Then B(Move_List(depth, w, 2)) = 6
          If Move_List(depth, w, 3) = 4 Then B(Move_List(depth, w, 2)) = 4
          If Move_List(depth, w, 3) = 3 Then B(Move_List(depth, w, 2)) = 3
          If Move_List(depth, w, 3) = 2 Then B(Move_List(depth, w, 2)) = 2
        End If

        'GoTo cm2
      Else
        'cm1:

        If B(Move_List(depth, w, 2)) = -7 Then bkloc = Move_List(depth, w, 2)

        ' test for black O-O
        If Move_List(depth, w, 1) = 95 And Move_List(depth, w, 2) = 97 And B(97) = -7 Then
          B(98) = 0 : B(96) = -4
          ' b_casflags(depth, 1) = 1
        End If

        ' test for black O-O-O
        If Move_List(depth, w, 1) = 95 And Move_List(depth, w, 2) = 93 And B(93) = -7 Then

          B(91) = 0 : B(94) = -4
          ' b_casflags(depth, 2) = 1
        End If

        'If b_casflags(depth, 1) <> 1 And
        If (B(95) <> -7 Or B(98) <> -4) Then b_cas(1) = -1
        'If b_casflags(depth, 2) <> 1 And
        If (B(95) <> -7 Or B(91) <> -4) Then b_cas(2) = -1

        If Move_List(depth, w, 2) < 29 And B(Move_List(depth, w, 2)) = -1 Then
          bprom(depth) = 1                       'set promotion flag
          If Move_List(depth, w, 3) = -6 Then B(Move_List(depth, w, 2)) = -6
          If Move_List(depth, w, 3) = -4 Then B(Move_List(depth, w, 2)) = -4
          If Move_List(depth, w, 3) = -3 Then B(Move_List(depth, w, 2)) = -3
          If Move_List(depth, w, 3) = -2 Then B(Move_List(depth, w, 2)) = -2
        End If

        'cm2:
        'Print "make root move.."
        '   Print
        '
        'FOR X = 9 TO 2 STEP -1
        '    FOR Yl = 1 TO 8
        '        Print B(X*10+Yl);
        '
        '    NEXT Yl
        '    print
        'NEXT X
        'Print
        'sleep
      End If
      'cm2:

      ' test to see if move results in check
      'If it_depth = 1 Then Check_root = incheck(side)

      ' if so then illegal and take-back move (jump to cm2a to enable reverse of pawn prom if in check - 7/11/08
      ' If it_depth = 1 And Check_root = TRUE Then GoTo cm2a

      If it_depth = 1 And incheck(side)= TRUE Then GoTo cm2a
      ' ### not the correct spot for checking the 50 move rule
      ' If fmove(depth) = 100 Then               'score = if 50 move rule is tripped
      '   score = 0
      '   GoTo cm2x
      '  End If

      ' condition to determine if endgame eval is engaged
      If (ENDGAME = 1 Or Mat_Left < 2426) Then ENDGAME = 1 Else ENDGAME = 0

      '====================================================
      w_casflags(depth, 1) = w_cas(1)  ' ###
      w_casflags(depth, 2) = w_cas(2)
      b_casflags(depth, 1) = b_cas(1)
      b_casflags(depth, 2) = b_cas(2)
      '====================================================

      score = -ab_search(-side, depth - 1, Alpha, beta, fmove())

      '====================================================
      w_cas(1) = w_casflags(depth, 1)  ' ###
      w_cas(2) = w_casflags(depth, 2)
      b_cas(1) = b_casflags(depth, 1)
      b_cas(2) = b_casflags(depth, 2)
      '====================================================

      If (No_Pieces = 3 And (Bbp + Wbp + Bkn + Wkn = 1)) Or (No_Pieces = 4 And (Bbp + Bkn = 1 And Wbp + Wkn = 1)) Then score = 0'bad_score
      If No_Pieces = 2 Then score = 0'bad_score

      ' test for move repitition
      If search_depth > 4 And score > best_score And Check_root = FALSE Then
        ' check_rep = rep_check(side)
        ' If check_rep = TRUE Then score = 0
        If rep_check = TRUE Then score = 0'bad_score

      End If

      cm2x:

      root += 1

      If it_depth = 1 Then orig_root = root

      root_sort(root) = score

      Move_list_root(depth, root, 1) = Move_List(depth, w, 1)
      Move_list_root(depth, root, 2) = Move_List(depth, w, 2)
      Move_list_root(depth, root, 3) = Move_List(depth, w, 3)

      If temp_root_move_val <> 0 Or B(Move_List(depth, w, 2)) = 1 Or B(Move_List(depth, w, 2)) = -1 Then
        fmove(depth) = fifty_move
      Else
        fmove(depth) -= 1
      End If

      If perft = TRUE Then
        'Print Move_List(depth,w,1);"-"; Move_List(depth,w,2);
        'BMFROM_str = Chr(Move_List(depth,w,1) Mod 10 + 96) + Chr(Move_List(depth,w,1) \ 10 + 47)
        'BMTO_str = Chr(Move_List(depth,w,2) Mod 10 + 96) + Chr(Move_List(depth,w,2) \ 10 + 47)

        ' BMFROM_str = move_2_str(Move_List(depth, w, 1))
        'BMTO_str = move_2_str(Move_List(depth, w, 2))
        ' Print BMFROM_str; "-"; BMTO_str;

        Print move_2_str(Move_List(depth, w, 1)); "-"; move_2_str(Move_List(depth, w, 2)); _
        " "; nodes - nodes_root_move; " nodes"
      End If
      ' Print "Depth = ";depth; " ";Move_List(depth,w,1);"-"; Move_List(depth,w,2); " ";nodes - nodes_root_move ; " nodes"; " "; "score = ";score'; " bkloc = ";bkloc

      'sleep 60000
      cm2a:

      If Move_List(depth, w, 2) > 90 Then
        If B(Move_List(depth, w, 2)) = 6 And Move_List(depth, w, 3) = 6 And wprom(depth) = 1 Then B(Move_List(depth, w, 2)) = 1
        If B(Move_List(depth, w, 2)) = 4 And Move_List(depth, w, 3) = 4 And wprom(depth) = 1 Then B(Move_List(depth, w, 2)) = 1
        If B(Move_List(depth, w, 2)) = 3 And Move_List(depth, w, 3) = 3 And wprom(depth) = 1 Then
          B(Move_List(depth, w, 2)) = 1
          Wbp -= 1                               ' see explanation in make move function - don't need to ++ piece no in make move as eval counts number of pieces by type
        End If
        If B(Move_List(depth, w, 2)) = 2 And Move_List(depth, w, 3) = 2 And wprom(depth) = 1 Then
          B(Move_List(depth, w, 2)) = 1
          Wkn -= 1
        End If
      End If

      If Move_List(depth, w, 2) < 29 Then
        If B(Move_List(depth, w, 2)) = -6 And Move_List(depth, w, 3) = -6 And bprom(depth) = 1 Then B(Move_List(depth, w, 2)) = -1
        If B(Move_List(depth, w, 2)) = -4 And Move_List(depth, w, 3) = -4 And bprom(depth) = 1 Then B(Move_List(depth, w, 2)) = -1
        If B(Move_List(depth, w, 2)) = -3 And Move_List(depth, w, 3) = -3 And bprom(depth) = 1 Then
          B(Move_List(depth, w, 2)) = -1
          Bbp -= 1
        End If
        If B(Move_List(depth, w, 2)) = -2 And Move_List(depth, w, 3) = -2 And bprom(depth) = 1 Then
          B(Move_List(depth, w, 2)) = -1
          Bkn -= 1
        End If
      End If

      ' unmake root move

      ' cm3:
      ' unmake move at root
      B(Move_List(depth, w, 1)) = B(Move_List(depth, w, 2))
      'replace original contents of sq
      B(Move_List(depth, w, 2)) = temp_root_move_val

      If temp_root_move_val <> 0 Then No_Pieces += 1

      If Abs(temp_root_move_val) = 6 Then
        Mat_left += Queen_Value
        If temp_root_move_val = 6 Then WQ = 1 Else BQ = 1
      End If

      If Abs(temp_root_move_val) = 4 Then Mat_left += Rook_Value
      If Abs(temp_root_move_val) = 3 Then Mat_left += Bishop_Value
      If Abs(temp_root_move_val) = 2 Then Mat_left += Knight_Value
      If Abs(temp_root_move_val) = 1 Then
        Knight_Value += 1
        Rook_Value -= 1
      End If

      ' replace white pawn after undo root move of black pxp ep
      If Move_List(depth, w, 2) = act_epsq And B(Move_List(depth, w, 1)) = -1 Then B(Move_List(depth, w, 2) + 10) = 1

      ' replace black pawn after undo root move of white pxp ep
      If Move_List(depth, w, 2) = act_epsq And B(Move_List(depth, w, 1)) = 1 Then B(Move_List(depth, w, 2) - 10) = -1

      ' If B(Move_List(depth, w, 1)) = 7 Then wkloc = Move_List(depth, w, 1)
      ' If B(Move_List(depth, w, 1)) = -7 Then bkloc = Move_List(depth, w, 1)

      ' If thru_check = TRUE Then GoTo cm3a ' ### check for castling are made in gen_move

      ' if K-side castled then unmake rook move at root
      'If side = white And w_casflags(depth, 1) = 1 And w_cas(1) = 0 Then
      '  B(26) = 0 : B(28) = 4
      ' w_casflags(depth, 1) = 0               ' reset kingside castle flag
      'corrected 24/3/09 thanks Leo!
      ' ElseIf side = black And b_casflags(depth, 1) = 1 And b_cas(1) = 0 Then
      ' B(96) = 0 : B(98) = -4
      ' b_casflags(depth, 1) = 0
      ' End If

      'if Q-side castled then unmake rook move at root
      ' If side = white And w_casflags(depth, 2) = 1 And w_cas(2) = 0 Then
      '  B(24) = 0 : B(21) = 4
      '  w_casflags(depth, 2) = 0                 ' reset queenside castle flag
      ' ElseIf side = black And b_casflags(depth, 2) = 1 And b_cas(2) = 0 Then
      ' B(94) = 0 : B(91) = -4
      ' b_casflags(depth, 2) = 0
      ' End If

      If B(Move_List(depth, w, 1)) = 7 Then
        wkloc = Move_List(depth, w, 1)
        If wkloc = 25 Then
          If move_list(depth, w, 2) = 27 Then
            B(26) = 0 : B(28) = 4
          ElseIf move_list(depth,w,2) = 23 Then
            B(24) = 0 : B(21) = 4
          End If
        End If
      End If

      If B(Move_List(depth, w, 1)) = -7 Then
        bkloc = Move_List(depth, w, 1)
        If bkloc = 95 Then
          If move_list(depth,w, 2) = 97 Then
            B(96) = 0 : B(98) = -4
          ElseIf move_list(depth,w,2) = 93 Then
            B(94) = 0 : B(91) = -4
          End If
        End If
      End If

      '====================================================
      w_cas(1) = w_casflags(0, 1) ' ###
      w_cas(2) = w_casflags(0, 2)
      b_cas(1) = b_casflags(0, 1)
      b_cas(2) = b_casflags(0, 2)
      '====================================================

      If perft = TRUE Then Continue For          ' GoTo cm4
      '
      '            Print "unmake root move.."
      '   Print
      '
      'FOR X = 9 TO 2 STEP -1
      '    FOR Yl = 1 TO 8
      '        Print B(X*10+Yl);
      '
      '    NEXT Yl
      '    print
      'NEXT X
      'Print
      'sleep


      If timed_out = 1 Then                      ' if run out of time then finish search
        ' w = Pseu_Moves(depth)
        it_depth = max_depth
        GoTo it6
      End If

      ' if new best score then
      '  If score > best_score And Check_root = FALSE Then
      If root > 0 And score > best_score And check_root = FALSE Then ' FB_Numpty

        best_score = score' set new best-score
        bestscore(MoveNo) = best_score  ' update history of bestscores to be able to assess whether to resign

        B_M_From = Move_List(depth, w, 1)
        B_M_To = Move_List(depth, w, 2)
        Pe = B(Move_List(depth, w, 1))
        'reset promotion flag if promotion is no longer best move in current ply
        'If prom_last_ply = 1 Then prom_last_ply = 0  ' ###
        prom_last_ply = 0
        If side = white And wprom(depth) = 1 Then
          prom_last_ply = 1
          Pe = Move_List(depth, w, 3)            'if promoted piece then update sqr with new piece
          Move_List(depth, w, 3) = 0             ' reset prom piece else 2nd prom with Q on back rank turns Q back to P! 14/11/08
        End If
        If side = black And bprom(depth) = 1 Then
          prom_last_ply = 1
          Pe = Move_List(depth, w, 3)            'if promoted piece then update sqr with new piece
          Move_List(depth, w, 3) = 0             ' reset prom piece else 2nd prom with Q on back rank turns Q back to P! 14/11/08
        End If
        If it_depth > 3 Then                     ' for depth > 3 then display pv etc
          '  elp_time = Timer
          ' accum1 = (elp_time - ply_start_time)
          ' BMFROM_str = move_2_str(Move_List(depth, w, 1))
          ' BMTO_str = move_2_str(Move_List(depth, w, 2))
          ' If post = 1 Then Print it_depth; " "; best_score; " "; CInt((elp_time - ply_start_time) * 100); " "; total_nodes + nodes; "  "; BMFROM_str; BMTO_str
          '###

          If post = 1 Then
            Print it_depth; " "; best_score; " "; CInt((Timer - ply_start_time) * 100);" "; _
            total_nodes + nodes; "  "; move_2_str(Move_List(depth, w, 1)); move_2_str(Move_List(depth, w, 2))
          End If
        End If
      End If

      'display_node = total_nodes + nodes         '###

      ' above 3 ply following lines attempt to implement some crude reductions in root moves searched at deeper ply based on score
      If root > 3 Then   ' ### root must be greater then 1 else
        'If it_depth = 3 And score - best_score < - 500 Then root -= 1
        If it_depth = 4 And score - best_score < - 475 Then root -= 1  '275
        If it_depth = 5 And score - best_score < - 200 Then root -= 1  ' 255
        If it_depth = 6 And score - best_score < - 175 Then root -= 1  ' 200
        If it_depth > 6 And score - best_score < - 150 Then root -= 1  ' 175
      End If ' ###

      If best_score > 9980 Then                  ' if mate is found then no need to search further!
        w = Pseu_Moves(depth)
        it_depth = max_depth
      End If

      cm3a:

      If fix_depth = 1 Then Continue For         ' GoTo cm4
      /'
      test_time = Timer

      ' if used more than think time for move then stop searching unless almost at the end of current iteration
      ' test here rather than at start of move so that after make move.

      'ply_end_time? = original -  2.2 26/10
      If conv_clock = 1 And ((test_time - ply_start_time) * 100) > 2.5 * av_think_time Then
        ' w = Pseu_Moves(depth)
        it_depth = max_depth
        GoTo it6
      End If

      '0.9  26/10/09 'changed to 0.95 - 4/07/10
      If (((test_time - ply_start_time) * 100) >= (0.95 * think_time)) Then
        ' w = Pseu_Moves(depth)
        it_depth = max_depth
        GoTo it6
      End If
      ' next line added dev3 - 14/1/2010
      '0.9  26/10/09
      If (((test_time - ply_start_time) * 100) >= (0.35 * time_left)) Then
        ' w = Pseu_Moves(depth)
        it_depth = max_depth
        GoTo it6
      End If
'/

      ' ### reworked some code
      test_time = (Timer - ply_start_time) * 100

      ' if used more than think time for move then stop searching unless almost at the end of current iteration
      ' test here rather than at start of move so that after make move.

      'ply_end_time? = original -  2.2 26/10
      If conv_clock = 1 And (test_time > (2.5 * av_think_time)) Then
        it_depth = max_depth
        GoTo it6
      End If
      '0.9  26/10/09 'changed to 0.95 - 4/07/10
      If test_time >= (0.95 * think_time) Then
        it_depth = max_depth
        GoTo it6
      End If
      ' next line added dev3 - 14/1/2010
      '0.9  26/10/09
      If test_time >= (0.35 * time_left) Then
        it_depth = max_depth
        GoTo it6
      End If

      'cm4:

    Next w

    If perft = TRUE Then Exit Sub                'GoTo it7

    ' if only 1 legal move, or close to mate then no point searching further!
    If (it_depth = 1 And root = 1) Or best_score < - 9993 Then it_depth = max_depth

    If it_depth = max_depth Then GoTo it6

    Do                                           ' bubble sort moves according to score ready for next ply
      unsorted = 0

      For t = 1 To root - 1

        If root_sort(t) < root_sort(t + 1) Then
          Swap root_sort(t), root_sort(t + 1)    ' ### used swap
          Swap Move_list_root(depth, t, 1), Move_list_root(depth, t + 1, 1)
          Swap Move_list_root(depth, t, 2), Move_list_root(depth, t + 1, 2)
          Swap Move_list_root(depth, t, 3), Move_list_root(depth, t + 1, 3)
          unsorted = 1
        End If

      Next t

    Loop While unsorted

    'it3:

    For t = 1 To root
      Move_list(depth + 1, t, 1) = Move_list_root(depth, t, 1)
      Move_list(depth + 1, t, 2) = Move_list_root(depth, t, 2)
      Move_list(depth + 1, t, 3) = Move_list_root(depth, t, 3)
    Next t

    If depth = 1 And root_sort(1) - root_sort(2) > 250 Then
       B_M_To_easy_move = B_M_To
       B_M_From_easy_move = B_M_From
      'B_M_To_easy_move = move_list_root(depth,1,1)   ' ###
      'B_M_From_easy_move = move_list_root(depth,1,2) ' ###
    End If

    ' If depth = 1 then
    '    print " at d=1 rs1 = "; root_sort(1) ;  " rs2 = "; root_sort(2)
    '    Print B_M_From_easy_move; B_M_To_easy_move
    '    end if
    '

    'if depth = 4 then Print  "at d=4 "; "rs1 = "; root_sort(1) ;  " rs2 = "; root_sort(2)

    If depth = 5 And root_sort(1) - root_sort(2) > 250 And Move_list(depth + 1, 1, 1) = B_M_From_easy_move And Move_list(depth + 1, 1, 2) = B_M_To_easy_move Then it_depth = max_depth

    ' If fix_depth = 0 Then ply_end_time = Timer
    ' If it_depth < 6 And think_time < ((ply_end_time - ply_start_time) * 150) Then it_depth = max_depth      'default = 400 (26/1/09) ' 275 26/10/09  '200 21/11/09 '150 (5/7/10)
    ' If it_depth >= 6 And think_time < ((ply_end_time - ply_start_time) * 225) Then it_depth = max_depth     'default = 400 (26/1/09) ' 275 26/10/09  '200 21/11/09 '150 (5/7/10)

    ' If conv_clock = 1 And (ply_end_time - ply_start_time) > 2.5 * av_think_time Then it_depth = max_depth   '2.2 (26/10/09
    /'
    If fix_depth = 0 Then ply_end_time = Timer
    'default = 400 (26/1/09) ' 275 26/10/09  '200 21/11/09 '150 (5/7/10)
    If it_depth < 6 And think_time < ((ply_end_time - ply_start_time) * 150) Then it_depth = max_depth
    'default = 400 (26/1/09) ' 275 26/10/09  '200 21/11/09 '150 (5/7/10)
    If it_depth >= 6 And think_time < ((ply_end_time - ply_start_time) * 225) Then it_depth = max_depth
    '2.2 (26/10/09
    'If conv_clock = 1 And (ply_end_time - ply_start_time) > 2.5 * av_think_time Then it_depth = max_depth
    If conv_clock = 1 And ((ply_end_time - ply_start_time)*100) > 2.5 * av_think_time Then it_depth = max_depth
'/

    If fix_depth = 0 Then '### reworked the code
      ply_end_time = Timer - ply_start_time

      If it_depth < 6 Then
        'default = 400 (26/1/09) ' 275 26/10/09  '200 21/11/09 '150 (5/7/10)
        If think_time < (ply_end_time * 150) Then it_depth = max_depth : GoTo it6
      Else
        If think_time < (ply_end_time * 225) Then it_depth = max_depth : GoTo it6
      End If
      '2.2 (26/10/09
      'If conv_clock = 1 And (ply_end_time - ply_start_time) > 2.5 * av_think_time Then it_depth = max_depth
      '### 100 / 2.5 ==> 40 / 1
      If conv_clock = 1 And (ply_end_time * 40) > av_think_time Then it_depth = max_depth
    End If
    'B_M_To_last_ply = B_M_To:B_M_From_last_ply = B_M_From
    'last_ply_piece = Pe

    it6:

    'Print "think = ";think_time; "plye-plys = ";(ply_end_time - ply_start_time) * 100
    'Print "moved in (s) = ";(ply_end_time - ply_start_time) * 100

    'it7:
    total_nodes += nodes

  Next it_depth

  If perft = TRUE Then Exit Sub ' Return nodes   'goto cm7a

  
'Print " Move no = ";MoveNo
'Print " bestscore(MoveNo) = ";bestscore(MoveNo)
'Print " bestscore(MoveNo-1) = ";bestscore(MoveNo-1)
'Print " bestscore(MoveNo-2) = ";bestscore(MoveNo-2)

  If MoveNo > 5 and (best_score < -750) and (bestscore(MoveNo-1) < -750) and (bestscore(MoveNo-2) < -750) and (bestscore(MoveNo-3) < -750)then GIVEUP = TRUE 'ok enough punishment!

  If GIVEUP = TRUE then Exit sub'goto c8
      
  
  
  
  If Not prom_last_ply = 0 Then                  'identify promotion piece for winboard
    If Abs(Pe) = 6 Then Prom_Piece_str = "q"
    If Abs(Pe) = 4 Then Prom_Piece_str = "r"
    If Abs(Pe) = 3 Then Prom_Piece_str = "b"
    If Abs(Pe) = 2 Then Prom_Piece_str = "n"
  End If
  ' ### need to update mat_left and other piece counters to reflect new piece
  If B(B_M_To) <> 0 Then No_Pieces -= 1          'if piece is taken then reduce number of pieces on board

  If B(B_M_To) = 2 Then Wkn -= 1
  If B(B_M_To) = 3 Then Wbp -= 1
  If B(B_M_To) = -2 Then Bkn -= 1
  If B(B_M_To) = -3 Then Bbp -= 1               '###   Bkn -= 1 ????

  If B(B_M_To) <> 0 Or B(B_M_From) = 1 Or B(B_M_From) = -1 Then fifty_move = 0 Else fifty_move += 1

  If Abs(B(B_M_To)) = 6 Then
    Mat_left -= Queen_Value
    If B(B_M_To) = 6 Then WQ = 0 Else BQ = 0     ' ### should be -= 1 there can be more then 1 queen
  End If

  If Abs(B(B_M_To)) = 4 Then
    Mat_left -= Rook_Value
    If B(B_M_To) = 4 Then wr -= 1 Else br -= 1
  End If

  If Abs(B(B_M_To)) = 3 Then Mat_left -= Bishop_Value
  If Abs(B(B_M_To)) = 2 Then Mat_left -= Knight_Value
  If Abs(B(B_M_To)) = 1 Then
    Knight_Value -= 1
    Rook_Value += 1
  End If

  B(B_M_From) = 0 : B(B_M_To) = Pe               'make computer move

  If side = white Then                           ' update move history with current move
    move_hist(MoveNo, 1) = B_M_From
    move_hist(MoveNo, 2) = B_M_To
  Else                                           ' If side = black Then
    move_hist(MoveNo, 3) = B_M_From
    move_hist(MoveNo, 4) = B_M_To
  End If

  best_score = root_sort(moveno)                 ' ###

  ' promotion piece is updated in best move routine - remember if problems later that it perhaps should be updated after the move is made? (cht 29/10/08)

  Dim As String CompMoveFrom_str = move_2_str(B_M_From)
  Dim As String CompMoveTo_str = move_2_str(B_M_To)

  'cm4a:

  'If side = black Then GoTo cm5
  If side = white Then

    If B(B_M_To) = 7 Then         ' if king then check for castling
      wkloc = B_M_To
      If B_M_From = 25 Then       ' if king moves then castling is not longer allowed
        If B_M_To = 27 Then
          B(28) = 0
          B(26) = 4
        ElseIf B_M_To = 23 Then
          B(21) = 0
          B(24) = 4
        End If
      End If
    End If
    'take off black pawn if white move is pxp ep capture
    If B(B_M_To) = 1 And B_M_To = act_epsq Then B(B_M_To - 10) = 0

    'set ep-square for next move (if next move is black)
    If B(B_M_To) = 1 And (B_M_To - B_M_From = 20) Then act_epsq = B_M_To - 10 Else act_epsq = -1

    ' If B_M_From = 21 And B(B_M_To) = 4 Then w_cas(2) = -1

    ' If B_M_From = 28 And B(B_M_To) = 4 Then w_cas(1) = -1
    If wkloc <> 25 Or b(28) <> 4 Then w_cas(1) = -1
    If wkloc <> 25 Or b(21) <> 4 Then w_cas(2) = -1


    ' If B_M_From = 25 And B_M_To = 27 And B(B_M_To) = 7 Then
    ' B(28) = 0
    ' B(26) = 4
    ' w_cas(1) = -1                               ' 1
    ' w_cas(2) = -1                             'added 16/1/2010 - Leo castle bug
    ' ElseIf B_M_From = 25 And B_M_To = 23 And B(B_M_To) = 7 Then
    ' B(21) = 0
    ' B(24) = 4
    ' w_cas(2) = -1                               ' 1
    ' w_cas(1) = -1                             'added 16/1/2010 - Leo castle bug
    ' End If

    ' If B_M_From = 25 And B(B_M_To) = 7 And (w_cas(1) <> 1 Or w_cas(2) <> 1) Then
    ' w_cas(1) = -1
    ' w_cas(2) = -1
    ' End If

  Else                                           'GoTo cm7

    'cm5:
    ' black moves

    If B(B_M_To) = -7 Then                ' update the relevant king position
      bkloc = B_M_To
      If B_M_From = 95 Then
        b_cas(1) = -1
        b_cas(2) = -1
        If B_M_To = 97 Then
          B(98) = 0
          B(96) = -4
        ElseIf B_M_To = 93 Then
          B(91) = 0
          B(94) = -4
        End If
      End If
    End If

    'take off white pawn if black move is pxp ep capture
    If B(B_M_To) = -1 And B_M_To = act_epsq Then B(B_M_To + 10) = 0

    'set ep-square for next move (if next move is white)
    If B(B_M_To) = -1 And (B_M_To - B_M_From = -20) Then act_epsq = B_M_To + 10 Else act_epsq = -1

    'If B_M_From = 91 And B(B_M_To) = -4 Then b_cas(2) = -1

    'If B_M_From = 98 And B(B_M_To) = -4 Then b_cas(1) = -1

    If bkloc <> 95 Or b(98) <> -4 Then b_cas(1) = -1
    If bkloc <> 95 Or b(91) <> -4 Then b_cas(2) = -1

    ' If B_M_From = 95 And B_M_To = 97 And B(B_M_To) = -7 Then
    ' B(98) = 0
    ' B(96) = -4
    ' b_cas(1) = -1
    ' b_cas(2) = -1                             'added 16/1/2010 - Leo castle bug
    ' ElseIf B_M_From = 95 And B_M_To = 93 And B(B_M_To) = -7 Then
    ' B(91) = 0
    ' B(94) = -4
    ' b_cas(2) = -1
    ' b_cas(1) = -1                             'added 16/1/2010 - Leo castle bug
    ' End If

    ' If B_M_From = 95 And B(B_M_To) = -7 And (b_cas(1) <> 1 Or b_cas(2) <> 1) Then
    ' b_cas(1) = -1
    ' b_cas(2) = -1
    ' End If
  End If

  ' If perft = TRUE Then Return nodes   'goto cm7a

  'cm7:
  If perft = TRUE Then Exit Sub                  ' ###
  '  Print "Total moves at root = ";orig_root'root
  '  print "Total node count = ";total_node

  'If winboardmode = 0 Then display_node = total_nodes
  ' display_node = nodes
  '    sleep
  '    end if

  ' set insufficient material flag if appropriate
  If (No_Pieces = 3 And (Bbp + Wbp + Bkn + Wkn = 1)) Then IM = 1

  STM = Opponent

  If WinboardMode = 1 Then  ' And  root <> 0 Then
    conv_clock_move_no += 1
    SendCommand( "move " + LCase(CompMoveFrom_str + CompMoveTo_str + Prom_Piece_str))
  End If

  'cm7a:
  'Return nodes

  'c8:

End Sub


'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
Function ab_search(side As Integer, depth As Integer, Alpha As Integer, beta As Integer, fmove() As Integer) As Integer

  ' if fmove(depth+1) = 100 then   'insearch 50  move rule test - removed 21/2/2010 (c5% speed up in total)
  '    nodes = nodes + 1
  '    depth = 0
  '    return 0
  '    end if

  If timed_out = 1 Then Exit Function ' ### we have no more time for this

  Dim As Integer Vo, swap_value
  Dim As Integer ps, Check, thru_check, y

  ' If perft = TRUE Then GoTo ab
  If perft = FALSE Then                          ' ###
    If depth = 0 And temp_val1(1) <> 0 Then      ' if move @ horizon is capture then engage swap-off routine
      stand_pat = evaluate(side)
      Vo = init_swp_val
      swap_value = QS(Vo, side)
      'If winboardmode = 0 and ((cmpclr = 1 and side = black) or (cmpclr = -1 and side = white)) and swap_value < 0 then swap_value = 0
      'If winboardmode = 1 and ((cmpclr = 1 and side = white) or (cmpclr = -1 and side = black)) and swap_value < 0 then swap_value = 0 'orig line
      If swap_value < 0 Then swap_value = 0
      Return stand_pat + swap_value
    End If

    'if horizon move is promotion, extend to see if pawn is captured
    If depth = 0 And (wprom(1) = 1 Or bprom(1) = 1) Then
      stand_pat = evaluate(side)
      Vo = init_swp_val
      If wprom(1) = 1 Then   ' ###
        Vo = 6
      ElseIf bprom(1) = 1 Then
        Vo = -6
      EndIf
      swap_value = QS(Vo, side)
      If winboardmode = 0 And ((cmpclr = 1 And side = black) Or (cmpclr = -1 And side = white)) And swap_value < 0 Then swap_value = 0
      If winboardmode = 1 And ((cmpclr = 1 And side = white) Or (cmpclr = -1 And side = black)) And swap_value < 0 Then swap_value = 0
      ' If swap_value < 0 Then swap_value = 0
      Return stand_pat + swap_value
    End If
    ' ab:
  End If                                         '###

  If depth = 0 Then
    nodes += 1
    '   nodes
    If perft = FALSE Then Return evaluate(side) Else Return nodes
  End If

  If fix_depth = 0 Then                          ' ### don't invoke this when fix_depth=1 (console)
    'sd criteria added to make sure move from last ply is available to play (21/6/10)
    If search_depth > 5 And (orig_root < 5 Or (Move_Control - conv_clock_move_no) < 4) Then
      'break = Timer
      ''(changed from 0.3  - 4/7/10)
      'If (((break - start_time) * 100) > think_time) Or (((break - start_time) * 100) >= (0.4 * time_left)) Then
      ' timed_out = 1
      ' Exit Function                            'return 0'evaluate(side)
      'End If
      Dim As Double break = (Timer - start_time) * 100
      If break > think_time Or break >= (0.4 * time_left) Then
        timed_out = 1
        Exit Function
      EndIf
    End If
  End If

  gen_move(side, depth)

  ' xg = 0 ' counter to test for legal moves
  Dim As UInteger xg  ' ### easy to see if it 0
  '====================================================
  w_casflags(depth, 1) = w_cas(1)  ' ###
  w_casflags(depth, 2) = w_cas(2)
  b_casflags(depth, 1) = b_cas(1)
  b_casflags(depth, 2) = b_cas(2)
  '====================================================

  For y = 1 To Pseu_Moves(Depth)

    'make_move(side, depth, Pseu_Moves(), y, fmove())

    ' ===================================================== make move
    '$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    'Function make_move(side As Integer, depth As Integer, Pseu_Moves() As Integer, y As Integer, fmove() As Integer) As Integer


    If (Move_List(depth, y, 2) - Move_List(depth, y, 1) = side*20) And B(Move_List(depth, y, 1)) = side And (B(Move_List(depth, y, 2) - 1) = -side Or B(Move_List(depth, y, 2) + 1) = -side) Then
      epsqr(depth - 1) = Move_List(depth, y, 1) + (side*10)
    Else
      epsqr(depth - 1) = -1                       ' no ep square found
    End If

    If (Move_List(depth, y, 2) - Move_List(depth, y, 1) <> side*11 And Move_List(depth, y, 2) - Move_List(depth, y, 1) <> side*9) Or (B(Move_List(depth, y, 1)) <> side) Then GoTo m1

    ' take off pawn
    If (Move_List(depth, y, 2) = epsqr(depth)) And B(epsqr(depth)) = 0 Then B(epsqr(depth) + (side* - 10)) = 0

    m1:

    'castle code
    If side = white Then

      If Move_List(depth, y, 1) = 25 And Move_List(depth, y, 2) = 27 And B(Move_List(depth, y, 1)) = 7 Then
        B(28) = 0 : B(26) = 4                      ' move rook
      End If

      If Move_List(depth, y, 1) = 25 And Move_List(depth, y, 2) = 23 And B(Move_List(depth, y, 1)) = 7 Then
        B(21) = 0 : B(24) = 4
      End If

    Else                                           ' GoTo mm2
      ' mm1:

      ' Black castle moves


      If Move_List(depth, y, 1) = 95 And Move_List(depth, y, 2) = 97 And B(Move_List(depth, y, 1)) = -7 Then
        B(98) = 0 : B(96) = -4
      End If

      If Move_List(depth, y, 1) = 95 And Move_List(depth, y, 2) = 93 And B(Move_List(depth, y, 1)) = -7 Then
        B(91) = 0 : B(94) = -4
      End If

      ' make move

    End If                                         'mm2:

    temp_val1(depth) = B(Move_List(depth, y, 2))
    B(Move_List(depth, y, 2)) = B(Move_List(depth, y, 1))
    B(Move_List(depth, y, 1)) = 0

    cap_sq = Move_List(depth, y, 2)                'save capture square for swap off extension
    init_swp_val = B(Move_List(depth, y, 2))       'save piece value for swap off extension

    'If side = black Then GoTo mp1
    If side = white Then
      If B(Move_List(depth, y, 2)) = 7 Then wkloc = Move_List(depth, y, 2)

      'If w_casflags(depth, 1) <> 1 And
      If (B(25) <> 7 Or B(28) <> 4) Then w_cas(1) = -1
      'If w_casflags(depth, 2) <> 1 And
      If (B(25) <> 7 Or B(21) <> 4) Then w_cas(2) = -1

      If Move_List(depth, y, 2) > 90 And B(Move_List(depth, y, 2)) = 1 Then wprom(depth) = 1 Else wprom(depth) = 0

      'goto mp2
      If wprom(depth) = 0 Then GoTo mm3   ' ### skip the rest

      If Move_List(depth, y, 3) = 6 Then B(Move_List(depth, y, 2)) = 6
      If Move_List(depth, y, 3) = 4 Then B(Move_List(depth, y, 2)) = 4
      If Move_List(depth, y, 3) = 3 Then B(Move_List(depth, y, 2)) = 3
      If Move_List(depth, y, 3) = 2 Then B(Move_List(depth, y, 2)) = 2

      GoTo mm3   ' ### skip the rest
    End If

    'mp1:

    If B(Move_List(depth, y, 2)) = -7 Then bkloc = Move_List(depth, y, 2)

    'If b_casflags(depth, 1) <> 1 And
    If (B(95) <> - 7 Or B(98) <> - 4) Then b_cas(1) = -1
    'If b_casflags(depth, 2) <> 1 And
    If (B(95) <> - 7 Or B(91) <> - 4) Then b_cas(2) = -1

    If Move_List(depth, y, 2) < 29 And B(Move_List(depth, y, 2)) = -1 Then bprom(depth) = 1 Else bprom(depth) = 0

    'goto mp2
    If bprom(depth) = 0 Then GoTo mm3   ' ### skip the rest

    If Move_List(depth, y, 3) = -6 Then B(Move_List(depth, y, 2)) = -6
    If Move_List(depth, y, 3) = -4 Then B(Move_List(depth, y, 2)) = -4
    If Move_List(depth, y, 3) = -3 Then B(Move_List(depth, y, 2)) = -3
    If Move_List(depth, y, 3) = -2 Then B(Move_List(depth, y, 2)) = -2

    ' Return temp_val1(depth)

    mm3:

    'End Function

    ' ==================================================end make move


    Check = incheck(side)

    'If Check = TRUE Then GoTo p1
    If check = FALSE Then
      '===============================================================
      ' only legal castling moves are generated, making test redundant
      '===============================================================
      xg += 1                                    ' if here then must be a legal move

      score = -ab_search(-side, depth - 1, -beta, -Alpha, fmove())

    End If
    ' p1:

    'unmake_move(side, depth, Pseu_Moves(), y, temp_val1(), fmove())

    ' ===================================================== unmake move
    '$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    'Sub unmake_move(side As Integer, Depth As Integer, Pseu_Moves() As Integer, y As Integer, temp_val1() As Integer, fmove() As Integer)


    If ((Move_List(depth, y, 2) - Move_List(depth, y, 1) <> side * 11) And (Move_List(depth, y, 2) - Move_List(depth, y, 1) <> side * 9)) Or (B(Move_List(depth, y, 2)) <> side) Then GoTo um1
    ' ###
    If Move_List(depth, y, 2) = epsqr(depth) Then B(epsqr(depth) + (side * -10)) = -side

    um1:
    'If side = black Then GoTo um8
    If side = white Then
      If Move_List(depth, y, 2) = 27 And Move_List(depth, y, 1) = 25 And B(Move_List(depth, y, 2)) = 7 Then
        B(26) = 0 : B(28) = 4
      End If

      If Move_List(depth, y, 2) = 23 And Move_List(depth, y, 1) = 25 And B(Move_List(depth, y, 2)) = 7 Then
        B(24) = 0 : B(21) = 4
      End If

    Else                                           'GoTo um10:

      'um8:

      If Move_List(depth, y, 2) = 97 And Move_List(depth, y, 1) = 95 And B(Move_List(depth, y, 2)) = -7 Then
        B(96) = 0 : B(98) = -4
      End If

      If Move_List(depth, y, 2) = 93 And Move_List(depth, y, 1) = 95 And B(Move_List(depth, y, 2)) = -7 Then
        B(94) = 0 : B(91) = -4
      End If
    End If

    'um10:
    B(Move_List(depth, y, 1)) = B(Move_List(depth, y, 2))
    B(Move_List(depth, y, 2)) = temp_val1(depth)



    If B(Move_List(depth, y, 1)) = 7 Then wkloc = Move_List(depth, y, 1)
    If B(Move_List(depth, y, 1)) = -7 Then bkloc = Move_List(depth, y, 1)

    'If side = black Then GoTo u11
    If side = white Then

      '====================================================
      w_cas(1) = w_casflags(depth, 1) ' ###
      w_cas(2) = w_casflags(depth, 2)
      '====================================================

      If wprom(depth) <> 1 Then GoTo u12

      If Move_List(depth, y, 2) > 90 Then
        If B(Move_List(depth, y, 1)) = 6 And Move_List(depth, y, 3) = 6 Then B(Move_List(depth, y, 1)) = 1
        If B(Move_List(depth, y, 1)) = 4 And Move_List(depth, y, 3) = 4 Then B(Move_List(depth, y, 1)) = 1
        If B(Move_List(depth, y, 1)) = 3 And Move_List(depth, y, 3) = 3 Then
          B(Move_List(depth, y, 1)) = 1
          Wbp -= 1                                   ' 17/1/09 - reduce no white bishops (ie undo promoted bishop) - needed to avoid incorrect draw by insufficient material
        End If
        If B(Move_List(depth, y, 1)) = 2 And Move_List(depth, y, 3) = 2 Then
          B(Move_List(depth, y, 1)) = 1
          Wkn -= 1                                   ' 17/1/09 - reduce no white Kn (ie undo promoted Kn) - needed to avoid incorrect draw by insufficient material
        End If                                       ' don't worry about Q & R because can still mate - don't worry about no Queens coz if incremented for promoted Q then endgame eval won't kick in (ie K goes to corners)
      End If

      ' Exit Sub

    Else                                           '  u11:
      '====================================================
      b_cas(1) = b_casflags(depth, 1)
      b_cas(2) = b_casflags(depth, 2)
      '====================================================

      If bprom(depth) <> 1 Then GoTo u12

      If Move_List(depth, y, 2) < 29 Then
        If B(Move_List(depth, y, 1)) = -6 And Move_List(depth, y, 3) = -6 Then B(Move_List(depth, y, 1)) = -1
        If B(Move_List(depth, y, 1)) = -4 And Move_List(depth, y, 3) = -4 Then B(Move_List(depth, y, 1)) = -1
        If B(Move_List(depth, y, 1)) = -3 And Move_List(depth, y, 3) = -3 Then
          B(Move_List(depth, y, 1)) = -1
          Bbp -= 1
        End If
        If B(Move_List(depth, y, 1)) = -2 And Move_List(depth, y, 3) = -2 Then
          B(Move_List(depth, y, 1)) = -1
          Bkn -= 1
        End If
      End If
    End If

    u12:

    'End Sub

    '$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    ' ==================================================end unmake move


    /'

    '====================================================
    w_cas(1) = w_casflags(depth, 1) ' ###
    w_cas(2) = w_casflags(depth, 2)
    b_cas(1) = b_casflags(depth, 1)
    b_cas(2) = b_casflags(depth, 2)
    '====================================================
'/

    'if perft = true or check = true then goto p4
    'GoTo p4
    If check = TRUE Or perft = TRUE Then Continue For

    If score >= beta Then Return beta            ' if score 'too good' then cut-off search as opponent won't move here

    If score > Alpha Then Alpha = score

    ' If alpha = -9999 then alpha = (-9999 + search_depth) elseif  alpha = 9999 then alpha = (9999 - search_depth + depth)
    If Alpha = -9999 Then
      Alpha = (-9999 + search_depth)
    ElseIf Alpha = 9999 Then
      Alpha = (9999 - search_depth + depth)
    End If
    'p4:
  Next y
  'If xg = 0 And incheck(side) = TRUE Then Alpha = -9999 + search_depth ElseIf  xg = 0 And incheck(side) = FALSE Then Alpha = 0
  ' ######################
  If xg = 0 Then                        ' ###
    If incheck(side) = TRUE Then        ' ###
      Alpha = -9999 + search_depth      ' ###
    Else                                ' ###
      Alpha = 0                         ' ###
    End If                              ' ###
  End If                                ' ###

  Return Alpha

End Function


'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
Function evaluate(side As Integer) As Integer

  Dim As Integer k, l, x, k1, l1, Bbp ,Wbp, Bkn, Wkn ' ### added Bbp ,Wbp, Bkn, Wkn
  Dim As Integer Pos_Eval, Material
  Dim As Integer black_p_p, bbp_sq1, bp1r, bp1f
  Dim As Integer white_p_p, wpp_sq1, wp1r, wp1f
  Dim As Integer bbp_sq2, bp2r, bp2f, wpp_sq2, wp2r, wp2f
  Dim As Integer bpp, Br_ev, bqev, wpp, Wr_ev, wqev
  Dim As Integer end_pos_eval, rk_support, pp_support, pp_obst, brk_support, bpp_support, bpp_obst
  Dim As Integer Weg_condition4, Beg_condition4, r2prom
  Dim As Integer King_inside, prom_path, scan, h, br2prom, B_King_inside, bprom_path, b_scan
  Dim As UInteger KL

  For K = 9 To 2 Step -1
    For L = 1 To 8

      ' If  B(K*10+L) = 0 Then Continue For ' GoTo evl1
      ' KL = K*10+L

      'KL = K*10 + L                              ' ###
      kl  = k * 8 + k + k + l                      ' ###
      If B(KL) = 0 Then Continue For             ' GoTo evl1   ' ###

      'If  B(KL) > 0 Then GoTo lp1
      If B(KL) < 0 Then
        If B(KL) = -1 Then
          Material -= Pawn_Value
          If ENDGAME = 0 Then
            Pos_Eval -= BP_PST(KL)
            ' penalty for doubled pawns - added 4/1/09 in end5
            If B(KL - 10) = -1 Or B(KL - 20) = -1 Or B(KL - 30) = -1 Then Pos_Eval += 15
            Continue For                         ' GoTo evl1
          End If
          Pos_Eval -= BP_end_PST(KL)
          ' penalty for doubled pawns - added 4/1/09 in end5
          If B(KL - 10) = -1 Or B(KL - 20) = -1 Or B(KL - 30) = -1 Then Pos_Eval += 30
          bpp = TRUE
          k1 = K - 1
          l1 = L - 1
          For x = k1 To 2 Step - 1
            'If B(x*10 + l1) = 1 Then bpp = FALSE
            If B(x*8+x+x+ l1) = 1 Then bpp = FALSE ' ###
          Next x
          If bpp = FALSE Then Continue For ' GoTo evl1
          l1 += 1   ' ### l1 = L ###  l1 = l - 1 so added 1 makes it equal to l
          For x = k1 To 2 Step - 1
            'If B(x*10 + l1) = 1 Then bpp = FALSE
            If B(x*8+x+x+l1) = 1 Then bpp = FALSE ' ###
          Next x
          If bpp = FALSE Then Continue For ' GoTo evl1
          l1 += 1  ' ### l1 = L + 1
          For x = k1 To 2 Step - 1
            'If B(x*10 + l1) = 1 Then bpp = FALSE
            If B(x*8+x+x+l1) = 1 Then bpp = FALSE ' ###
          Next x
          If bpp = FALSE Then Continue For       ' GoTo evl1
          black_p_p += 1
          If black_p_p = 1 Then
            bbp_sq1 = KL
            bp1r = L
            bp1f = K
          End If
          If black_p_p = 2 Then
            bbp_sq2 = KL
            bp2r = L
            bp2f = K
          End If
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = -4 Then
          Material = Material - Rook_Value
          Pos_Eval -= BRk_PST(KL)
          Br_ev = 1
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = -3 Then
          Material = Material - Bishop_Value
          Pos_Eval -= BBp_PST(KL)
          Bbp += 1
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = -2 Then
          Material = Material - Knight_Value
          Pos_Eval -= BKt_PST(KL)
          Bkn += 1
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = -6 Then
          Material = Material - Queen_Value
          Pos_Eval -= BQn_PST(KL)
          If ENDGAME = 1 Then bqev = 1
          Continue For                           ' GoTo evl1
        End If

      Else
        'lp1:

        If B(KL) = 1 Then
          Material += Pawn_Value
          If ENDGAME = 0 Then
            Pos_Eval += WP_PST(KL)
            ' penalty for doubled pawns - added 4/1/09 in end5
            If B(KL + 10) = 1 Or B(KL + 20) = 1 Or B(KL + 30) = 1 Then Pos_Eval -= 15
            Continue For                         ' GoTo evl1
          End If
          Pos_Eval += WP_end_PST(KL)
          ' penalty for doubled pawns - added 4/1/09 in end5
          If B(KL + 10) = 1 Or B(KL + 20) = 1 Or B(KL + 30) = 1 Then Pos_Eval -= 30
          wpp = TRUE
          k1 = K + 1
          l1 = L - 1
          For x = k1 To 8
            'If B(x*10 + l1) = -1 Then wpp = FALSE
            If B(x*8+x+x+l1) = -1 Then wpp = FALSE  ' ###
          Next x
          If wpp = FALSE Then Continue For       ' GoTo evl1
          l1 += 1  ' ### l1 = L ###  l1 = l - 1 so added 1 makes it equal to l
          For x = k1 To 8
            'If B(x*10 + l1) = -1 Then wpp = FALSE
            If B(x*8+x+x+l1) = -1 Then wpp = FALSE  ' ###
          Next x
          If wpp = FALSE Then Continue For       ' GoTo evl1
          l1 += 1   ' ### l1 = L + 1
          For x = k1 To 8
            'If B(x*10 + l1) = -1 Then wpp = FALSE
            If B(x*8+x+x+l1) = -1 Then wpp = FALSE  ' ###
          Next x
          If wpp = FALSE Then Continue For       ' GoTo evl1
          white_p_p += 1
          If white_p_p = 1 Then
            wpp_sq1 = KL
            wp1r = L
            wp1f = K
          End If
          If white_p_p = 2 Then
            wpp_sq2 = KL
            wp2r = L
            wp2f = K
          End If
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = 4 Then
          Material += Rook_Value
          Pos_Eval += WRk_PST(KL)
          Wr_ev = 1
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = 3 Then
          Material += Bishop_Value
          Pos_Eval += WBp_PST(KL)
          Wbp += 1
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = 2 Then
          Material += Knight_Value
          Pos_Eval += WKt_PST(KL)
          Wkn += 1
          Continue For                           ' GoTo evl1
        End If

        If B(KL) = 6 Then
          Material += Queen_Value
          Pos_Eval += WQn_PST(KL)
          If ENDGAME = 1 Then wqev = 1
        End If
      End If
      'evl1:

    Next l
  Next k

  'If endgame = 1 Then GoTo eg1
  If endgame = 0 Then

    Pos_Eval += (WKg_PST(wkloc) - BKg_PST(bkloc))

    ' trapped piece penalty

    'trapped white king rook
    If B(26) = 7 And (B(27) + B(28) = 4) Then Pos_Eval -= 25

    If B(27) = 7 And (B(28) = 4 Or B(38) = 4) Then Pos_Eval -= 50

    If B(23) = 7 And (B(21) + B(22) = 4) Then Pos_Eval -= 25

    If B(22) = 7 And (B(21) = 4 Or B(31) = 4) Then Pos_Eval -= 50

    ' trapped black king rook
    If B(96) = -7 And (B(97) + B(98) = -4) Then Pos_Eval += 25
    If B(97) = -7 And (B(98) = -4 Or B(88) = -4) Then Pos_Eval += 50

    ' trapped black king rook
    If B(93) = -7 And (B(92) + B(91) = -4) Then Pos_Eval += 25
    If B(92) = -7 And (B(91) = -4 Or B(81) = -4) Then Pos_Eval += 50


    ' trapped white bishop
    If B(88) = 3 And B(77) = -1 And B(86) = -1 Then Pos_Eval -= 25
    If B(81) = 3 And B(72) = -1 And B(83) = -1 Then Pos_Eval -= 25

    ' trapped black bishop
    If B(38) = -3 And B(47) = 1 And B(36) = 1 Then Pos_Eval += 25
    If B(31) = -3 And B(42) = 1 And B(33) = 1 Then Pos_Eval += 25

    'penalty for lack of pawn protection if castled O-O
    ' If w_cas(1) = 1 And (B(36) + B(37)) <> 2 Then Pos_Eval -= 20
    'If b_cas(1) = 1 And (B(86) + B(87)) <> -2 Then Pos_Eval += 20
    If w_cas(1) = 1 And B(36) = 1 And B(37) = 1 Then Pos_Eval -= 20     ' ###
    If b_cas(1) = 1 And B(86) = -1 And B(87) = -1 Then Pos_Eval += 20   ' ###

  End If                                         ' ###
  'If endgame = 0 Then GoTo ev1
  If endgame = 1 Then

    '**********************************
    ' endgame evaluation
    ' passed pawn
    '**********************************

    'eg1:

    Pos_Eval += (WKg_end_PST(wkloc) - BKg_end_PST(bkloc))

    If black_p_p + white_p_p = 0 Then GoTo ev1 'egc   ' added 12/10/09 - to prevent endpos eval assessment when no passed pawns!

    If wpp_sq2 > wpp_sq1 Then                    ' only consider furthest advanced passed pawn
      wpp_sq1 = wpp_sq2
      wpp_sq2 = 0
      wp1r = wp2r
      wp2r = 0
      wp1f = wp2f
      wp2f = 0
    End If

    If (bbp_sq2 <> 0) And bbp_sq2 < bbp_sq1 Then
      bbp_sq1 = bbp_sq2
      bbp_sq2 = 0
      bp1r = bp2r
      bp2r = 0
      bp1f = bp2f
      bp2f = 0
    End If

    ' eval based on no passed pawns + bonus for progression
    end_pos_eval = (white_p_p - black_p_p) * 30 + ((wp1f * 10) - (- bp1f + 9) * 10)

    If wr_ev = 0 And br_ev = 0 Then GoTo egc     ' minor speed advantage with no rooks on board in endgame

    If wpp_sq1 <> 0 Then                         ' additional eval bonus if passed pawn is supported by rook
      rk_support = wpp_sq1
      pp_support = FALSE
      pp_obst = FALSE
      Do
        rk_support -= 10
        If B(rk_support) = 4 Then pp_support = TRUE
      Loop Until B(rk_support) <> 0
      rk_support = wpp_sq1
      Do
        rk_support += 10
        If B(rk_support) <> 0 Then pp_obst = TRUE
      Loop Until rk_support > 90 And rk_support < 99
      If pp_obst = FALSE And PP_support = TRUE Then end_pos_eval += 40
    End If


    If bbp_sq1 <> 0 Then
      brk_support = bbp_sq1
      bpp_support = FALSE
      bpp_obst = FALSE
      Do
        brk_support += 10
        If B(brk_support) = -4 Then bpp_support = TRUE
      Loop Until B(brk_support) <> 0
      brk_support = bbp_sq1
      Do
        brk_support -= 10
        If B(brk_support) <> 0 Then bpp_obst = TRUE
      Loop Until brk_support > 20 And brk_support < 29
      If bpp_obst = FALSE And bpp_support = TRUE Then end_pos_eval -= 40
    End If

    egc:

    'code below to test for K outside square of pawn conditions - only engaged if opponent has no pieces
    Weg_condition4 = (BQ = 0 And bqev = 0 And br_ev = 0 And Bbp = 0 And Bkn = 0)

    Beg_condition4 = (WQ = 0 And wqev = 0 And wr_ev = 0 And Wbp = 0 And Wkn = 0)

    If Weg_condition4 And wpp_sq1 <> 0 Then
      r2prom = 9 - wp1f
      If side = black Then r2prom += 1
      King_inside = FALSE
      prom_path = wpp_sq1
      scan = wpp_sq1
      Do
        For h = -r2prom To r2prom
          ' GoTo sc1 ' avoid wrap around on board
          If -h > (wp1r - 1) Or (h + wp1r > 8) Then Continue For
          scan = prom_path
          scan += h
          If B(scan) = -7 Then King_inside = TRUE
          scan = prom_path
          ' sc1:
        Next h
        prom_path += 10
        scan = prom_path
      Loop Until scan > 98 Or King_inside = TRUE
      If king_inside = FALSE Then Material += 800' if king can't catch pawn then increase material by 950 - 110 )ie Q less promoted pawn)
    End If

    If Beg_condition4 And bbp_sq1 <> 0 Then
      br2prom = bp1f - 2
      If side = white Then br2prom += 1
      B_King_inside = FALSE
      bprom_path = bbp_sq1
      Do
        For h = -br2prom To br2prom
          ' GoTo sc2    ' don't check further if hit edge of board marker
          If -h > (bp1r - 1) Or (h + bp1r > 8) Then Continue For
          b_scan = bprom_path
          b_scan += h
          If B(b_scan) = 7 Then B_King_inside = TRUE
          ' sc2:
          b_scan = bprom_path

        Next h
        bprom_path -= 10
        b_scan = bprom_path
      Loop Until b_scan < 21 Or B_King_inside = TRUE
      'SC3:
      ' if king can't catch pawn then increase material by 950 - 110 )ie Q less promoted pawn)
      If B_king_inside = FALSE Then Material -= 800
    End If

  End If                                         ' ###
  ev1:

  ' EV = (Material + Pos_Eval + end_pos_eval)
  ' If side = black Then Eval = -EV Else Eval = EV
  Eval = (Material + Pos_Eval + end_pos_eval)
  If side = black Then Eval = -Eval
  Return Eval

End Function

'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

Function QS(Vo As Integer, side As Integer) As Integer

  '***********************************************
  '       Capture extension routine
  '
  'This routine is based on the 'swap-off' ideas
  'explained by David Levy in his book 'Chess &
  'Computers', see pages 44 - 48
  '***********************************************
  'array to flip swap off values depending on colour
  Dim As Integer FlipSwapOffB2W(4), FlipSwapOffW2B(4)
  Dim As Integer SwapOffAtk(5), SwapOffDef(5)    ' array to hold swap off piece values
  Dim As Integer bgain(5), wgain(5), tempgain(5) ' array to hold swap off values

  Dim As Integer K                                  ' = cap_sq        'identify location sqr of leaf node capture
  ' ,u, L, S
  Dim As Integer T, SWP_STM = side, RR = cap_sq, i, j
  Dim As Integer QF, u2, x, id, id1, pid, pid1, id_copy, pid_copy   ', p
  'reset flags/values
  ' Dim As Byte cpflag = 0  'capture flag
  Dim As Integer Atkno                              '= 0  'number of captures by white
  Dim As Integer Defno                              '= 0  'number of captures by black
  Dim As Integer SwapOffValue                    '= 0
  Dim As Integer stemp_1

  'S = 0:T = 0

  'SWP_STM = Side

  'change piece representation values to evaluation values where necessary

  If Abs(Vo) = 4 Then Vo = 500
  If Abs(Vo) = 6 Then Vo = 950
  If Abs(Vo) = 2 Then Vo = 300
  If Abs(Vo) = 3 Then Vo = 325
  If Abs(Vo) = 1 Then Vo = 100

  Dim As Integer Atk_bp_list_pos, Def_bp_list_pos, Atk_Rk_list_pos, Def_Rk_list_pos

  For t = 1 To 2                                 'loop through routine twice, once for each side

    Side = -Side

    If Side = White Then J = -1 Else J = 1      ' instead of L use J

    ' RR = K

    '*******************************
    'test for recapture by pawn
    '*******************************

    For I = 1 To 2                               ' replaced u by I

      If I = 1 Then K = RR + J * 9
      If I = 2 Then K = RR + J * 11

      If B(K) * Side = Pawn Then
        If Side = SWP_STM Then
          Atkno += 1                             'increment white capture counter
          SwapOffAtk(Atkno) = 100                'save swap-off piece value
          'ElseIf Side = -SWP_STM Then        ' ### side = swp_stm or -swp_stm
        Else
          Defno += 1
          SwapOffDef(Defno) = 100
        End If
      End If

      'K = RR
    Next I
    '
    '*******************************
    'test for recapture by knight
    '*******************************
    '
    For I = 9 To 16
      'K = RR

      K = RR + Move_Offset(I)

      If B(K) = -99 Then Continue For           ' GoTo qs1

      If B(K) * Side = Knight Then
        If Side = SWP_STM Then
          Atkno += 1                             'increment white capture counter
          SwapOffAtk(Atkno) = 300                'save swap-off piece value
          'ElseIf Side = -SWP_STM Then
        Else
          Defno += + 1
          SwapOffDef(Defno) = 300
        End If
      End If

      'qs1:
    Next I

    'K = RR
    '
    '*******************************
    'test for recapture by bishop
    '*******************************

    For I = 1 To 4
      K = RR
      For J = 1 To 7

        K += Move_Offset(I)

        If B(K) = 0 Then Continue For            'GoTo qs2

        If B(K) = -99 Then
          'J = 7
          Exit For                               ' GoTo qs2
        End If

        If B(K) * Side = Bishop Then
          'J = 7
          'I = 4
          If Side = SWP_STM Then
            Atkno += 1
            Atk_bp_list_pos = Atkno
            SwapOffAtk(Atkno) = 325
            Exit For, For                        ' GoTo qs2
            ' ElseIf Side = -SWP_STM Then
          Else
            Defno += 1
            Def_bp_list_pos = Defno
            SwapOffDef(Defno) = 325
            Exit For, For                        ' GoTo qs2
          End If
        End If

        If B(K) * Side > 0 And B(K) * Side = Queen Then
          Continue For                           ' GoTo qs2
        ElseIf B(K) * Side > 0 Then              'need to build in flag if bishop is behind Q
          'J = 7
          Exit For                               ' GoTo qs2
        End If

        If B(K) * Side < 0 Then
          Exit For                               ' J = 7
        End If

        'qs2:
      Next J

      'K = RR

    Next I

    'K = RR

    '*******************************
    'test for recapture by rook
    '*******************************

    For I = 5 To 8
      K = RR
      For J = 1 To 7

        K += Move_Offset(I)

        If B(K) = 0 Then Continue For            ' GoTo qs3

        If B(K) = -99 Then
          'J = 7
          Exit For                               ' GoTo qs3
        End If

        If B(K) * Side = Rook Then
          'J = 7 ' change direction only (may be more than 1 rook!) - take out rooks may be doubled!
          If Side = SWP_STM Then
            Atkno += 1
            Atk_Rk_list_pos = Atkno
            SwapOffAtk(Atkno) = 500
            Exit For                             ' GoTo qs3
            'ElseIf Side = -SWP_STM Then
          Else
            Defno += 1
            Def_Rk_list_pos = Defno
            SwapOffDef(Defno) = 500
            Exit For                             ' GoTo qs3
          End If
        End If

        If B(K) * Side > 0 And (B(K) * Side = Queen) Then
          Continue For                           ' GoTo qs3
        ElseIf B(K) * Side > 0 Then
          'J = 7
          Exit For                               ' GoTo qs3
        End If

        If B(K) * Side < 0 Then
          'J = 7
          Exit For
        End If

        'qs3:

      Next J

      'K = RR

    Next I

    'K = RR

    '*******************************
    'test for recapture by queen
    '*******************************
    ' ignore if no queen on board
    If (side = white And WQ = 0) Or (side = black And BQ = 0) Then GoTo qs4a

    For I = 1 To 8
      K = RR
      QF = 0

      For J = 1 To 7

        K += Move_Offset(I)

        If B(K) = 0 Then Continue For            ' GoTo qs4

        If B(K) = -99 Then
          'J = 7
          Exit For                               ' GoTo qs4
        End If

        If B(K) * Side = Queen Then
          'J = 7  ' ### original remark so Continue For
          QF = 1
          If Side = SWP_STM Then
            Atkno += 1
            SwapOffAtk(Atkno) = 950
            Continue For                         ' GoTo qs4
            ' ElseIf Side = -SWP_STM Then
          Else
            Defno += 1
            SwapOffDef(Defno) = 950
            Continue For                         ' GoTo qs4
          End If
        End If

        If I < 5 And QF = 1 And B(K) * Side = Bishop Then
          If Side = SWP_STM Then
            SwapOffAtk(Atk_bp_list_pos) = 950
            SwapOffAtk(Atkno) = 325
            'J = 7
            Exit For                             ' GoTo qs4
            ' ElseIf Side = -SWP_STM Then
          Else
            SwapOffDef(Def_bp_list_pos) = 950
            SwapOffDef(Defno) = 325
            'J = 7
            Exit For                             ' GoTo qs4
          End If
        End If

        If I > 4 And QF = 1 And B(K) * Side = Rook Then
          If Side = SWP_STM Then
            SwapOffAtk(Atk_Rk_list_pos) = 950
            SwapOffAtk(Atkno) = 500
            'J = 7
            Exit For                             ' GoTo qs4
            ' ElseIf Side = -SWP_STM Then
          Else
            SwapOffDef(Def_Rk_list_pos) = 950
            SwapOffDef(Defno) = 500
            'J = 7
            Exit For                             ' GoTo qs4
          End If
        End If

        If I < 5 And B(K) * Side > 0 And (B(K) * Side = Bishop) Then
          Continue For                           ' GoTo qs4
        ElseIf I < 5 And B(K) * Side > 0 Then
          'J = 7
          Exit For                               ' GoTo qs4
        End If

        If I > 4 And B(K) * Side > 0 And (B(K) * Side = Rook) Then
          Continue For                           ' GoTo qs4
        ElseIf I > 4 And B(K) * Side > 0 Then
          'J = 7
          Exit For                               ' GoTo qs4
        End If

        If B(K) * Side < 0 Then
          'J = 7
          Exit For
        End If

        'qs4:

      Next J

      'K = RR

    Next I

    'K = RR

    qs4a:
    '*******************************
    'test for recapture by king
    '*******************************

    For I = 1 To 8                               'directions for kings
      ' ### replaced var p with I
      K = RR + Move_Offset(I)

      If B(K) * Side <> King Then Continue For   ' Goto qs5
      'p = 8 ' only 1 king so if found can't repeat!
      If Side = SWP_STM Then
        Atkno += 1
        SwapOffAtk(Atkno) = 9999
        Exit For                                 ' GoTo qs5 ' ## we can exit loop
        ' ElseIf Side = -SWP_STM Then
      Else
        Defno += 1
        SwapOffDef(Defno) = 9999
        Exit For                                 ' GoTo qs5 ' ## we can exit loop
      End If

      'qs5:

      'K = RR

    Next I

    'K = RR

  Next t

  If Atkno = 0 Then Return Stemp_1               'GoTo qs20  ' nothing to take with!

  If Atkno + Defno = 0 Then Return Stemp_1       'GoTo qs20  ' if flag not set then no swap-off value to do

  If Defno >= Atkno Then u2 = Atkno Else If Defno < Atkno Then u2 = Defno + 1

  For x = 1 To u2                                ' assume that there won't be more than 8 successive recaptures!

    If x = 1 Then
      wgain(x) = Vo - (SwapOffAtk(x))
      bgain(x) = Vo
    End If

    If x = 2 Then
      wgain(x) = Vo - (SwapOffAtk(x - 1)) + (SwapOffDef(x - 1)) - (SwapOffAtk(x))
      bgain(x) = Vo - (SwapOffAtk(x - 1)) + (SwapOffDef(x - 1))
    End If

    If x = 3 Then
      wgain(x) = Vo - (SwapOffAtk(x - 2)) + (SwapOffDef(x - 2)) - (SwapOffAtk(x - 1)) + (SwapOffDef(x - 1)) - (SwapOffAtk(x))
      bgain(x) = Vo - (SwapOffAtk(x - 2)) + (SwapOffDef(x - 2)) - (SwapOffAtk(x - 1)) + (SwapOffDef(x - 1))
    End If

    If x = 4 Then
      wgain(x) = Vo - (SwapOffAtk(x - 3)) + (SwapOffDef(x - 3)) - (SwapOffAtk(x - 2)) + (SwapOffDef(x - 2)) - (SwapOffAtk(x - 1)) + (SwapOffDef(x - 1)) - (SwapOffAtk(x))
      bgain(x) = Vo - (SwapOffAtk(x - 3)) + (SwapOffDef(x - 3)) - (SwapOffAtk(x - 2)) + (SwapOffDef(x - 2)) - (SwapOffAtk(x - 1)) + (SwapOffDef(x - 1))
    End If

  Next x

  ' insert last value replica
  If Defno >= Atkno Then bgain(u2 + 1) = wgain(u2) Else wgain(u2) = bgain(u2)

  For i = 1 To u2

    If i = 1 Then
      id1 = wgain(i)
      id = i
      pid1 = bgain(i)
      pid = i
    End If

    If i > 1 And id1 < wgain(i) Then
      id1 = wgain(i)
      id = i
    End If

    If i > 1 And pid1 > bgain(i) Then
      pid1 = bgain(i)
      pid = i
    End If

    If Defno >= Atkno And i = u2 And pid1 > bgain(u2 + 1) Then
      pid1 = bgain(u2 + 1)
      pid = u2 + 1
    End If

  Next i

  id_copy = id
  pid_copy = pid


  If Defno >= Atkno Then
    Stemp_1 = id1
    Do
      If bgain(id_copy) < Stemp_1 Then Stemp_1 = bgain(id_copy)
      id_copy -= 1
    Loop Until id_copy <= 1
  End If

  If Defno < Atkno Then
    Stemp_1 = pid1
    Do
      If wgain(pid_copy) > Stemp_1 Then Stemp_1 = wgain(pid_copy)
      pid_copy -= 1
    Loop Until pid_copy <= 1
  End If

  'qs20:

  Return Stemp_1

End Function

'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
Function assessboard(side As Integer, best_score As Integer) As Integer

  Dim As Integer game_end = FALSE, valid_move, thru_check, temp_user_var, Check_root
  ' Dim As Byte Check_root
  Dim As Integer w, ps, sm, cm                      ', i1
  Dim As Integer rep_pos, n1, x, y
  Dim As String posit_str  ', n1_str

  
  If fifty_move = 100 And WinboardMode = 1 Then SendCommand("offer draw")  'added 28414 (can't end game with result
 
  
  
  If fifty_move > 100 And WinboardMode = 1 Then ' changed to > 100 28414 (def draw if >100)
    SendCommand( "1/2-1/2 {Draw by 50 move rule}")
    Return TRUE
  ElseIf fifty_move > 100 And WinboardMode = 0 Then
    display
    Print "Draw by 50 move rule!"
    Return TRUE                              'GoTo asb7
  End If

  Side = -Side

  depth = 1
  'nb line below commented out 14/11/11 as engine was resigning instead of playing pxpep to escape from checkmate; line below is too restrictive as pxpep can be valid option to escape mate - needed to add in line 2634
  'epsqr(depth) = -1 ' ignore ep square in genmove as we are testing if own side is in check therefore not relevant eg 1 f3 e5 2 g4 Qh4++ (ep should not be 47 otherwise hg is recorded as possible move)
  If act_epsq <> - 1 Then epsqr(depth) = act_epsq'set ep sqr if computer last moved pawn 2 squares
  b_casflags(0, 1) = b_cas(1) : b_casflags(0, 2) = b_cas(2)   ' need to reset search flags for ply 3 before generating moves to test legality

  w_casflags(0, 1) = w_cas(1) : w_casflags(0, 2) = w_cas(2)

  gen_move(side, depth)

  'valid_move = FALSE

  For w = 1 To Pseu_Moves(depth)
    MoveFrom = Move_List(depth, w, 1) : MoveTo = Move_List(depth, w, 2)
    '===============================================================
    ' only legal castling moves are generated, making test redundant
    '===============================================================

    temp_user_var = B(MoveTo)
    B(MoveTo) = B(MoveFrom)
    B(MoveFrom) = 0

    If B(MoveTo) = 7 Then wkloc = MoveTo Else If B(MoveTo) = -7 Then bkloc = MoveTo

    Check_root = incheck(side)       ' test to see if move results in check

    ' if so then illegal and take-back move
    If Check_root = TRUE Then valid_move = FALSE Else valid_move = TRUE

    'valid move if get out check by PxPep!
    If (Check_root = TRUE And B(MoveTo) = 1 And MoveTo = epsqr(depth)) Or (Check_root = TRUE And B(MoveTo) = -1 And MoveTo = epsqr(depth)) Then valid_move = TRUE

    B(MoveFrom) = B(MoveTo)                      'undo move
    B(MoveTo) = temp_user_var

    If B(MoveFrom) = 7 Then wkloc = MoveFrom Else If B(MoveFrom) = -7 Then bkloc = MoveFrom

    ' only need 1 valid move - no need to continue if a valid move is found
    If valid_move = TRUE Then w = Pseu_Moves(depth)

  Next w

  'asb3:

  'If valid_move = FALSE And incheck(side) = FALSE Then SM = 1 Else SM = 0
  'If valid_move = FALSE And incheck(side) = TRUE Then CM = 1 Else CM = 0
  If valid_move = FALSE Then                        ' ###
    If incheck(side) = FALSE Then                   ' ###
      sm = 1 : cm = 0                               ' ###
    Else                                            ' ###
      sm = 0 : cm = 1                               ' ###
    End If                                          ' ###
  End If                                            ' ###

  Side = -Side

  If STM = Opponent Then GoTo asb4

  ' changed 4/10/09 following bug report from Leo - was black
  If WinboardMode = 1 And CM = 1 And Side = White Then
    SendCommand( "1-0 {Checkmate - Lost again!}")
    Return TRUE                              'GoTo asb7
  End If

  ' changed 4/10/09 following bug report from Leo - was white
  If WinboardMode = 1 And CM = 1 And Side = Black Then
    SendCommand( "0-1 {Checkmate - Lost again!}")
    Return TRUE                              'GoTo asb7
  End If

  If WinboardMode = 1 and GIVEUP = 1 and Side = White then
     SendCommand("0-1 {White resigns}")
     Return TRUE 
  End If

  If WinboardMode = 1 and GIVEUP = 1 and Side = Black then
     SendCommand("1-0 {Black resigns}")
     Return TRUE
  End If


  If WinboardMode = 0 And CM = 1 Then
    display
    Print "Congratulations! - checkmate you win!"
    Return TRUE                              'GoTo asb7
  End If

  If WinboardMode = 0 and GIVEUP = 1 then
        display
        print "Congratulations! - I resign!"
        Return TRUE
  End If


  If SM = 1 And WinboardMode = 0 Then
    display
    Print "Draw by stalemate!"
    Return TRUE                              'GoTo asb7
  End If

  If WinboardMode = 1 And SM = 1 Then
    SendCommand( "1/2-1/2 {Stalemate}")
    Return TRUE                              'GoTo asb7
  End If

  GoTo asb5

  asb4:

  If WinboardMode = 1 And CM = 1 And Side = Black Then
    SendCommand( "0-1 {Checkmate - I won!}")
    Return TRUE 
  End If

  If WinboardMode = 1 And CM = 1 And Side = White Then
    SendCommand( "1-0 {Checkmate - I won!}")
    Return TRUE 
  End If

  If winboardMode = 0 And CM = 1 Then
    display
    Print "Checkmate - I win!"
    Return TRUE                              'GoTo asb7
  End If

  If SM = 1 And WinboardMode = 0 Then
    display
    Print "Draw by stalemate!"
    Return TRUE                              'GoTo asb7
  End If

  If WinboardMode = 1 And SM = 1 Then
    SendCommand( "1/2-1/2 {Stalemate}")
    Return TRUE                              'GoTo asb7
  End If

  asb5:

  If WinboardMode = 1 And IM = 1 Then
    SendCommand( "1/2-1/2 {Draw by insufficient material}")
    Return TRUE
  ElseIf WinboardMode = 0 And IM = 1 Then
    display
    Print "Draw by insufficient material!"
    Return TRUE
  End If

  'If game_end = TRUE Then Return game_end        'GoTo asb7

  ' asb6:

  rep_pos = 0
  posit_str = ""
  N1 = 0
  For X = 90 To 20 Step -10                     ' ###
    For Y = X + 1 To X + 8                      ' ###
      ' If B(X*10 + Y) = 0 Then
      If B(Y) = 0 Then
        N1 += 1 : Continue For                   ' GoTo rp4
      End If
      ' If N1 = 0 Then GoTo rp2
      If N1 <> 0 Then
        ' N1_str = Str(N1)
        ' posit_str +=Right(N1_str,Len(N1_str))
        posit_str += Trim(Str(N1))               ' ###
        N1 = 0
      End If

      '  rp2:
      '  I1 = B(X*10 + Y)
      '  If I1 < 0 Then GoTo rp3
      '  posit_str += B_str(I1)
      '  GoTo rp4
      '  rp3:
      '  posit_str += B_str(I1 + 20)
      '  rp4:

      If B(Y) > 0 Then                           ' ###
        posit_str += B_str(B(Y))
      Else
        posit_str += B_str(B(Y) + 20)
      End If

    Next Y
  Next X

  If N1 <> 0 Then                                'added 15/10/09 (from N04C3 version - failing to pick up blank squares at end of posit (see rep_check)
    ' N1_str = Str(N1)
    ' posit_str +=Right(N1_str,Len(N1_str))
    posit_str += Trim(Str(N1))                   ' ###
  End If

  'store current game position
  If Side = White Then Pos_State_W_str(MoveNo) = posit_str Else Pos_State_B_str(MoveNo) = posit_str

  'If Side = Black Then GoTo rp5
  If Side = White Then                           ' ###
    'trip though all previous positions and compare with current board position
    For x = 1 To MoveNo
      If posit_str = Pos_State_W_str(x) Then
        rep_pos += 1
      End If
    Next

    'GoTo rp6
  Else
    ' rp5:
    For x = 1 To MoveNo
      If posit_str = Pos_State_B_str(x) Then
        rep_pos += 1
      End If
    Next
  End If
  'rp6:




  If rep_pos = 3 then                       'added draw code 28414
     if WinboardMode = 1 then
      SendCommand("offer draw")
      end if
  End if



  If rep_pos = 4 Then 'changed from 3 to 4 (28414) - def draw by now!
    game_end = TRUE
    If WinboardMode = 1 Then
      SendCommand( "1/2-1/2 {Draw by repetition}")
    Else
      display
      Print "Draw by 3 fold repetition!!!!"
    End If
  End If

  'asb7:

  Return game_end

End Function

'/
'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
Function rep_check As Integer

  ' Dim As Byte rep_draw = FALSE
  Dim As Integer wht_ingame_rep_pos, blk_ingame_rep_pos
  Dim As String ingame_posit_str                 ' , rc_str,
  Dim As Integer x, y, rc, irc1                     ' , t

  'RC = 0

  For X = 90 To 20 Step -10                     ' ###
    For Y = X+1 To X+8                          ' ###
      ' If B(X*10 + Y) = 0 Then
      IRC1 = B(Y)                               ' ###
      If IRC1 = 0 Then                          ' ###
        RC += 1
        Continue For                            ' GoTo repc4
      End If

      'If RC = 0 Then  GoTo repc2
      If RC <> 0 Then
        'rc_str = Str(RC)
        'ingame_posit_str += Right(rc_str,Len(rc_str))
        ingame_posit_str += Trim(Str(RC))        ' ###
        RC = 0
      End If

      'repc2:
      ' IRC1 = B(X*10 + Y)
      If IRC1 < 0 Then ingame_posit_str += B_str(IRC1 + 20) Else ingame_posit_str += B_str(IRC1)

      'repc4:
    Next Y
  Next X

  If RC <> 0 Then                                'added 15/10/09 - failing to pick up blank squares at end of ingame_posit
    'rc_str = Str(RC)
    'ingame_posit_str +=Right(rc_str,Len(rc_str))
    ingame_posit_str += Trim(Str(RC))            ' ###
  End If

  'trip though positions from last 7 moves and compare with current board position
  If moveno > 7 Then ' ###
    For X = MoveNo To (MoveNo - 7) Step - 1         ' ### replaced t by x
      If ingame_posit_str = Pos_State_W_str(X) Then wht_ingame_rep_pos += 1
      If ingame_posit_str = Pos_State_B_str(X) Then blk_ingame_rep_pos += 1
    Next
  End If ' ###

  'If side = white And (wht_ingame_rep_pos >= 1) Then rep_draw = TRUE ' only 1 because already have board which makes 2 (return 0.00 after 2nd rep for safety)
  'If side = black And (blk_ingame_rep_pos >= 1) Then rep_draw = TRUE 'as above
  ' ###
  If side = white And (wht_ingame_rep_pos >= 1) Then Return TRUE
  ' ###
  If side = black And (blk_ingame_rep_pos >= 1) Then Return TRUE

  Return FALSE

End Function
'-------------------------------------------------

Sub gen_move(side As Integer, depth As UInteger)

  'Cls
  '*************************************
  '      Generate legal moves
  '  (this routine is called for both
  '  engine and human moves)
  '*************************************
  Dim As UInteger piece, m, n                        ', i, j
  Dim As Integer Move 'Pawn_Move, KhtMove, BpMove, RkMove, KingMove all renamed to move
  'Dim As Integer King_Proximity
  'Dim As Integer king_loc = IIf(side = white, bkloc, wkloc)
  'Pseu_Moves(depth) = 0                          ' reset counter
  Dim As UInteger count  ' ###

  For Sq As Integer = 21 To 98

    If B(Sq) = 0 Or B(Sq) = -99 Then Continue For  ' goto g_m_next ' if empty loop back
    If B(Sq) * Side < 0 Then Continue For          ' goto g_m_next ' if opponent .

    'If B(Sq) = -99 Then Continue For              ' goto g_m_next ' if off board ...or

    Piece = Abs(B(Sq))                             ' establish absolute piece value (ie Knight, Bishop etc)

    Select Case Piece
      Case 1
        GoTo fh1
      Case 2
        GoTo m_g1
      Case 3
        GoTo m_g2
      Case 4
        GoTo m_g3
      Case 6
        GoTo m_g2
      Case 7
        GoTo m_g4
    End Select

    '***** White pawn moves *****

    fh1:
    If Side = white Then                         ' GoTo b_p_m

      For n = 1 To 2                             ' captures first

        Move = Sq + Move_Offset(n)

        If B(Move) > 0 Then Continue For    'goto w_p1 'if error loop again
        If B(Move) = -99 Then Continue For 'goto w_p1 'if error loop again

        'If Pawn_Move = epsqr(depth) And B(Pawn_Move) = 0 Then GoTo wep1
        'If B(Pawn_Move) = 0 Then Continue For  'goto w_p1  'loop if not En passant

        If B(Move) = 0 Then
          If Move = epsqr(depth) Then
            GoTo wep1
          Else
            Continue For                         'goto w_p1  'loop if not En passant
          End If
        End If

        If Move > 90 Then
          For m = 1 To 4
            count += 1
            Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
            If m = 1 Then
              Move_List(Depth, count, 3) = 6
            Else
              Move_List(Depth, count, 3) = 6 - m
            End If
            ' If m = 2 Then Move_List(Depth, count, 3) = 4
            ' If m = 3 Then Move_List(Depth, count, 3) = 3
            ' If m = 4 Then Move_List(Depth, count, 3) = 2
          Next m
          Continue For                           'goto w_p1
        End If

        wep1:
        count += 1
        Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
        ' w_p1:
      Next n

      Move = Sq + 10
      If B(Move) <> 0 Then Continue For     ' goto g_m_next
      If Move > 90 Then
        For m = 1 To 4
          count += 1
          Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
          If m = 1 Then
            Move_List(Depth, count, 3) = 6
          Else
            Move_List(Depth, count, 3) = 6 - m
          End If
          'If m = 2 Then Move_List(Depth, count, 3) = 4
          'If m = 3 Then Move_List(Depth, count, 3) = 3
          'If m = 4 Then Move_List(Depth, count, 3) = 2
        Next m
        Continue For                             ' goto g_m_next
      End If
      count += 1
      Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
      Move += 10
      If move < 59 And B(Move) = 0 Then
        count += 1
        Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
      End If

      Continue For                               'goto g_m_next

    End If                                       ' b_p_m:
    '***** Black pawn moves *****

    'For I = 1 To 2  ' captures first

    'Pawn_Move = Sq + Move_Offset(I+2)
    For n = 3 To 4                               ' captures first

      Move = Sq + Move_Offset(n)

      If B(Move) < 0 Then Continue For      'goto b_p1 this test will also filter out if square = -99
      ' If B(Pawn_Move) = -99 Then Continue For  'goto b_p1 'if error loop again

      'If Pawn_Move = epsqr(depth)And B(Pawn_Move) = 0 Then GoTo bep1
      'If B(Pawn_Move) = 0 Then Continue For  'goto b_p1

      If B(Move) = 0 Then
        If Move = epsqr(depth) Then
          GoTo bep1
        Else
          Continue For
        End If
      End If

      If Move < 29 Then
        For m = 1 To 4
          count += 1
          Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
          If m = 1 Then
            Move_List(Depth, count, 3) = -6
          Else
            Move_List(Depth, count, 3) = m - 6
          End If
          'If m = 2 Then Move_List(Depth, count, 3) = -4
          'If m = 3 Then Move_List(Depth, count, 3) = -3
          'If m = 4 Then Move_List(Depth, count, 3) = -2
        Next m
        Continue For                             'goto b_p1
      End If

      bep1:
      count += 1
      Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move

      ' b_p1:
    Next n

    Move = Sq - 10
    If B(Move) <> 0 Then Continue For       ' goto g_m_next
    If Move < 29 Then
      For m = 1 To 4
        count += 1
        Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
        If m = 1 Then
          Move_List(Depth, count, 3) = -6
        Else
          Move_List(Depth, count, 3) = m - 6
        End If
        'If m = 2 Then Move_List(Depth, count, 3) = -4
        'If m = 3 Then Move_List(Depth, count, 3) = -3
        'If m = 4 Then Move_List(Depth, count, 3) = -2
      Next m
      Continue For                               ' goto g_m_next
    End If
    count += 1
    Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
    Move -= 10
    If move > 60 And B(Move) = 0 Then
      count += 1
      Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
    End If
    Continue For                                 'goto g_m_next

    m_g1:
    '    '***** knight legal moves  *****

    ' generate Knight moves

    For N = 9 To 16
      Move = Sq + Move_Offset(N)
      If Side = White And B(Move) > 0 Then Continue For    ' GOTO k_n
      If Side = Black And B(Move) < 0 Then Continue For    ' GOTO k_n
      If B(Move) = -99 Then Continue For                   ' GOTO k_n
      ' If B(KhtMove) * Side = -King Then Continue For  'GOTO k_n ' can't take king!

      count += 1
      Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
      ' k_n:
    Next N

    Continue For                                 'goto g_m_next

    m_g2:

    '***** Non-pinned Bishop legal moves *****

    For N = 1 To 4                               ' bishop can move in 4 directions
      Move = Sq                                ' ###
      For M = 1 To 7                             ' for a maximum of 7 squares

        ' BpMove = Sq + Move_Offset(N) * M
        Move += Move_Offset(N)                 '###

        ' IF (B(BpMove) = -99) OR (B(BpMove) * Side > 0) OR (B(BpMove) * Side = -King) THEN
        ' M = 7
        ' GOTO b_p
        ' END IF

        If (B(Move) * Side > 0) Or (B(Move) = -99) Then
          'M = 7
          Exit For                               'GOTO b_p
        End If

        ' If (B(BpMove) = -99) Then
        ' M = 7
        ' GOTO b_p
        ' End If

        ' If (B(BpMove) * Side = -King) Then
        ' M = 7
        ' Exit For  'GOTO b_p
        ' End If

        If B(Move) * Side < 0 Then M = 7

        count += 1
        Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move

        'b_p:
      Next M
    Next N

    If Piece <> Queen Then Continue For          'goto g_m_next

    m_g3:

    '***** rook legal moves ******

    For N = 5 To 8
      Move = Sq                                '###
      For M = 1 To 7

        'RkMove = Sq + Move_Offset(N) * M
        Move += Move_Offset(N)                 ' ###

        ' IF (B(RkMove) = -99) or (B(RkMove) * Side > 0) OR (B(RkMove) * Side = -King) THEN
        ' M = 7
        ' GOTO r_m
        ' END IF

        If (B(Move) * Side > 0) Or (B(Move) = -99) Then
          ' M = 7
          Exit For                               'GOTO r_m
        End If

        ' If (B(RkMove) = -99) Then
        ' M = 7
        ' GOTO r_m
        ' End If

        ' If (B(RkMove) * Side = -King) Then
        ' M = 7
        ' Exit For  'GOTO r_m
        ' End If

        If B(Move) * Side < 0 Then M = 7

        count += 1
        Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move

        'r_m:
      Next M
    Next N

    Continue For                                 'goto g_m_next

    m_g4:

    '***** king legal moves *****

    'King_Proximity = 0 'adjacent kings flag
    '  ### side * -King
    Dim As Integer King_Proximity
    Dim As Integer king_loc = IIf(side = white, bkloc, wkloc)
    Dim As Integer side_king = IIf(side = white, -7, 7)

    For M = 1 To 8                               'all possible king move directions

      Move = Sq + Move_Offset(M)             'work out potential moves

      ' GoTo k_m
      If B(Move) = -99 Or B(Move) * Side > 0 Then Continue For
      If Abs(move - king_loc) < 12 Then      ' ### if distance is greater than 11, no enemy king's adjecent
        For N = 1 To 8                           ' loop to check whether king is adjacent
          ' (Side * -King) Then
          If B(Move + Move_Offset(N)) = side_king Then
            King_Proximity = 1                ' flag to exclude any such moves (ie adjacent squares)
            'N = 8
            Exit For
          End If
        Next N
      End If

      If King_Proximity = 0 Then
        count += 1
        Move_List(Depth, count, 1) = Sq : Move_List(Depth, count, 2) = Move
      Else
        King_Proximity = 0
      End If

      ' k_m:
    Next M

    '**********************************
    'next deal with castling options
    '1 - White castle options
    '  need to build in castle flags still
    '**********************************
    ' ### only generate castling moves if they are possible
    ' ### makes other test's elsewhere redundant

    If Side = White Then
      If wkloc = 25 And (w_cas(1)= 0 Or w_cas(2) = 0) Then   ' ### is castling allowed
        Dim As UInteger flag = TRUE
        ' ### any black piece on sq 37 makes castling impossible
        If w_cas(1) = 0 And B(26) = 0 And B(27) = 0 And B(28) = 4 And B(37) >= 0 Then
          flag = incheck(white)
          If flag = TRUE Then Continue For
          If attack(white, 26) = FALSE And attack(white, 27) = FALSE Then ' ### incheck(white) was false
            count += 1
            Move_List(Depth, count, 1) = 25 : Move_List(Depth, count, 2) = 27
          End If
        End If
        ' ### any black piece on sq 33 makes castling impossible
        If w_cas(2) = 0 And B(22) = 0 And B(23) = 0 And B(24) = 0 And B(21) = 4 And B(32) <> -7 And B(33) >= 0 Then
          ' ### if flag = true than no incheck(white) was we need to do it now
          If flag = TRUE Then If incheck(white) = TRUE Then Continue For
          If attack(white, 23) = FALSE And attack(white, 24) = FALSE Then
            count += 1
            Move_List(Depth, count, 1) = 25 : Move_List(Depth, count, 2) = 23
          End If
        End If
      End If

      Continue For

      '**********************************
      '2 - Black castle options
      '**********************************

    Else   ' ### black castling
      If bkloc = 95 And (b_cas(1) = 0 Or b_cas(2) = 0) Then
        Dim As UInteger flag = TRUE
        ' ### any white piece on sq 87 makes castling impossible
        If b_cas(1) = 0 And B(96) = 0 And B(97) = 0 And B(98) = -4 And B(87) < 1 Then
          flag = incheck(black)
          If flag = TRUE Then Continue For
          If attack(black, 96) = FALSE And attack(black, 97) = FALSE Then
            count += 1
            Move_List(Depth, count, 1) = 95 : Move_List(Depth, count, 2) = 97
          End If
        End If
        ' ### any black piece on sq 83 makes castling impossible
        If b_cas(2) = 0 And B(92) = 0 And B(93) = 0 And B(94) = 0 And B(91) = -4 And B(82) <> 7 And B(83) < 1 Then
          ' ### if flag = true than no incheck(black) was we need to do it now
          If flag = TRUE Then If incheck(black) = TRUE Then Continue For
          If attack(black, 93) = FALSE And attack(black, 94) = FALSE Then
            count += 1
            Move_List(Depth, count, 1) = 95 : Move_List(Depth, count, 2) = 93
          End If
        End If
      End If

    End If                                       '###

    ' loop back to check next square
    'g_m_next:

  Next Sq

  Pseu_Moves(depth) = count ' ###


End Sub

'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

Function incheck(side As Integer) As Integer
  ' ### faster version replacing the old code
  ' ### Assume that check by Q/R/B is more likely than by N or pawn
  Dim As Integer K = IIf(side = white, wkloc, bkloc), R = K

  '************************************
  'test for check by Rook / Queen
  '************************************
  Dim As Integer val_queen = IIf(side = White, -6, 6)
  Dim As Integer val_rook = IIf(side = White, -4, 4)

Do : K -= 10 : Loop Until (B(k) <> 0)
If B(k) = val_rook Or B(k) = val_queen Then Return TRUE
k = r : Do : K -= 1 : Loop Until (B(k) <> 0)
If B(k) = val_rook Or B(k) = val_queen Then Return TRUE
k = r : Do : K += 1 : Loop Until (B(k) <> 0)
If B(k) = val_rook Or B(k) = val_queen Then Return TRUE
k = r : Do : K += 10 : Loop Until (B(k) <> 0)
If B(k) = val_rook Or B(k) = val_queen Then Return TRUE

'************************************
'test for check by Bishop / Queen
'************************************

Dim As Integer val_bishop = IIf(side = White, -3, 3)

k = r : Do : K -= 11 : Loop Until (B(k) <> 0)
If B(k) = val_bishop Or B(k) = val_queen Then Return TRUE
k = r : Do : K -= 9 : Loop Until (B(k) <> 0)
If B(k) = val_bishop Or B(k) = val_queen Then Return TRUE
k = r : Do : K += 9 : Loop Until (B(k) <> 0)
If B(k) = val_bishop Or B(k) = val_queen Then Return TRUE
k = r : Do : K += 11 : Loop Until (B(k) <> 0)
If B(k) = val_bishop Or B(k) = val_queen Then Return TRUE

'************************************
'test for check by Knight
'************************************

Dim As Integer val_knight = IIf(side = White, -2, 2)

If B(R - 21) = val_knight Then Return TRUE
If B(R - 19) = val_knight Then Return TRUE
If B(R - 12) = val_knight Then Return TRUE
If B(R - 8)  = val_knight Then Return TRUE
If B(R + 8)  = val_knight Then Return TRUE
If B(R + 12) = val_knight Then Return TRUE
If B(R + 19) = val_knight Then Return TRUE
If B(R + 21) = val_knight Then Return TRUE

'***********************************
' test for check by pawn
'***********************************

If side = white Then
  If b(r + 9) = -1 Or b(r + 11) = -1 Then Return TRUE
Else
  If b(r - 9) = 1 Or b(r - 11) = 1 Then Return TRUE
End If

Return FALSE

End Function


'$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

Function attack(side As Integer, ps As Integer) As Integer
  ' ### faster version replacing the old code
  ' ### Assume that check by Q/R/B is more likely than by N or pawn
  ' ### reduced testing since the square's are at the bord edge
  Dim As Integer r = ps

  If side = white Then

    '************************************
    'test for check by Rook / Queen
    '************************************

Do : ps += 10 : Loop Until (B(ps)<> 0)
If B(ps)= -4 Or B(ps)= -6 Then Return TRUE

'************************************
'test for check by Bishop / Queen
'************************************

ps = r : Do : ps += 9 : Loop Until (B(ps)<> 0)
If B(ps)= -3 Or B(ps)= -6 Then Return TRUE
ps = r : Do : ps += 11 : Loop Until (B(ps)<> 0)
If B(ps)= -3 Or B(ps)= -6 Then Return TRUE

'************************************
'test for check by Knight
'************************************

If B(r + 8)  = -2 Then Return TRUE
If B(r + 12) = -2 Then Return TRUE
If B(r + 19) = -2 Then Return TRUE
If B(r + 21) = -2 Then Return TRUE

'***********************************
' test for check by pawn
'***********************************

If B(r + 9) = -1 Or B(r + 11) = -1 Then Return TRUE

  Else 'black

    '************************************
    'test for check by Rook / Queen
    '************************************

Do : ps -= 10 : Loop Until (B(ps)<> 0)
If B(ps)= 4 Or B(ps)= 6 Then Return TRUE

'************************************
'test for check by Bishop / Queen
'************************************

ps = r : Do : ps -= 9 : Loop Until (B(ps)<> 0)
If B(ps)= 3 Or B(ps)= 6 Then Return TRUE
ps = r : Do : ps -= 11 : Loop Until (B(ps)<> 0)
If B(ps)= 3 Or B(ps)= 6 Then Return TRUE

'************************************
'test for check by Knight
'************************************

If B(r - 8)  = 2 Then Return TRUE
If B(r - 12) = 2 Then Return TRUE
If B(r - 19) = 2 Then Return TRUE
If B(r - 21) = 2 Then Return TRUE

'***********************************
' test for check by pawn
'***********************************

If B(r - 9) = 1 Or B(r - 11) = 1 Then Return TRUE

  End If

  Return FALSE

End Function


'-----------------------------------------------------------------
'function undo()

'Print "in undo"
'Print "side = ";side
'Print "undo "; move_hist(MoveNo,1); "- ";move_hist(MoveNo,2)

'if side = black then              ' update move history with current move
'     B(move_hist(MoveNo,1)) = B(move_hist(MoveNo,2))
'      B(move_hist(MoveNo,2)) = TB_piece
'      Print "B";move_hist(MoveNo,1); "=";B(move_hist(MoveNo,1)); " ";"B";move_hist(MoveNo,2);"=";TB_piece

'     move_hist(MoveNo,1) = 0
'     move_hist(MoveNo,2) = 0
' elseif side = black then
'     move_hist(MoveNo,3) = B_M_From
'     move_hist(MoveNo,4) = B_M_To
'     end if

'end function

'/
'-------------------------------------------------------------------
Sub clear_board
  'Print "clear_board"
  Dim As Integer x, y

  For x = 0 To 119
    B(x) = -99
  Next x

  For X = 20 To 90 Step 10                      ' ###
    For Y = X + 1 To X + 8                      ' ###
      B(Y) = 0                                  ' ###
    Next Y
  Next X

End Sub

'----------------------------------------------------------------------------


Sub fen(o_str As String)

  clear_board

  Dim As Integer valid_fen = TRUE
  'Dim As Byte fen_check = FALSE
  Dim As Integer b_king_no, w_king_no
  Dim As Integer b_loc = 90, new_loc = 90           'a8
  Dim As Integer a_h, r_count, f_ield
  Dim As UInteger bpn, wpn
  Dim As Integer fmfen, mcfen
  Dim As String fmstring, mvstring
  Dim As UInteger w                                 ' , z = Len(o_str)
  Dim As String pce, pce_last
  Dim As UInteger num

  mat_left = 0                                   ' ###
  No_Pieces = 0                                  ' ###

  For w = 10 To Len(o_str)                       ' z

    num = Val(Mid(o_str, w, 1))                  'get number from string

    If f_ield > 3 And num <> 0 Then GoTo e2

    If num <> 0 Then
      new_loc += num
      a_h += num
      Continue For                               ' GoTo n1
    End If

    pce = Mid(o_str, w, 1)

    new_loc += 1

    If f_ield <> 0 Then GoTo e1

    Select Case pce
      Case "p"
        No_Pieces += 1
        B(new_loc) = -1
        bpn += 1                                 ' increment pawn counter
        ' pawn can't be on 1/8th rank
        If new_loc > 90 Or new_loc < 29 Then valid_fen = FALSE
      Case "b"
        B(new_loc) = -3
        No_Pieces += 1
        Bbp += 1
        Mat_left += Bishop_Value
      Case "n"
        B(new_loc) = -2
        No_Pieces += 1
        Bkn += 1
        Mat_left += Knight_Value
      Case "r"
        B(new_loc) = -4
        No_Pieces += 1
        br += 1
        Mat_left += Rook_Value
      Case "q"
        B(new_loc) = -6
        No_Pieces += 1
        BQ += 1
        Mat_left += Queen_Value
      Case "P"
        B(new_loc) = 1
        No_Pieces += 1
        wpn += 1
        If new_loc > 90 Or new_loc < 29 Then valid_fen = FALSE
      Case "B"
        B(new_loc) = 3
        No_Pieces += 1
        Wbp += 1
        Mat_left += Bishop_Value
      Case "N"
        B(new_loc) = 2
        No_Pieces += 1
        Wkn += 1
        Mat_left += Knight_Value
      Case "R"
        B(new_loc) = 4
        No_Pieces += 1
        wr += 1
        Mat_left += Rook_Value
      Case "Q"
        B(new_loc) = 6
        No_Pieces += 1
        WQ += 1
        Mat_left += Queen_Value
      Case "k"
        b_king_no += 1
        No_Pieces += 1
        bkloc = new_loc
        'If b_king_no = 1 Then B(new_loc) = -7 Else valid_fen = FALSE
        B(new_loc) = -7
      Case "K"
        w_king_no += 1
        No_Pieces += 1
        wkloc = new_loc
        'If w_king_no = 1 Then B(new_loc) = 7 Else valid_fen = FALSE
        B(new_loc) = 7
      Case "/"
        If a_h = 8 And r_count <= 7 Then
          b_loc -= 10
          new_loc = b_loc
          a_h = -1
          r_count += 1
        ElseIf pce = "/" Then
          valid_fen = FALSE
        End If
    End Select

    a_h += 1

    e1:
    If pce = " " Then
      a_h = 0                                    'reset file counter if finished first field
      f_ield += 1
      pce_last = pce
      Continue For                               ' GoTo n1
    End If

    If F_ield = 0 Then Continue For

    e2:

    If pce_last = " " And f_ield = 1 Then
      If pce = "w" Then side = white
      If pce = "b" Then side = black
    ElseIf pce = " " And f_ield = 1 Then valid_fen = FALSE
    End If

    If pce_last = " " And f_ield = 2 Then
      If pce = "K" Then w_cas(1) = 99
      If pce = "Q" Then w_cas(2) = 99
      If pce = "k" Then b_cas(1) = 99
      If pce = "q" Then b_cas(2) = 99
      If pce = "-" Then Continue For
    End If

    If pce_last = " " And f_ield = 3 And pce = "-" Then act_epsq = -1

    ' need to build in ep square

    If (pce_last = " " And f_ield = 4) Or (fmfen > 0 And f_ield = 4) Then
      fmfen += 1
      If fmfen = 1 Then fmstring = Str(num)
      If fmfen = 2 Then fmstring += Str(num)       ' only need to check 2 digits as > 99 is invalid
      fifty_move = Val(fmstring)
    End If

    If pce_last = " " And f_ield = 5 Or (mcfen > 0 And f_ield = 5) Then
      mcfen += 1
      If mcfen = 1 Then mvstring = Str(num)
      If mcfen = 2 Then mvstring += Str(num)
      If mcfen = 3 Then mvstring += Str(num)
      MoveNo = Val(mvstring)
      conv_clock_move_no = MoveNo
    End If

    If a_h > 8 Or f_ield > 5 Then valid_fen = FALSE

    ' n1:

  Next w

  If Mat_left < 2426 Then endgame = 1

  ' validation checks

  ' ###
  If No_Pieces < 2 And No_Pieces > 32 Then valid_fen = FALSE
  ' ### only 1 white king AND 1 black king allowed
  If w_king_no <> 1 And b_king_no <> 1 Then valid_fen = FALSE
  ' ### only when all condition are met is castling allowed
  If w_cas(1) = 99 And B(25) = 7 And B(28) = 4 Then w_cas(1) = 0 Else w_cas(1) = -1
  If w_cas(2) = 99 And B(25) = 7 And B(21) = 4 Then w_cas(2) = 0 Else w_cas(2) = -1
  If b_cas(1) = 99 And B(95) = -7 And B(98) = -4 Then b_cas(1) = 0 Else b_cas(1) = -1
  If b_cas(2) = 99 And B(95) = -7 And B(91) = -4 Then b_cas(2) = 0 Else b_cas(2) = -1

  If bpn > 8 Or wpn > 8 Then valid_fen = FALSE
  ' ### need test for promoted pawn's

  For w = 1 To 8
    If bkloc - wkloc = Move_Offset(w) Then valid_fen = FALSE
  Next w

  If r_count <> 7 Then valid_fen = FALSE

  ' fen_check = incheck(-side)
  ' If fen_check = TRUE Then valid_fen = FALSE
  If incheck(-side) = TRUE Then valid_fen = FALSE

  If WinboardMode = 0 Then
    display
    If side = white Then
      Print "white ";
    Else
      Print "black ";
    End If
    Print "to move"
    Print "white castling possible:  ";
    If w_cas(1) = 0 Then Print "short (0-0) ";
    If w_cas(2) = 0 Then Print "long (0-0-0)" Else Print
    Print "black castling possible:  ";
    If b_cas(1) = 0 Then Print "short (0-0) ";
    If b_cas(2) = 0 Then Print "long (0-0-0)" Else Print
    Print
    Print "hit any key"
  End If

  If WinboardMode = 0 And valid_fen = FALSE Then
    Print "Invalid fen - please re-enter"
  ElseIf WinboardMode = 1 And valid_fen = FALSE Then
    SendCommand( "Illegal FEN - I'm not playing and I'm quitting!")
    End
  End If

End Sub

'-----------------------------------------------------------------------------

Sub display
  ' ### rewrote the display routine to avoid flipping the board if player = black

  '***************************************
  '           DisplayBoard
  '       Prints board to screen
  '***************************************
  If WinboardMode = 1 Then Return                ' skip if in winboard mode

  Dim As Integer x, y, piece
  Dim As Integer x1 = 9, x2 = 2, x3 = -1
  Dim As Integer y1 = 1, y2 = 8, y3 = 1

  ' if player = white
  ' x1 = 9 : x2 = 2 : x3 = -1
  ' y1 = 1 : y2 = 8 : y3 = 1
  ' if player = black
  ' x1 = 2 : x2 = 9 : x3 = 1
  ' y1 = 8 : y2 = 1 : y3 = -1

  If cmpclr = 1 Then
    Swap x1, x2
    Swap y1, y2
    Swap x3, y3
  End If

  Cls
  Print : Print
  For x = x1 To x2 Step x3
    For y = x*10 + y1 To x*10 + y2 Step y3
      piece = B(y)
      If piece < 0 Then piece += 20
      Print "   "; b_str(piece);
    Next y
    Print "   ***  "; x - 1
    Print "   -----------------------------"
  Next x
  Print "   *********************************** "
  Print "   *********************************** "
  If cmpclr = -1 Then
    Print "   a   b   c   d   e   f   g   h   "
  Else
    Print "   h   g   f   e   d   c   b   a   "
  End If

  If move_hist(MoveNo, 1) = 0 Then
    Print "Numpty's move... <list empty>"
  Else

    If BookHit = 1 Then
      If cmpclr = -1 Then
        Print "Numpty's move... "; move_2_str(move_hist(MoveNo, 3)); "-"; move_2_str(move_hist(MoveNo, 4)); " < Book move >"
      Else
        Print "Numpty's move... "; move_2_str(move_hist(MoveNo, 1)); "-"; move_2_str(move_hist(MoveNo, 2)); " < Book move >"
      End If
    End If

    If MoveNo >= 1 And BookHit = 0 Then
      If cmpclr = 1 Then
        Print "Numpty's move... "; move_2_str(move_hist(MoveNo, 1)); "-"; move_2_str(move_hist(MoveNo, 2)); " Eval = "; best_score
      Else
        Print "Numpty's move... "; move_2_str(move_hist(MoveNo, 3)); "-"; move_2_str(move_hist(MoveNo, 4)); " Eval = "; best_score
      End If
    End If
  End If

  If MoveNo >= 1 And Bookhit = 1 And perft = FALSE Then Print "Total nodes searched = 0" Else Print "Total nodes searched = "; display_node
  Print : Print "---------------------------------------"
  If cmpclr = -1 Then Print "White # Move No:"; MoveNo + 1 Else Print "Black # Move No: "; MoveNo

End Sub

Sub ending

  Dim As UInteger hitspace

  Do
    hitspace = GetKey
  Loop Until hitspace = 32
  'Cls
  End

End Sub

'-------------------------------------------------------------------------------

'*******************************************************
'The core winboard interface code is written by
'Tom McBurney and is used here with his permission
'Its integration and adaptation within NOmega is my own
'and any resulting errors are my responsibility
'several wb features are still to be implemented
'*******************************************************
Sub CommandLoop

  ' Process commands from Winboard.

  Dim As String c1, c2, d1, e1, sWBCmd, sTemp                   ' c1$, c2$, d1S, e1$
  Dim As Single tim
 
  Do
    'Sleep 50                                                   ' ### sleep moved to getcommand
    sWBCmd = GetCommand
    If sWBCmd = "protover 2" Then
      sTemp = "feature myname=" + Chr$(34) + "Numpty_Recharged_64" + Chr$(34) + " sigint=0 sigterm=0 reuse=0 analyze=0 setboard=1 draw=1 name=0" 'added draw=1 28414
      SendCommand(sTemp)
      'SendCommand("feature done=1")
      Continue Do
    ElseIf sWBCmd = "quit" Then
        'savegame : Close #99  'used for debug file 3514
      Exit Do
    ElseIf sWBCmd = "white" Then
      side = white
      cmpclr = -1
      Continue Do
    ElseIf sWBCmd = "black" Then
      side = black
      cmpclr = 1
      Continue Do
    ElseIf sWBCmd = "force" Then
      ForceMode = 1
      Continue Do
    ElseIf sWBCmd = "go" Then
      ForceMode = 0
      'If MoveNo > 8 Then ' ### test now in computer_move
      computer_move
      'ElseIf open_book = FALSE Then
      ' computer_move(side)
      'End If
      side = -side
      Continue Do
    ElseIf sWBCmd = "post" Then
      post = 1
      Continue Do
       ELSEIF sWBCmd = "draw" THEN                   'added draw code 28/4/14
        if best_score <= -100 then
            SendCommand("offer draw")
            ending
        end if
        continue Do
      'ELSEIF sWBCmd = "undo" THEN
      ' call undo()
    ElseIf Left(sWBCmd, 8) = "setboard" Then
      o_str = sWBCmd
      fen(o_str)
      Continue Do
    ElseIf Left(sWBCmd, 5) = "level" Then
      fix_depth = 0
      c1 = Mid(sWBCmd, 6)
      d1 = Mid(c1, 4)
      Move_control = ValInt(c1)
      tim = ValInt(d1)
      If Right(SWBCmd, 2) = " 0" Then conv_clock = 1 Else conv_clock = 0
      If conv_clock = 0 Then inc = ValInt(Right(c1, 2)) * 100
      game_time = tim * 60 * 100
      Continue Do
    ElseIf Left(sWBCmd, 3) = "sd " Then          'handle fix depth search
      fix_depth = 1
      conv_clock = 99
      e1 = Mid(sWBCmd, 4)
      max_depth = ValInt(e1)
      Continue Do
    ElseIf Left(sWBCmd, 4) = "time" Then
      c2 = Mid(sWBCmd, 5)
      time_left = ValInt(c2)
      '  Print "time_left = ";time_left
      ' if made t/c then reset move counter
      If conv_clock = 1 And Move_Control <> 0 And conv_clock_move_no = Move_Control Then conv_clock_move_no = 0
      'line below added 18/6/2010 to take account of setboard cases where ccmn > Move control - eg set position at move 69 for 40/1 time control crashed
      If conv_clock = 1 And Move_Control <> 0 And (conv_clock_move_no > Move_Control) Then conv_clock_move_no = conv_clock_move_no - Move_Control
      ' if conv_clock = 1 then print" TLEFT = "; time_left; " MC = ";Move_Control; " ccmn = ";conv_clock_move_no
      If conv_clock = 1 And Move_Control <> 0 Then
        ' default 0.8 - changed 16/1/2010 'changed from 0.9 - 4/7/10
        think_time = (0.95 * (time_left / (Move_Control - conv_clock_move_no)))
        av_think_time = game_time / Move_Control
      End If
      If conv_clock = 1 And Move_Control = 0 Then
        think_time = (time_left / 30)
        av_think_time = game_time / 80           ' assume average length of game
      End If
      'default 1.5
      If conv_clock = 0 Then think_time = (0.04*(time_left + (40 * inc))) / 1.3
      If game_time <= 6000 Or (tim = 1 And inc < 200) Then post = 0
      Continue Do
    Else
      ' ### looks to be a move, check if it is
      o_str = sWBCmd
      Dim As UInteger tmp = Len(o_str)
      If tmp = 4 Or tmp = 5 Then
        MoveFrom = o_str[0] - 96 + 10 * (o_str[1] - 47)
        If movefrom < 21 Or movefrom > 98 Or b(movefrom) = -99 Then Continue Do ' ### GCC 64x gave error (movefrom) needs to be b(movefrom)
        MoveTo = o_str[2] - 96 + 10 * (o_str[3] - 47)
        If moveto < 21 Or moveto > 98 Or b(moveto) = -99 Then Continue Do
        If tmp = 5 Then pr_str = Right(o_str, 1)
        user_move(Movefrom, MoveTo)
      End If
    End If

  Loop
End Sub


'-------------------------------------------------------------------------------

Function GetCommand As String

  Dim As UInteger Ptr inputHandle = GetStdHandle(STD_INPUT_HANDLE)
  'This function will retrieve one command at a time from winboard.

  Dim As String sBuff, sTemp
  Dim As UInteger iBytesRead, iTotalBytes, iAvailBytes ' ### changed to uinteger
  Dim As Integer iReturnCode
  'GetCommand = ""

  Do   '### loop until we get something
    Sleep 50          ' ### moved from commandloop
    ' Null character will make sBuff a C style empty string
    sBuff = Chr(0)

    ' PeekNamedPipe tells us if there is any data waiting to be retrieved.
    iReturnCode = PeekNamedPipe(InputHandle, ByVal StrPtr(sBuff), 1, @iBytesRead, @iTotalBytes, @iAvailBytes)

    ' return empty string when there is no data to be retrieved.
    If iTotalBytes = 0 Then Exit Function

    ' Retrieve data
    Do
      iReturnCode = ReadFile(InputHandle, ByVal StrPtr(sBuff), 1, @iBytesRead, NULL)
      If sBuff = Chr(10) Then Exit Do              ' exit on line feed
      sTemp += sBuff
    Loop

  Loop Until sTemp <> ""

  GetCommand = sTemp

'Print #99,"##";stemp;"##"  'used for debug file 3514
  'CloseHandle(inputhandle)
End Function


Function SendCommand(sWBCmd As String) As Integer

'Print #99,"$$";swbcmd;"$$" 'used for debug file 3514
  
  Dim As UInteger Ptr OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE)

  'This function sends commands to Winboard

  Dim As UInteger iBytesWritten      ' ### changed to uinteger
  Dim As Integer iBytes
  Dim As UByte iReturnCode

  sWBCmd += Chr(10)                              ' add line feed to end of string

  iBytes = Len(sWBCmd)
  iReturnCode = WriteFile(OutputHandle, ByVal StrPtr(sWBCmd), iBytes, @iBytesWritten, NULL)

  SendCommand = 0

  'CloseHandle(outputhandle)
End Function


Sub WaitForWinboard

  ' If we get 'xboard' command from Winboard then play in Winboard mode
  ' or if we get a key stroke from a human then play in normal mode.

  Dim As String sKey
  Dim As String sWBCommand

  Do
    sKey = InKey
    Sleep 100

    sWBCommand = GetCommand
    If sWBCommand = "xboard" Then
      WinboardMode = 1
      Exit Do
    End If
  Loop Until sKey <> ""

End Sub

'------------------------------------------------------------------------------

Function open_book As Integer

  If perft = TRUE Then Return FALSE

  '**************************************************************
  '                       Opening book
  'method:
  '       routine scans board, saves current position in 'book$'
  '       and compares it to positions held in DATA statements
  '       therefore it can handle all book transpositions also.
  '       Choice of book move is determined by a random number
  '       (RD) which is compared to % value read from DATA which
  '       is held in variable (PC)
  '
  '       Book routine code, with minor adaptation was initially
  '       used by Bill Rogers in his program Warlord X1b and is
  '       used here with his permission.
  '**************************************************************
  Dim As Integer x, y, i
  Dim As Integer n
  Dim As String n_str, book_str = "*", d_str     ',  d12_str

  Dim As UInteger x1, x2                            ', x1a, x1b, x2a, x2b
  'Dim As String x1_str, x2_str, x1a_str, x1b_str, x2a_str, x2b_str
  Dim As String CompMoveFrom_str, CompMoveTo_str
  Dim As Integer rd

  'BookHit = FALSE

  If side = white Then
    MoveNo += 1
    Restore bookdata
  Else
    Restore bookdata_black
  End If
  If winboardmode = 0 And cmpclr = 1 And MoveNo = 0 Then MoveNo += 1

  'Bk1:
  'Book_str = "*"
  'N = 0

  For X = 90 To 20 Step -10
    For Y = X+1 To X+8
      I = B(Y)
      'If B(Y) = 0 Then
      If I = 0 Then
        N += 1 : Continue For                    ' GoTo Bk4
      End If
      If N <> 0 Then                             '  GoTo Bk2
        'n_str = Str(N)
        'Book_str += Right(n_str,Len(n_str))
        Book_str += Str(N)
        N = 0
        'Bk2:
      End If
      'I = B(Y)

      If I < 0 Then Book_str += B_str(I + 20) Else Book_str += B_str(I)

      ' Bk4:   ### FreeBasic doesn't like this
      '    NEXT X
      '  NEXT Y

    Next y
  Next x

  'n_str = Str(MoveNo)
  'Book_str +="*" + Right(n_str, Len(n_str))
  Book_str += "*" + Str(MoveNo)

  ' Bk5:
  Do
    Read d_str
    ' return bookhit :GoTo Bk8
    If d_str = "END" Then Exit Do

    ' d12_str = Left(d_str, 1)
    ' If d12_str <> "*" Then GoTo Bk5
    ' If Book_str <> d_str Then GoTo Bk5
  Loop While Book_str <> d_str

  If d_str = "END" Then    ' if nothing is found correct moveno
    bookhit = FALSE
    GoTo bk8
  End If
  'TP = 0
  Randomize Timer
  RD = Rnd * 100
  BookHit = TRUE

  ' Bk6:

  Read d_str, n_str
  ' d12_str = Left(n_str, 1)
  ' If d12_str = "*" then Exit Do  'GoTo Bk7
  ' If side = white And n_str = "WHITE_END" Or side = black And n_str = "END" Then GoTo Bk7
  If Left(n_str, 1) <> "*" And n_str <> "END" Then
    ' PC = Val(n_str)
    ' TP += PC
    ' If RD > TP GoTo Bk6
    If RD > Val(n_str) Then Read d_str           ' ### need to change this if there are more than 2 moves per situation
  End If
  'Bk7:

  '*** convert book move to correct co-ordinate format to enable move ***

  x1 = Val(Left(d_str, 2))
  x2 = Val(Right(d_str, 2))
  CompMoveFrom_str = Chr(d_str[1] + 48) + Chr(d_str[0] - 1)
  CompMoveTo_str = Chr(d_str[3] + 48) + Chr(d_str[2] - 1)

  If Abs(B(X2)) = 6 Then Mat_left = Mat_left - Queen_Value
  If Abs(B(X2)) = 4 Then Mat_left = Mat_left - Rook_Value
  If Abs(B(X2)) = 3 Then Mat_left = Mat_left - Bishop_Value
  If Abs(B(X2)) = 2 Then Mat_left = Mat_left - Knight_Value

  B(X2) = B(X1)                                  ' make book move
  B(X1) = 0

  conv_clock_move_no += 1

  If X1 = 95 And X2 = 97 Then                    'handle rook move when castling from book
    B(98) = 0
    B(96) = -4
    b_cas(1) = -1                                 'BlackBookCastle = 1  ' set castle flag
    b_cas(2) = -1                               ' if castled O-O then can't later castle O-O-O! 'added 16/1/2010 - Leo castle bug
  End If

  If X1 = 25 And X2 = 27 Then                    'handle rook move when castling from book
    B(28) = 0
    B(26) = 4
    w_cas(1) = -1
    w_cas(2) = -1                               ' if castled O-O then can't later castle O-O-O! 'added 16/1/2010 - Leo castle bug
  End If

  If (winboardmode = 1 And side = black) Or (winboardmode = 0 And cmpclr = -1) Then
    move_hist(MoveNo, 3) = X1                    'put moves into movelist
    move_hist(MoveNo, 4) = X2
  Else
    If (winboardmode = 1 And Side = White) Or (winboardmode = 0 And cmpclr = 1) Then
      move_hist(MoveNo, 1) = X1
      move_hist(MoveNo, 2) = X2
    End If
  End If

  If WinboardMode = 1 Then                       'make book move if in winboard mode
    If Post = 1 Then Print 0; " "; 0; " "; 0; " "; 0; " "; "<book move>: "; CompMoveFrom_str + CompMoveTo_str
    SendCommand( "move " + LCase(CompMoveFrom_str + CompMoveTo_str))
  End If

  Bk8:

  'subtract moveno if no move has been played
  If Side = White And BookHit = FALSE Then MoveNo -= 1

  Return BookHit

End Function

'-------------------------------------------------------------------------------

BookData:

'****************************************************************
'                          BookData
' Data format:
' Board position/move no/book move/% choice/alternate book move
' note:    1) numbers relate to book lines below
'          2) additional book lines can be added with further
'             'Data' statements
'****************************************************************

'White book moves

Data "*rnbqkbnrpppppppp32PPPPPPPPRNBQKBNR*1", "3454", "50", "3555" '1
Data "*rnbqkbnrppp1pppp11p7P12PPP1PPPPRNBQKBNR*2", "3353", "50", "2746" '2
Data "*rnbqkbnrppp2ppp4p6p6PP12PP2PPPPRNBQKBNR*3", "2243", "50", "2746" '3
Data "*rnbqkb1rppp2ppp4pn5p6PP6N5PP2PPPPR1BQKBNR*4", "2367", "75", "5364" '4
Data "*rnbqk2rppp1bppp4pn5p2B3PP6N5PP2PPPPR2QKBNR*5", "3545", "75", "2746" '5
Data "*rnbq1rk1ppp1bppp4pn5p2B3PP6N2N2PP2PPPPR2QKB1R*6", "2433"  ' 5_1
Data "*r1bq1rk1pppnbppp4pn5p2B3PP6N2N2PPQ1PPPPR3KB1R*7", "3545", "50", "5364" '5_2
Data "*r1bq1rk1pppnbppp5n5p2B4P6N2N2PPQ1PPPPR3KB1R*8", "3545" ' 5_3
Data "*rnbq1rk1ppp1bpp5pn1p3p2B3PP6N2N2PPQ1PPPPR3KB1R*7", "6758" ' 5_4

Data "*rnbq1rk1ppp1bppp4pn5p2B3PP6N1P3PP3PPPR2QKBNR*6", "2746" '5a
Data "*rnbq1rk1ppp1bpp5pn1p3p2B3PP6N1PN2PP3PPPR2QKB1R*7", "6758", "75", "6776" '5b
Data "*rnbq1rk1p1p1bpp2p2pn1p3p6PP3B2N1PN2PP3PPPR2QKB1R*8", "2644" '5c
Data "*r1bqkb1rpppn1ppp4pn5p2B3PP6N5PP2PPPPR2QKBNR*5", "5364", "50", "3545"'6
Data "*r1bqkb1rpppn1ppp5n5p2B4P6N5PP2PPPPR2QKBNR*6", "3545" '6a
Data "*r1b1kb1rpp1n1ppp2p1pn2q2p2B3PP6N1PN2PP3PPPR2QKB1R*7", "5364", "75", "4634" '6a1
Data "*r1bqkb1rpp1n1ppp2p1pn5p2B3PP6N1P3PP3PPPR2QKBNR*6", "2746" '6b
Data "*r1bqk2rpppnbppp4pn5p2B3PP6N1P3PP3PPPR2QKBNR*6", "2746" '6c
Data "*r1bq1rk1pppnbppp4pn5p2B3PP6N1PN2PP3PPPR2QKB1R*7", "2644", "50", "5364" '6d
Data "*r1bqk2rpppn1ppp4pn5p2B2bPP6N1P3PP3PPPR2QKBNR*6", "5364" '6e
Data "*r1bqk2rpppn1ppp5n5p2B2b1P6N1P3PP3PPPR2QKBNR*7", "2644" '6f
Data "*rnbqkbnrpp3ppp2p1p6p6PP6N5PP2PPPPR1BQKBNR*4", "3555" '7
Data "*rnbqkbnrpp3ppp2p1p13PPp5N5PP3PPPR1BQKBNR*5", "4355" '7a
Data "*rnbqk1nrpp3ppp2p1p6p5bPPP5N5PP3PPPR1BQKBNR*5", "5564" '7b
Data "*rnbqk1nrpp3ppp2p8p5bPP6N5PP3PPPR1BQKBNR*6", "2746" '7c
Data "*rnbqkb1rppp1pppp5n5p7P9N2PPP1PPPPRNBQKB1R*3", "3353", "50", "2367" '11
Data "*rnbqkbnrppp2ppp4p6p7P9N2PPP1PPPPRNBQKB1R*3", "3353" '11a
Data "*rnbqkb1rppp2ppp4pn5p2B4P9N2PPP1PPPPRN1QKB1R*4", "3545" '11b
Data "*rn1qkb1rppp1pppp5n5p2B4P2b6N2PPP1PPPPRN1QKB1R*4", "3545" '11c
Data "*rnbqkb1rppp1pppp11p2B4Pn8N2PPP1PPPPRN1QKB1R*4", "6758" '11d
Data "*rnbqkb1rpp2pppp10pp7Pn2B5N2PPP1PPPPRN1QKB1R*5", "5463" '11d1
Data "*r1bqkb1rpp2pppp2n7Pp8n2B5N2PPP1PPPPRN1QKB1R*6", "2234" '11d2
Data "*r1bqkb1rpp2pppp2n7np11B5N2PPPNPPPPR2QKB1R*7", "3545" '11d3
Data "*rnbqkb1rpp2pppp2p8p7Pn2B5N2PPP1PPPPRN1QKB1R*5", "3343" '11d7
Data "*rnb1kb1rpp2pppp1qp8p7Pn2B2P2N2PP2PPPPRN1QKB1R*6", "2442" '11d8

Data "*rnbqkbnrpp2pppp2p8p6PP12PP2PPPPRNBQKBNR*3", "2746" '13a
Data "*rnbqkb1rpp2pppp2p2n5p6PP9N2PP2PPPPRNBQKB1R*4", "2243" '13b
Data "*rnbqkb1rpp3ppp2p1pn5p6PP6N2N2PP2PPPPR1BQKB1R*5", "2367" '13c
Data "*rnbqkb1rpp3ppp2p1pn8B3pP6N2N2PP2PPPPR2QKB1R*6", "3555" '13d
Data "*rnbqkb1rpp2pppp2p2n12pP6N2N2PP2PPPPR1BQKB1R*5", "3151" '13p
Data "*rn1qkb1rpp2pppp2p2n7b2P1pP6N2N3P2PPPPR1BQKB1R*6", "4665", "50", "4658" '13q

Data "*rnbqkbnrppppp1pp13p5P12PPP1PPPPRNBQKBNR*2","2746" '17
Data "*rnbqkb1rppppp1pp5n7p5P9N2PPP1PPPPRNBQKB1R*3", "2367" '17a
Data "*rnbqkb1rpppp2pp4pn7pB4P9N2PPP1PPPPRN1QKB1R*4", "2234" '17b
Data "*rnbqk2rppppb1pp4pn7pB4P9N2PPPNPPPPR2QKB1R*5", "6776", "75","3343" '17c
Data "*rnbqkb1rppp3pp4pn5p1pB4P9N2PPPNPPPPR2QKB1R*5", "3545", "50", "3353" '17d
Data "*rnbqkb1rppppp1pp13pB4Pn8N2PPP1PPPPRN1QKB1R*4", "6758", "50", "6756" '17e

Data "*rnbqkbnrpp1ppppp10p8P12PPP1PPPPRNBQKBNR*2", "5464" 'b1
Data "*rnbqkbnrpp1p1ppp10pPp19PPP1PPPPRNBQKBNR*3", "3555", "50", "2243" 'b1a
Data "*rnbqkbnrpp3ppp3p6pPp7P11PPP2PPPRNBQKBNR*4", "2243", "50", "2662" 'b1b
Data "*rnbqkbnrpp3ppp3p6pPp13N5PPP1PPPPR1BQKBNR*4", "3555" 'b2
Data "*rnbqkb1rpp1ppppp5n4pP20PPP1PPPPRNBQKBNR*3", "2243" 'b3
Data "*rnbqkb1rpp2pppp3p1n4pP14N5PPP1PPPPR1BQKBNR*4", "3555" 'b3a

Data "*rnbqkbnrpppp1ppp4p14P12PPP1PPPPRNBQKBNR*2", "3353", "50", "3555" '21
Data "*rnbqkbnrppp1pppp3p15P12PPP1PPPPRNBQKBNR*2", "3555" '22
Data "*rnbqkb1rppp1pppp3p1n13PP11PPP2PPPRNBQKBNR*3", "2243" '22a
Data "*rnbqkb1rppp1pp1p3p1np12PP5N5PPP2PPPR1BQKBNR*4", "2345", "50", "2367" '22b
Data "*rnbqkb1rppp2ppp3p1n6p6PP5N5PPP2PPPR1BQKBNR*4", "2746" '22c

Data "*rnbqkbnrpppppp1p6p12P12PPP1PPPPRNBQKBNR*2", "3555" '23
Data "*rnbqkbnrppp1pp1p3p2p12PP11PPP2PPPRNBQKBNR*3", "2243" '23a
Data "*rnbqk1nrppp1ppbp3p2p12PP5N5PPP2PPPR1BQKBNR*4","2345", "50", "3656"'23b

Data "*rnbqkbnrppp1pppp18pP12PP2PPPPRNBQKBNR*3", "2746" '25
Data "*rnbqkb1rppp1pppp5n12pP9N2PP2PPPPRNBQKB1R*4", "3545" '25a
Data "*rnbqkb1rppp2ppp4pn12pP8PN2PP3PPPRNBQKB1R*5", "2653" '25b
Data "*rnbqkbnr1pp1ppppp17pP9N2PP2PPPPRNBQKB1R*4", "3555" '25c
Data "*rnbqkbnr2p1ppppp8p8pPP8N2PP3PPPRNBQKB1R*5", "3151" '25d
Data "*rn1qkbnr1bp1ppppp8p6P1pPP8N3P3PPPRNBQKB1R*6", "5162" '25e
Data "*rn1qkbnr1bp1pppp9p8pPP8N3P3PPPRNBQKB1R*7", "2191" '25f
Data "*bn1qkbnr2p1pppp9p8pPP8N3P3PPP1NBQKB1R*8", "2243" '25g
Data "*rnbqkbnrpp2pppp10p7pP9N2PP2PPPPRNBQKB1R*4", "5464" '25h
Data "*rnbqkbnrpp3ppp4p5pP6p10N2PP2PPPPRNBQKB1R*5","2243" '25i
Data "*rnbqkbnrpp3ppp10pp6p7N2N2PP2PPPPR1BQKB1R*6", "2464" '25j
Data "*rnb1kbnrpp3ppp10pq6p7N2N2PP2PPPPR1B1KB1R*7", "4364" '25k
Data "*rnbqkbnrppp2ppp11pp5PP12PP2PPPPRNBQKBNR*3", "5465" '26
Data "*rnbqkbnrppp2ppp12P5Pp12PP2PPPPRNBQKBNR*4", "2746" '26a
Data "*r1bqkbnrppp2ppp2n9P5Pp9N2PP2PPPPRNBQKB1R*5", "3141", "50", "2234" '26b
Data "*rnbqkbnrpp3ppp10p1P5Pp9N2PP2PPPPRNBQKB1R*5", "3545" '26c
Data "*r1bqkbnrpp3ppp2n7p1P5Pp8PN2PP3PPPRNBQKB1R*6", "4554" '26d

Data "*rnbqkb1rpppppppp5n13P12PPP1PPPPRNBQKBNR*2", "3353", "50", "2367" '31

Data "*rnbqkb1rpppppppp14B4Pn11PPP1PPPPRN1QKBNR*3", "6756", "75", "6758" '33
Data "*rnbqkb1rpp1ppppp10p8PnB10PPP1PPPPRN1QKBNR*4", "3545" '34
Data "*rnb1kb1rpp1ppppp1q8pP8nB10PPP1PPPPRN1QKBNR*5", "2234" '34a
Data "*rnb1kb1rpp1ppppp1q8pP9B10PPPnPPPPR2QKBNR*6", "5624" '34b
Data "*rnb1kb1rpp1ppppp10pP20PqPBPPPPR2QKBNR*7", "3555" '34c
Data "*rnb1kb1rpp1ppp1p6p3pP8P11PqPB1PPPR2QKBNR*8", "2122" '34c1
Data "*rnb1kb1rpp1ppppp10pPq7P11P1PB1PPPR2QKBNR*8", "2644" '34c2
Data "*rnbqkb1rpp2pppp3p6pP8nB10PPP1PPPPRN1QKBNR*5", "3646" '34d
Data "*rnbqkb1rpp2pppp3p1n4pP9B7P2PPP1P1PPRN1QKBNR*6", "3555" '34e
Data "*rnb1kb1rpp2pppp3p4q1pP8nB7P2PPP1P1PPRN1QKBNR*6", "2234" '34f
Data "*rnbqkb1rppp1pppp11p7PnB10PPP1PPPPRN1QKBNR*4", "3545" '35
Data "*rnbqkb1rpp2pppp10pp7PnB6P3PPP2PPPRN1QKBNR*5", "2644" '35a
Data "*rnbqkb1rpp2pppp5n4pp7P1B5BP3PPP2PPPRN1QK1NR*6", "3343" '35b
Data "*rnbqkb1rppp1pppp3p15PnB10PPP1PPPPRN1QKBNR*4", "2234" '36
Data "*rnbqkb1rppp1pppp3p1n13P1B10PPPNPPPPR2QKBNR*5", "3555", "50", "3545" '36a
Data "*rnbqkb1rpppp1ppp4p14PnB10PPP1PPPPRN1QKBNR*4", "2234", "50", "3646" '37
Data "*rnbqkb1rpppp1ppp4pn13P1B7P2PPP1P1PPRN1QKBNR*5", "3555" '37a
Data "*rnbqkb1rpppp1ppp4pn8B4P12PPP1PPPPRN1QKBNR*3", "3555" '38
Data "*rnbqkb1rpppp1pp5pn1p6B4PP11PPP2PPPRN1QKBNR*4", "6776" '38a

Data "*rnb1kb1rpppp1pp5pq1p11PP11PPP2PPPRN1QKBNR*5", "2746" '38b
Data "*rnb1kb1rppp2pp5pq1p3p7PP8N2PPP2PPPRN1QKB1R*6", "2234", "50", "5565" '38b1
Data "*rnb1kb1rppp2pp4ppq1p11PP8N2PPP2PPPRN1QKB1R*6", "2243" '38c
Data "*rnbqk2rppppbppp4pn8B4PP11PPP2PPPRN1QKBNR*4", "2644" '38d
Data "*rnbqk2rppp1bppp4pn5p2B4PP6B4PPP2PPPRN1QK1NR*5", "5565" '38e
Data "*rnbqk2rppp1bppp4p6pP1B4Pn6B4PPP2PPPRN1QK1NR*6", "6785" '38e1
Data "*rnb1k2rppp1qppp4p6pP6Pn6B4PPP2PPPRN1QK1NR*7", "3343" ' 38e2
Data "*rnbqk2rpppnbppp4p6pP1B4P7B4PPP2PPPRN1QK1NR*6", "6785" '38e4
Data "*rnb1k2rpppnqppp4p6pP6P7B4PPP2PPPRN1QK1NR*7", "3343", "50", "2243" ' 38e5

Data "*rnbqk2rpp1pbppp4pn4p3B4PP6B4PPP2PPPRN1QK1NR*5", "5463", "50", "3343" '38f
Data "*rnbqkb1rpp1p1ppp4pn4p3B4PP11PPP2PPPRN1QKBNR*4", "5464" '38g
Data "*rnbqkb1rpp3ppp3ppn4pP2B5P11PPP2PPPRN1QKBNR*5", "2243" '38h
Data "*rnbqk2rpp2bppp3ppn4pP2B5P5N5PPP2PPPR2QKBNR*6", "2662", "50", "2746" '38i
Data "*rnbqkb1rppp1pppp5n5p2B4P12PPP1PPPPRN1QKBNR*3", "6776" '39
Data "*rnbqkb1rppp2ppp5p5p7P12PPP1PPPPRN1QKBNR*4", "3545" '39a
Data "*rnbqk2rppp2ppp3b1p5p7P8P3PPP2PPPRN1QKBNR*5", "3353" '39b
Data "*rnbq1rk1ppp2ppp3b1p12BP8P3PP3PPPRN1QK1NR*7", "2746", "75", "2243" '39c
Data "*rnbqkb1rpp3ppp2p2p5p7P8P3PPP2PPPRN1QKBNR *5", "2644" '39d
Data "*rnbqk2rpp3ppp2pb1p5p7P7BP3PPP2PPPRN1QK1NR*6", "2735", "75", "2446" '39e
Data "*rnbqkb1rppp1pp1p5p5p7P12PPP1PPPPRN1QKBNR*4", "3545", "50", "2746" '39f
Data "*rnbqkb1rpp2pp1p5p4pp7P8P3PPP2PPPRN1QKBNR*5", "2746", "50", "2662" '39g
Data "*rnbqkb1rpp1ppppp5n4p3B4P12PPP1PPPPRN1QKBNR*3", "6776", "50", "5464" '39h
Data "*rnbqkb1rpp1ppp1p5p4p8P12PPP1PPPPRN1QKBNR*4", "5464" '39i
Data "*rnb1kb1rpp1ppppp1q3n4pP2B17PPP1PPPPRN1QKBNR*4", "6776" '39j
Data "*rnb1kb1rpp1ppp1p1q3p4pP20PPP1PPPPRN1QKBNR*5", "2423" '39k
Data "*rnbqkb1rpppp1ppp4pn12PP12PP2PPPPRNBQKBNR*3", "2243", "75", "2746" '41
Data "*rnbqkb1rppp2ppp4pn5p6PP9N2PP2PPPPRNBQKB1R*4", "2243", "50", "2367" '42
Data "*rnbqkbnrpp3ppp2p1p6p6PP9N2PP2PPPPRNBQKB1R*4", "2433"  '43a
Data "*rnbqkb1rpp3ppp2p1pn5p6PP9N2PPQ1PPPPRNB1KB1R*5", "2367" '43b
Data "*rnbqkbnrpp3ppp4p5pp6PP9N2PP2PPPPRNBQKB1R*4", "5364" '43c
Data "*rnbqkbnrpp3ppp10pp7P9N2PP2PPPPRNBQKB1R*5", "2243", "50", "2367" '43d
Data "*rnbqk2rpppp1ppp4pn11bPP6N5PP2PPPPR1BQKBNR*4", "2442" , "50", "2433" '50
Data "*rnbqk2rpp1p1ppp4pn4p6bPP5QN5PP2PPPPR1B1KBNR*5","5463", "80", "2746" '53
Data "*r1bqk2rpp1p1ppp2n1pn4P6bP6QN5PP2PPPPR1B1KBNR*6", "2746", "50", "2367" '55
Data "*rnbq1rk1pppp1ppp4pn11bPP6N5PPQ1PPPPR1B1KBNR*5", "2746" '60
Data "*rnbqk2rpp1p1ppp4pn4p6bPP6N5PPQ1PPPPR1B1KBNR*5", "5463" '63
Data "*rnbq1rk1pp1p1ppp4pn4P6bP7N5PPQ1PPPPR1B1KBNR*6","3141", "75", "2746" '64
Data "*rnbqkb1rpppp1ppp5n6p5PP12PP2PPPPRNBQKBNR*3", "5465" '66
Data "*rnbqkb1rpppp1ppp12P5P3n9PP2PPPPRNBQKBNR*4", "2356", "50", "2746" '66a
Data "*r1bqkb1rpppp1ppp2n9P5P2Bn9PP2PPPPRN1QKBNR*5", "2746" '66b
Data "*r1bqk2rpppp1ppp2n9P4bP2Bn6N2PP2PPPPRN1QKB1R*6", "2234" '66c
Data "*r1b1k2rppppqppp2n9P4bP2Bn6N2PP1NPPPPR2QKB1R*7", "3545", "50", "3141" '66d
Data "*rnbqk2rpppp1ppp12P4bP2Bn9PP2PPPPRN1QKBNR*5", "2234" '66e
Data "*r1bqk2rpppp1ppp2n9P4bP2Bn9PP1NPPPPR2QKBNR*6", "2746" '66f
Data "*rnbqk2rpppp1ppp10b1P5P3n6N2PP2PPPPRNBQKB1R*5", "3545" '66g
Data "*r1bqk2rpppp1ppp2n7b1P5P3n5PN2PP3PPPRNBQKB1R*6", "2635" '66h
Data "*r1bqk2rpppp1ppp2n7b1n5P9PN2PP2BPPPRNBQK2R*7", "4665", "50", "2243" '66i
Data "*rnbqkb1rpppp1ppp12P5P1n11PP2PPPPRNBQKBNR*4", "2746", "50", "3141" '66j
Data "*rnbqk2rpppp1ppp12P4bP1n8N2PP2PPPPRNBQKB1R*5", "2234" '66k
Data "*r1bqk2rpppp1ppp2n9P4bP1n8N2PP1NPPPPR1BQKB1R*6", "3141" '66l
Data "*r1bqkb1rpppp1ppp2n9P5P3n6N2PP2PPPPRNBQKB1R*5", "2356" '67a
Data "*r1bqk2rpppp1ppp2n9P4bP2Bn6N2PP2PPPPRN1QKB1R*6", "2234" '67b
Data "*r1b1k2rppppqppp2n9P4bP2Bn6N2PP1NPPPPR2QKB1R*7", "3545", "50", "3141" '67c

Data "*r1bqkb1rpppp1ppp2n9P5P1n3P8P2PPPPRNBQKBNR*5", "2746" '66m
Data "*r1bqkb1rppp2ppp2np8P5P1n3P4N3P2PPPPRNBQKB1R*6", "2433" '66n
Data "*r2qkb1rppp2ppp2np8Pb4P1n3P4N3PQ1PPPPRNB1KB1R*7", "2243" '66o
Data "*rnbqkb1rpppppp1p5np11PP12PP2PPPPRNBQKBNR*3", "2243", "50", "2746" '75
Data "*rnbqk2rppppppbp5np11PP6N5PP2PPPPR1BQKBNR*4", "3555", "75", "2367" '77
Data "*rnbqk2rppp1ppbp3p1np11PPP5N5PP3PPPR1BQKBNR*5", "2746", "75", "2635" '78
Data "*rnbq1rk1ppp1ppbp3p1np11PPP5N2N2PP3PPPR1BQKB1R*6", "2635", "50", "2367" '80
Data "*rnbq1rk1ppp1ppbp3p1np7B3PP6N2N2PP2PPPPR2QKB1R*6", "3545", "85", "3555" '83
Data "*rnbq1rk1ppp1ppbp3p1np11PP1B4N2N2PP2PPPPR2QKB1R*6", "3545" '85
Data "*rnbqkb1rppp1pp1p5np4p6PP6N5PP2PPPPR1BQKBNR*4", "5364" '86
Data "*rnbqkb1rppp1pp1p6p4n7P6N5PP2PPPPR1BQKBNR*5", "2334" '86a
Data "*rnbqk2rppp1ppbp6p4n7P6N5PP1BPPPPR2QKBNR*6", "3555", "50", "2746" '86b
Data "*rnbqk2rppp1ppbp1n4p12PP5N5PP1B1PPPR2QKBNR*7", "3445" '86c
Data "*rnbqkb1rppp1pp1p1n4p12P6N5PP1BPPPPR2QKBNR*6", "3467", "50", "2746" '86d

Data "*rnbqkb1rpp1ppppp5n4p7PP12PP2PPPPRNBQKBNR*3", "5464" '88
Data "*rnbqkb1rp2ppppp5n3ppP6P13PP2PPPPRNBQKBNR*4", "2746" '88a
Data "*rnbqkb1rp2ppp1p5np2ppP6P10N2PP2PPPPRNBQKB1R*5", "2433" '88b
Data "*rnbqkb1rp2ppp1p5np3pP6p10N2PPQ1PPPPRNB1KB1R*6", "3555" '88c
Data "*rnbqk2rp2pppbp5np2ppP6P10N2PPQ1PPPPRNB1KB1R*6", "3555" '88d
Data "*rn1qkb1rpb1ppppp5n3ppP6P10N2PP2PPPPRNBQKB1R*5", "2433" '88g
Data "*rn1qkb1rpb1ppppp5n4pP6p10N2PPQ1PPPPRNB1KB1R*6", "3555" '88h
Data "*rn1qkb1rpb1p1ppp4pn4pP6p1P8N2PPQ2PPPRNB1KB1R*7", "2653" '88i
Data "*rnbqkb1rp2ppppp5n4pP6p10N2PP2PPPPRNBQKB1R*5", "2243" '88k
Data "*rnbqkb1rp3pppp3p1n4pP6p7N2N2PP2PPPPR1BQKB1R*6", "3555" '88l
Data "*rnbqkb1rp3pppp3p1n3ppP6P10N2PP2PPPPRNBQKB1R*5", "5362" '88m
Data "*rnbqkb1r4ppppp2p1n3PpP17N2PP2PPPPRNBQKB1R*6", "3545" '88n
Data "*rnbqkb1r4pp1pp2p1np2PpP16PN2PP3PPPRNBQKB1R*7", "2243" '88o
Data "*rnbqk2r4ppbpp2p1np2PpP14N1PN2PP3PPPR1BQKB1R*8", "3151", "50", "2635" '88p

Data "*rnbqkb1rpp1p1ppp4pn4pP6P13PP2PPPPRNBQKBNR*4", "2746" '88t
Data "*rnbqkb1rpp1p1ppp5n4pp6P10N2PP2PPPPRNBQKB1R*5", "5364" '88u
Data "*rnbqkb1rpp3ppp3p1n4pP17N2PP2PPPPRNBQKB1R*6", "2243", "75", "3555"  '88v
Data "*rnbqkb1rpp3p1p3p1np3pP14N2N2PP2PPPPR1BQKB1R*7", "2356", "50", "2367" '88w

Data "*rnbqkbnrpppp1ppp12p7P11PPPP1PPPRNBQKBNR*2", "2746", "75", "2653" '101
Data "*rnbqkb1rpppp1ppp5n6p5B1P11PPPP1PPPRNBQK1NR*3", "3444" '102
Data "*r1bqkb1rpppp1ppp2n2n6p5B1P6P4PPP2PPPRNBQK1NR*4", "2746", "75", "2243" '103
Data "*r1bqk2rppppbppp2n2n6p5B1P6P1N2PPP2PPPRNBQK2R*5", "2527" '104
Data "*r1bq1rk1ppppbppp2n2n6p5B1P6P1N2PPP2PPPRNBQ1RK*6", "2625", "70", "5342" '105
Data "*r1bqk2rpppp1ppp2n2n4b1p5B1P6P1N2PPP2PPPRNBQK2R*5", "2527", "75", "3343" '106
Data "*r1bqk2rpppp1ppp2n2n6p4bB1P5NP4PPP2PPPR1BQK1NR*5", "2735", "75", "2746" '107
Data "*rnbqkb1rpp1p1ppp2p2n6p5B1P6P4PPP2PPPRNBQK1NR*4", "2746" '108
Data "*rnbqkb1rpp3ppp2p2n5pp5B1P6P1N2PPP2PPPRNBQK2R*5", "5342" '109
Data "*rnbqkb1rpp3ppp2p2n6p7p4B1P1N2PPP2PPPRNBQK2R*6", "4667" '109a
Data "*rnbqk2rpp1pbppp2p2n6p5B1P6P1N2PPP2PPPRNBQK2R*5", "2527" '110

Data "*rnbqkbnrppp2ppp3p8p7P8N2PPPP1PPPRNBQKB1R*3", "3454" '110a
Data "*rnbqkbnrppp2ppp3p15pP8N2PPP2PPPRNBQKB1R*4", "4654" '110b
Data "*rnbqkb1rppp2ppp3p1n13NP11PPP2PPPRNBQKB1R*5", "2243" '110c
Data "*rnbqk2rppp1bppp3p1n13NP5N5PPP2PPPR1BQKB1R*6", "2635", "50", "2644" '110d
Data "*rnbqkb1rppp2ppp3p1n6p6PP8N2PPP2PPPRNBQKB1R*4", "5465" '110k
Data "*rnbqkb1rppp2ppp3p8P7n8N2PPP2PPPRNBQKB1R*5", "2464" '110l
Data "*rnbqkb1rppp2ppp3p6nQP16N2PPP2PPPRNB1KB1R*6", "2367" '110m

Data "*r1bqkbnrpppp1ppp2n9p7P8N2PPPP1PPPRNBQKB1R*3", "2653" '111

Data "*r1bqk1nrppppbppp2n9p5B1P8N2PPPP1PPPRNBQK2R*4", "3454" '111a
Data "*r1bqk1nrppp1bppp2np8p5BPP8N2PPP2PPPRNBQK2R*5", "5465" '111b
Data "*r1bqk1nrppp1bppp2n9p5B1P8N2PPP2PPPRNBQK2R*6", "2494" '111c
Data "*r1bbk1nrppp2ppp2n9p5B1P8N2PPP2PPPRNB1K2R*7", "2243", "50", "5362" '111d

Data "*r1bqk1nrppppbppp2n15BpP8N2PPP2PPPRNBQK2R*5", "3343", "50", "2527" '111t
Data "*r1bqk1nrppp1bppp2np14BpP5P2N2PP3PPPRNBQK2R*6", "2442", "50", "4354" '111u

Data "*r1bqkb1rpppp1ppp2n2n6p5B1P8N2PPPP1PPPRNBQK2R*4","3444", "75", "3454" ' 112
Data "*r1bqk2rppppbppp2n2n6p5B1P6P1N2PPP2PPPRNBQK2R*5", "2527"  '114
Data "*r1bqk2rppp2ppp2np1n4b1p5B1P6P1N2PPP2PPPRNBQ1RK*6", "3848" '115
Data "*r1bq1rk1ppppbppp2n2n6p5B1P6P1N2PPP2PPPRNBQ1RK*6", "3848" '116
Data "*r1bqk2rppp1bppp2np1n6p5B1P6P1N2PPP2PPPRNBQ1RK*6", "3848" '116c

Data "*r1bqk1nrpppp1ppp2n7b1p5B1P8N2PPPP1PPPRNBQK2R*4", "3343" '117
Data "*r1bqk2rpppp1ppp2n2n4b1p5B1P5P2N2PP1P1PPPRNBQK2R*5", "3444" '118
Data "*r1bqk2rppp2ppp2np1n4b1p5B1P5PP1N2PP3PPPRNBQK2R*6", "2527" '119
Data "*r1bq1rk1ppp2ppp2np1n4b1p5B1P5PP1N2PP3PPPRNBQ1RK*7", "3848", "50", "2367" '120
Data "*r1b1k1nrppppqppp2n7b1p5B1P5P2N2PP1P1PPPRNBQK2R*5", "2527" '121
Data "*r1bqk1nrppp2ppp2np6b1p5B1P5P2N2PP1P1PPPRNBQK2R*5", "2527", "50", "3444" '122
Data "*r1bqk2r1ppp1pppp1n2n4b1p5B1P5PP1N2PP3PPPRNBQK2R*6", "2527", "75", "5342" '123
Data "*r1bqk2r1pp2pppp1np1n4b1p5B1P5PP1N2PP3PPPRNBQ1RK*7", "5342" '124
Data "*r1bqk2rbpp2pppp1np1n6p7P4BPP1N2PP3PPPRNBQ1RK*8", "2625", "50", "2345" '124_1
Data "*r1bqk2rbppp1pppp1n2n6p7P4BPP1N2PP3PPPRNBQK2R*7", "2527" '124_2
Data "*r1bqk2rbppp1pppp1n2n6p5B1P5PP1N2PP3PPPRNBQ1RK*7", "5342" ' 124_3

Data "*rnbqkb1rpppp1ppp5n6p7P8N2PPPP1PPPRNBQKB1R*3", "3454" '124a
Data "*rnbqkb1rpppp1ppp12p6Pn8N2PPP2PPPRNBQKB1R*4", "2644" '124b
Data "*rnbqkb1rppp2ppp11pp6Pn6B1N2PPP2PPPRNBQK2R*5", "5465", "50", "4665" '124c
Data "*rnbqkb1rpppp1ppp5n13pP8N2PPP2PPPRNBQKB1R*4", "5565", "50", "2653" '124f
Data "*rnbqkb1rpppp1ppp12P6pn8N2PPP2PPPRNBQKB1R*5", "2454" '124g
Data "*rnbqkb1rpppp1ppp18Bpn8N2PPP2PPPRNBQK2R*5", "2454" '124h
Data "*rnbqkb1rpppp1ppp5n12BQ9N2PPP2PPPRNB1K2R*6", "2367" '124i

Data "*rnbqkbnrpp1ppppp10p9P11PPPP1PPPRNBQKBNR*2", "2746", "75", "2243" '125
Data "*rnbqkbnrpp2pppp3p6p9P8N2PPPP1PPPRNBQKB1R*3", "2662" '131
Data "*rn1qkbnrpp1bpppp3p5Bp9P8N2PPPP1PPPRNBQK2R*4", "6284" '132
Data "*rn2kbnrpp1qpppp3p6p9P8N2PPPP1PPPRNBQK2R*5", "2527" '133
Data "*rn2kb1rpp1qpppp3p1n4p9P8N2PPPP1PPPRNBQ1RK*6", "5565" '134
Data "*rn2kb1rpp1qpppp5n4p1p16N2PPPP1PPPRNBQ1RK*7", "4665" '135
Data "*r2qkbnrpp1npppp3p6p9P8N2PPPP1PPPRNBQK2R*5", "2527" '137
Data "*r2qkb1rpp1npppp3p1n4p9P8N2PPPP1PPPRNBQ1RK*6", "3444", "75", "2435" '138
Data "*r2qkb1rpp1n1ppp3ppn4p9P6P1N2PPP2PPPRNBQ1RK*7", "2243" '139
Data "*rnbqkbnrpp1p1ppp4p5p9P8N2PPPP1PPPRNBQKB1R*3", "2243" '139a
Data "*rnbqkbnr1p1p1pppp3p5p9P5N2N2PPPP1PPPR1BQKB1R*4", "3454" '139b
Data "*rnbqkbnr1p1p1pppp3p14pP5N2N2PPP2PPPR1BQKB1R*5", "4654" '139c
Data "*rnb1kbnr1pqp1pppp3p14NP5N5PPP2PPPR1BQKB1R*6", "2644" '139d
Data "*r1bqkbnrpp1p1ppp2n1p5p9P5N2N2PPPP1PPPR1BQKB1R*4", "3454", "50", "2662" '139e
Data "*r1bqkbnrpp1p1ppp2n1p14pP5N2N2PPP2PPPR1BQKB1R*5", "4654" '139f
Data "*r1b1kbnrppqp1ppp2n1p14NP5N5PPP2PPPR1BQKB1R*6", "2635" '139g
Data "*r1bqkb1rpp1pnppp2n1p4Bp9P5N2N2PPPP1PPPR1BQK2R*5", "2527" '139h
Data "*r1bqkb1r1p1pnpppp1n1p4Bp9P5N2N2PPPP1PPPR1BQ1RK*6", "6273" '139j
Data "*r1bqkbnrpp1ppppp2n7p9P8N2PPPP1PPPRNBQKB1R*3", "2243", "50", "2653" '140
Data "*r1bqkbnrpp1p1ppp2n1p5p9P5N2N2PPPP1PPPR1BQKB1R*3", "3454", "50", "2662" '145
Data "*r1bqkbnrpp2pppp2np6p9P5N2N2PPPP1PPPR1BQKB1R*3", "3454", "50", "2662" '150
Data "*r2qkb1rpp1bpppp2np1n3Bp9P5N2N2PPPP1PPPR1BQ1RK*5", "3454", "50", "3444" '156
Data "*r1bqkbnrpp1ppppp2n7p9P5N5PPPP1PPPR1BQKBNR*3", "2662", "50", "2746" '180
Data "*rnbqkbnrpp2pppp3p6p9P5N5PPPP1PPPR1BQKBNR*3", "2746" '181
Data "*rnbqkb1rpp2pppp3p1n4p9P5N2N2PPPP1PPPR1BQKB1R*4", "3454" '181a
Data "*rnbqkb1rpp2pppp3p1n13pP5N2N2PPP2PPPR1BQKB1R*5", "2454" '181b
Data "*r1bqkb1rpp2pppp2np1n13QP5N2N2PPP2PPPR1B1KB1R*6", "2662" '181c
Data "*rnbqkbnr1p2ppppp2p6p9P5N2N2PPPP1PPPR1BQKB1R*4", "3454" '182a
Data "*rnbqkbnr1p2ppppp2p15pP5N2N2PPP2PPPR1BQKB1R*5", "4654" '182b
Data "*rnbqkbnrpp1ppppp2p17P11PPPP1PPPRNBQKBNR*2", "2243", "50", "3353" '200
Data "*rnbqkbnrpp2pppp2p8p8P5N5PPPP1PPPR1BQKBNR*3", "2746", "50", "3454" '220
Data "*rn1qkbnrpp2pppp2p8p8P1b3N2N2PPPP1PPPR1BQKB1R*4", "3848", "50", "2635" '222
Data "*rn1qkbnrpp3ppp2p1p6p8P5N2Q1PPPPP1PP1R1B1KB1R*6", "3454", "50", "2635" '223
Data "*rn1qkbnrpp3ppp2p1p6p8P1b3N2N2PPPPBPPPR1BQK2R*5", "2527", "75", "3747" '224
Data "*rn1qkb1rpp3ppp2p1pn5p8P1b3N2N2PPPPBPPPR1BQ1RK*6", "4665", "75", "3747" '225
Data "*rnbqkbnrpp2pppp2p16Pp5N5PPP2PPPR1BQKBNR*4", "4355" '227
Data "*rn1qkbnrpp2pppp2p10b5PN11PPP2PPPR1BQKBNR*5", "5547", "50", "5563" '228
Data "*rnbqkbnrpp2pppp2p8p6P1P11PP1P1PPPRNBQKBNR*3", "5364", "50", "5564"  '230
Data "*rnbqkbnrpp2pppp11p6P13PP1P1PPPRNBQKBNR*4", "5364" '231
Data "*rnbqkbnrpp2pppp11p8P11PP1P1PPPRNBQKBNR*4", "5564" '232
Data "*rnbqkb1rpp2pppp5n5P20PP1P1PPPRNBQKBNR*5", "2243", "50", "2662" '234
Data "*rnbqkb1rpp2pppp11n14N5PP1P1PPPR1BQKBNR*6", "2653", "50", "2746" '236
Data "*r1bqkb1rpp2pppp2n8n14N2N2PP1P1PPPR1BQKB1R*7", "3454", "50", "2662" '238
Data "*r1bqkb1rpp1npppp5n3B1P20PP1P1PPPRNBQK1NR*6", "2243", "75", "2746" ' 245
Data "*rnbqkb1rpppppppp5n14P11PPPP1PPPRNBQKBNR*2", "5565" '300
Data "*rnbqkb1rpppppppp11nP19PPPP1PPPRNBQKBNR*3", "3454", "75", "2746" '301
Data "*rnbqkb1rppp1pppp3p7nP16N2PPPP1PPPRNBQKB1R*4", "3454"'303
Data "*rnbqkb1rppp1pppp3p7nP6P12PPP2PPPRNBQKBNR*4", "2746", "75", "3353" '305
Data "*rn1qkb1rppp1pppp3p7nP6P2b6N2PPP2PPPRNBQKB1R*5", "2635" '307
Data "*rnbqkbnrpppp1ppp4p15P11PPPP1PPPRNBQKBNR*2", "3454" '320
Data "*rnbqkbnrppp2ppp4p6p7PP11PPP2PPPRNBQKBNR*3", "2243", "75", "5565" '322
Data "*rnbqkb1rppp2ppp4pn5p7PP5N5PPP2PPPR1BQKBNR*4", "2367" '322a
Data "*rnbqk2rppp1bppp4pn5p2B4PP5N5PPP2PPPR2QKBNR*5", "5565" '322b
Data "*rnbqk2rpppnbppp4p6pP1B4P6N5PPP2PPPR2QKBNR*6", "6785", "50", "3858" '322c
Data "*rnb1k2rpppnqppp4p6pP6P6N5PPP2PPPR2QKBNR*7", "4362", "50", "3656" '322d

Data "*rnbqk1nrppp2ppp4p6p5b1PP5N5PPP2PPPR1BQKBNR*4", "5564", "50", "5565" '325
Data "*rnbqk1nrppp2ppp11p5b1P6N5PPP2PPPR1BQKBNR*5", "2644" '330
Data "*r1bqk1nrppp2ppp2n8p5b1P6NB4PPP2PPPR1BQK1NR*6", "3141", "50", "2735" '331
Data "*rnbqk1nrpp3ppp4p5ppP4b1P6N5PPP2PPPR1BQKBNR*5", "3141" '335
Data "*rnbqk1nrpp3ppp4p3b1ppP6P4P1N6PP2PPPR1BQKBNR*6", "3252" '337
Data "*rnbqk1nrpp3ppp4p3b2pP4P1p4P1N7P2PPPR1BQKBNR*7", "4362" '338
Data "*r1bqkbnrppp2ppp2n1p6p7PP5N5PPP2PPPR1BQKBNR*4", "5565" '339a
Data "*r1bqkbnrppp3pp2n1pp5pP6P6N5PPP2PPPR1BQKBNR*5", "2662" '339b
Data "*rnbqkbnrpp3ppp4p5ppP6P12PPP2PPPRNBQKBNR*4", "3343" '340
Data "*r1bqkbnrpp3ppp2n1p5ppP6P6P5PP3PPPRNBQKBNR*5", "2746" '340a
Data "*r1b1kbnrpp3ppp1qn1p5ppP6P6P2N2PP3PPPRNBQKB1R*6", "3141" '340b
Data "*r2qkbnrpp1b1ppp2n1p5ppP6P6P2N2PP3PPPRNBQKB1R*6", "3141", "50", "2635" '340c

Data "*r1bqkbnrpppppppp2n17P11PPPP1PPPRNBQKBNR*2", "2746" '350
Data "*r1bqkbnrppp1pppp2np16P8N2PPPP1PPPRNBQKB1R*3", "3454" '351
Data "*r1bqkb1rppp1pppp2np1n13PP8N2PPP2PPPRNBQKB1R*4", "2243", "75", "5464" '353
Data "*r2qkb1rppp1pppp2np1n13PP1b3N2N2PPP2PPPR1BQKB1R*5", "2345", "75", "5464" '354
Data "*rnbqkbnrppp1pppp11p8P11PPPP1PPPRNBQKBNR*2", "5564", "85", "2243" '360
Data "*rnb1kbnrppp1pppp11q20PPPP1PPPRNBQKBNR*3", "2243", "80", "2746" '362
Data "*rnb1kbnrppp1pppp8q17N5PPPP1PPPR1BQKBNR*4", "3454", "75", "2746" '364
Data "*rnb1kb1rppp1pppp5n2q10P6N5PPP2PPPR1BQKBNR*5", "2746", "75", "2635" '366
Data "*rnb1kb1rppp1pppp5n2q17N2N2PPPP1PPPR1BQKB1R*5", "3454", "75","2635" '368
Data "*rnbqkbnrppp1pppp19pP5N5PPPP1PPPR1BQKBNR*3", "4335" '370
Data "*rnbqkbnrppp2ppp12p6pP11PPPPNPPPR1BQKBNR*4", "3547" '371
Data "*rnbqkbnrp1pppppp1p18P11PPPP1PPPRNBQKBNR*2", "3454" '390
Data "*rn1qkbnrpbpppppp1p17PP11PPP2PPPRNBQKBNR*3", "2644" '391
Data "*rn1qkbnrpbpp1ppp1p2p14PP6B4PPP2PPPRNBQK1NR*4", "2746", "50", "2735" '392
Data "*rn1qkbnrpb1p1ppp1p2p5p8PP6B1N2PPP2PPPRNBQK2R*5", "3343" '393
Data "*rn1qkb1rpb1p1ppp1p2pn4p8PP5PB1N2PP3PPPRNBQK2R*6", "2435" '394
Data "*rnbqkbnrpppppp1p6p13P11PPPP1PPPRNBQKBNR*2", "3454" '395
Data "*rnbqk1nrppppppbp6p12PP11PPP2PPPRNBQKBNR*3", "2243", "75", "2746" '396
Data "*rnbqk1nrppp1ppbp3p2p12PP5N5PPP2PPPR1BQKBNR*4", "2345", "50", "2746" '397
Data "*rnbqk2rppp1ppbp3p1np12PP5N2N2PPP2PPPR1BQKB1R*5", "2635" '397a
Data "*rnbq1rk1ppp1ppbp3p1np12PP5N2N2PPP1BPPPR1BQK2R*6", "2527", "50", "2367" '397b
Data "*rnbqk1nrppp1ppbp3p2p12PP8N2PPP2PPPRNBQKB1R*4", "2243" '397c

Data "END"

bookdata_black:
'Black book moves

Data "*rnbqkbnrpppppppp19P12PPP1PPPPRNBQKBNR*1","9776","50","8464"  ' 1
Data "*rnbqkb1rpppppppp5n12PP12PP2PPPPRNBQKBNR*2","8575","50","8565" ' 2
Data "*rnbqkbnrppp1pppp11p7PP11PPP2PPPRNBQKBNR*2", "6455" '290a
Data "*rnbqkbnrppp1pppp19Pp5N5PPP2PPPR1BQKBNR*3", "8565" '290b
Data "*rnbqkbnrppp2ppp12p6PN11PPP2PPPR1BQKBNR*4", "6554" '290c
Data "*rnbqkbnrppp2ppp12p2Q3Pp5N5PPP2PPPR1B1KBNR*4", "9273" '290d
Data "*rnbqkb1rppp1pppp5n5p7P8PN2PPP2PPPRNBQKB1R*3", "8363" '293a
Data "*rnbqkb1rpp2pppp5n4pp7P6P1PN2PP3PPPRNBQKB1R*4", "9273", "50", "9483" '293b

Data "*rnbqkbnrppp1pppp11p6PP12PP2PPPPRNBQKBNR*2","8575","75","6453" '3
Data "*rnbqkb1rpppp1ppp5n6P5P13PP2PPPPRNBQKBNR*3","7657"   ' 4
Data "*rnbqkb1rpppp1ppp12P5P1P1n9PP3PPPRNBQKBNR*4", "5765" '4a
Data "*rnbqkb1rpppp1ppp12n5P1PP10PP4PPRNBQKBNR*5", "6573" '4b
Data "*rnbqkb1rpppp1ppp5n5Pp5P13PP2PPPPRNBQKBNR*3", "9663"  '2a
Data "*rnbqk2rpppp1ppp5n4bPp5P7N5PP2PPPPR1BQKBNR*4", "8474" '2b
Data "*rnbqkbnrppp2ppp4p6p6PP6N5PP2PPPPR1BQKBNR*3","9776", "75", "8373"  ' 5
Data "*rnbqkbnrpp3ppp2p1p6p6PP6N1P3PP3PPPR1BQKBNR*4", "9776" '5a
Data "*rnbqkb1rpp3ppp2p1pn5p6PP6N1PN2PP3PPPR1BQKB1R*5", "9284" '5b
Data "*r1bqkb1rpp1n1ppp2p1pn5p6PP6NBPN2PP3PPPR1BQK2R*6", "6453" '5c
Data "*r1bqkb1rpp1n1ppp2p1pn12BP6N1PN2PP3PPPR1BQK2R*7", "8262" '5d
Data "*r1bqkb1rpp1n1ppp2p1pn5p6PP6N1PN2PPQ2PPPR1B1KB1R*6", "9674" '5m
Data "*r1bqk2rpp1n1ppp2pbpn5p6PP6NBPN2PPQ2PPPR1B1K2R*7", "9597" '5n
Data "*rnbqkbnrpp3ppp2p1p6p6PP6N2N2PP2PPPPR1BQKB1R*4", "6453" '5t
Data "*rnbqkbnrpp3ppp2p1p11P1pP6N2N3P2PPPPR1BQKB1R*5", "9652" '5u
Data "*rnbqk1nrpp3ppp2p1p11PbpP6N1PN3P3PPPR1BQKB1R*6", "8262" '5v
Data "*rnbqkbnrpp3ppp2p1p13pP6N1PN2PP3PPPR1BQKB1R*5", "8262" '5w
Data "*rnbqkbnrp4ppp2p1p4p6P1pP6N1PN3P3PPPR1BQKB1R*6", "6252", "50", "9652" '5x
Data "*rnbqkbnrpp3ppp2p1p13pPP5N2N2PP3PPPR1BQKB1R*5", "8262" '5z

Data "*rnbqkb1rpppp1ppp4pn12PP6N5PP2PPPPR1BQKBNR*3","9652", "50","8464" '6
Data "*rnbqk2rpppp1ppp4pn11bPP6N5PPQ1PPPPR1B1KBNR*4","9597" '7
Data "*rnbq1rk1pppp1ppp4pn11bPP4P1N6PQ1PPPPR1B1KBNR*5", "5243"  '7a
Data "*rnbq1rk1pppp1ppp4pn12PP4P1Q6P2PPPPR1B1KBNR*6", "8272" '7b
Data "*rnbq1rk1p1pp1ppp1p2pn8B3PP4P1Q6P2PPPPR3KBNR*7", "9371", "50", "9382" '7c
Data "*rnbq1rk1p1pp1ppp1p2pn12PP4P1Q2N3P2PPPPR1B1KB1R*7", "9382", "50", "7655" '7d
Data "*rnbq1rk1p1pp1ppp1p2pn12PP4P1Q2P3P2P1PPR1B1KBNR*7", "8363", "50", "8464" '7e
Data "*rnbqkb1rppp2ppp4pn5p2B3PP6N5PP2PPPPR2QKBNR*4", "9685", "50", "9284" '8
Data "*rnbqkbnrppp1pppp18pPP11PP3PPPRNBQKBNR*3", "8565", "50", "8363" '9
Data "*rnbqkbnrpp2pppp10pP6p1P11PP3PPPRNBQKBNR*4", "9776" '9a
Data "*rnbqkb1rpp2pppp5n4pP6p1P5N5PP3PPPR1BQKBNR*5", "8262" '9b
Data "*rnbqkb1rp3pppp5n3ppPP5p7N5PP3PPPR1BQKBNR*6", "6252" '9c
Data "*rnbqkb1rp3pppp5P4pP5pp7N5PP3PPPR1BQKBNR*7", "5243" '9d
Data "*rnbqkb1rp3pppp5P4pP6p7P5P4PPPR1BQKBNR*8", "9284" '9e
Data "*rnbqkb1rp3pppp5n3ppP6p1PB4N5PP3PPPR2QKBNR*6", "8171", "75", "9371" '9m
Data "*rnbqkb1r4ppppp4n3ppPP5p2B4N5PP3PPPR2QKBNR*7", "6252"  '9n
Data "*rnbqkb1r4ppppp4P4pP5pp2B4N5PP3PPPR2QKBNR*8", "5243" '9o

Data "*rnbqkb1rpppp1ppp12P5P3n6N2PP2PPPPRNBQKB1R*4", "9663", "50", "9273"   '10
Data "*rnbqkb1rpppp1ppp12P5P2Bn9PP2PPPPRN1QKBNR*4", "9273" '10a
Data "*r1bqkb1rpppp1ppp2n9P5P2Bn6N2PP2PPPPRN1QKB1R*5", "9652" '10b
Data "*r1bqk2rpppp1ppp2n9P4bP2Bn6N2PP1NPPPPR2QKB1R*6", "9485" '10c
Data "*r1b1k2rppppqppp2n9P4bP2Bn5PN2PP1N1PPPR2QKB1R*7", "5765" '10d
Data "*r1b1k2rppppqppp2n9P4bP2Bn1P4N3P1NPPPPR2QKB1R*7", "7365", "50", "5765" '10e  ' changed from 7364 in 0.4_b2 - leo bug 4/10/09
Data "*rnbqkb1rpppp1ppp4pn12PP9N2PP2PPPPRNBQKB1R*3", "8464", "50", "9652" '11
Data "*rnbqkbnrppp2ppp12p5pPP8N2PP3PPPRNBQKB1R*4", "6554", "50", "9652" '12
Data "*rnbqk2rpppp1ppp4pn11bPP9N2PP1BPPPPRN1QKB1R*4", "9485" '13
Data "*rnb1k2rppppqppp4pn11bPP9NP1PP1BPP1PRN1QKB1R*5", "9273" '13a
Data "*r1b1k2rppppqppp2n1pn11bPP9NP1PP1BPPBPRN1QK2R*6", "5234" '13b
Data "*rnbqk2rpppp1ppp4pn11bPP9N2PP1NPPPPR1BQKB1R*4", "8272" '13c
Data "*rnbqk2rp1pp1ppp1p2pn11bPP4P4N3P1NPPPPR1BQKB1R*5", "5234" '13d
Data "*rnbqk2rp1pp1ppp1p2pn12PP4P4N3P1BPPPPR2QKB1R*6", "9382" '13e
Data "*rnbqk2rp1pp1ppp1p2pn12PP4P4N3P1QPPPPR1B1KB1R*6", "9382" '13f
Data "*rnbqk2rpppp1ppp4pn11bPP6N1P3PP3PPPR1BQKBNR*4", "9597" '14
Data "*rnbq1rk1pppp1ppp4pn11bPP6NBP3PP3PPPR1BQK1NR*5", "8464" '14a
Data "*rnbq1rk1ppp2ppp4pn5p5bPP6NBPN2PP3PPPR1BQK2R*6", "8272", "50", "8363" '14b
Data "*rnbq1rk1pppp1ppp4pn11bPP6N1P3PP2NPPPR1BQKB1R*5", "8464" '14c
Data "*rnbq1rk1ppp2ppp4pn5p5bPP4P1N1P4P2NPPPR1BQKB1R*6", "5285", "75", "5274" '14d
Data "*rnbqk2rpppp1ppp4pn11bPP6N2N2PP2PPPPR1BQKB1R*4", "8464" '14e
Data "*rnbqk2rppp2ppp4pn5p2B2bPP6N2N2PP2PPPPR2QKB1R*5", "6453" '14f
Data "*rnbqk2rppp2ppp4pn8B2bpPP5N2N2PP3PPPR2QKB1R*6", "8363", "50", "8262" '14g
Data "*rnbqk2rppp2ppp4pn5P5b1P6N2N2PP2PPPPR1BQKB1R*5", "7564" '14h
Data "*rnbqk2rppp2ppp5n5p2B2b1P6N2N2PP2PPPPR2QKB1R*6", "8878", "50", "9597" '14i
Data "*rnbqk2rppp2ppp4pn5p5bPP6N1PN2PP3PPPR1BQKB1R*5", "8363", "50", "9597" '14j
Data "*rnbqk2rpp3ppp4pn4pp5bPP6NBPN2PP3PPPR1BQK2R*6", "6453", "50", "9597" '14k
Data "*rnbqk2rppp2ppp4pn5p4QbPP6N2N2PP2PPPPR1B1KB1R*5", "9273" '14l
Data "*r1bqk2rppp2ppp2n1pn5pN3QbPP6N5PP2PPPPR1B1KB1R*6", "9384" '14m
Data "*rnbqk2rppp1bppp4pn5p2B3PP6N1P3PP3PPPR2QKBNR*5", "9597", "50", "9284" '15
Data "*rnbqkbnrppp2ppp4p6p6PP9N2PP2PPPPRNBQKB1R*3", "8373" '16
Data "*rnbqkbnrpp3ppp2p1p6p6PP8PN2PP3PPPRNBQKB1R*4", "9674" '16a
Data "*rnbqk1nrpp3ppp2pbp6p6PP7BPN2PP3PPPRNBQK2R*5", "9284" '16b
Data "*rnbqkbnrpp3ppp2p1p6p6PP6N2N2PP2PPPPR1BQKB1R*4", "6453" '16h
Data "*rnbqkbnrpp3ppp2p1p11P1pP6N2N3P2PPPPR1BQKB1R*5", "9652" '16i
Data "*rnbqkbnrpp3ppp2p1p6p6PP9N2PPQ1PPPPRNB1KB1R*4", "6453", "50", "9776" '16p

Data "*rnbqk2rppp2ppp4pn5p5bPP6N1PN2PP3PPPR1BQKB1R*5", "9597", "50", "8363" '17
Data "*rnbq1rk1ppp1bppp4pn5p2B3PP6N1PN2PP3PPPR2QKB1R*6", "8878", "50", "9284" '18
Data "*r1bqk2rpppnbppp4pn5p2B3PP6N1PN2PP3PPPR2QKB1R*6", "9597" , "50", "8373" '19
Data "*rnbqk2rppp1bppp4pn5p2B3PP6N2N2PP2PPPPR2QKB1R*5", "9597" , "50", "8878" '19a
Data "*r1bqkb1rpppp1ppp2n9P5P3n3N2N2PP2PPPPR1BQKB1R*5", "5765", "75", "9663" '20
Data "*r1bqkb1rpppp1ppp2n9N5P7N5PP2PPPPR1BQKB1R*6", "7365" '20_1
Data "*r1bqkb1rpppp1ppp11Qn5P7N5PP2PPPPR1B1KB1R*7", "8474" '20_2
Data "*r1bqkb1rpppp1ppp12n3Q1P7N5PP2PPPPR1B1KB1R *7", "9685" '20_3
Data "*r1bqkb1rpppp1ppp12n5P7N1P3PP3PPPR1BQKB1R*7", "9652", "50", "8474" '20_4
Data "*r1bqkb1rpppp1ppp12n5PQ6N5PP2PPPPR1B1KB1R*7", "8474" '20_5
Data "*r1bqkb1rpppp1ppp2n9P5P2Bn6N2PP2PPPPRN1QKB1R*5", "9652" '20a
Data "*r1bqk2rpppp1ppp2n9P4bP2Bn6N2PP1NPPPPR2QKB1R*6", "9485" '20b
Data "*r1bqk2rpppp1ppp2n9P4bP2Bn3N2N2PP2PPPPR2QKB1R*6", "9485" '20b1
Data "*r1b1k2rppppqppp2n8QP4bP2Bn3N2N2PP2PPPPR3KB1R*7", "5243", "50", "8676" '20b2
Data "*r1b1k2rppppq1pp2n2P5Q5bP2Bn3N2N2PP2PPPPR3KB1R*8", "5776" '20b3
Data "*r1b1k2rppppqppp2n8QP5P2Bn3P2N2P3PPPPR3KB1R*8", "8676" '20b4
Data "*r1bqkb1rpppp1ppp2n9P5P3n5PN2PP3PPPRNBQKB1R*5", "5765" '20c
Data "*rnbqk2rpppp1ppp10b1P5P3n5PN2PP3PPPRNBQKB1R*5", "9273" '20d
Data "*rnbqkb1rpppp1ppp12P5PQ2n9PP2PPPPRNB1KBNR*4","8474" '20f
Data "*r1bqkb1rpppp1ppp2n9P1B3P3n6N2PP2PPPPRN1QKB1R*5", "9685" '20x
Data "*r1bqk2rppppbppp2n9P5P2Bn6N2PP2PPPPRN1QKB1R*6", "8552" '20y

Data "*rnbqkb1rpppppppp5n13P9N2PPP1PPPPRNBQKB1R*2", "8575", "50", "8464" '21
Data "*rnbqk1nrppp2ppp12p4bpPP8N2PP1B1PPPRN1QKB1R*5", "5234"  '22
Data "*rnbqkb1rpppppppp5n8B4P12PPP1PPPPRN1QKBNR*2", "8464", "50", "8363" '24
Data "*rnbqkb1rppp1pppp5B5p7P12PPP1PPPPRN1QKBNR*3", "8576" '24a
Data "*rnbqkb1rppp2ppp5p5p7P8P3PPP2PPPRN1QKBNR*4", "9674", "50", "9685" '24b
Data "*rnbqk2rppp2ppp3b1p5p6PP8P3PP3PPPRN1QKBNR*5", "6453" '24c
Data "*rnbqkb1rppp1pppp5n5p2B4P8P3PPP2PPPRN1QKBNR*3", "8363" '25
Data "*rnbqkb1rpp2pppp5B4pp7P8P3PPP2PPPRN1QKBNR*4", "8576", "50", "8776" ' 25a
Data "*rnbqkb1rpp2pp1p5p4pp7P6P1P3PP3PPPRN1QKBNR*5", "9472" '25b
Data "*rnbqkb1rpp2pppp5n4pp2B4P6P1P3PP3PPPRN1QKBNR*4", "9273", "50", "9472" '25c
Data "*rnbqkb1rppp1pppp5n5p2B4P12PPPNPPPPR2QKBNR*3", "9273", "50", "9284" '25d
Data "*rnbqkb1rpp1ppppp5n4p3B4P12PPP1PPPPRN1QKBNR*3", "8776" '26
Data "*rnbqkb1rpp1ppp1p5p4pP20PPP1PPPPRN1QKBNR*4", "9472", "50", "9687" '26a
Data "*rnb1kb1rpp1ppp1p1q3p4pP20PPP1PPPPRNQ1KBNR*5", "7666", "50", "9687" '26b
Data "*rnbqkb1rpp1ppppp5n4pP2B17PPP1PPPPRN1QKBNR*3", "7655" '26c
Data "*rnbqkb1rpp1ppppp10pP8nB10PPP1PPPPRN1QKBNR*4", "8474" '26d
Data "*rnbqkb1rpp2pppp3p6pP8nB7P2PPP1P1PPRN1QKBNR*5", "5576", "50", "9461" '26e
Data "*rnbqkb1rpp1ppppp10pP2B5n2P8PPP1PPP1RN1QKBNR*4","9472" '26f
Data "*rnbqkb1rppp2ppp4pn5p6PP6N2N2PP2PPPPR1BQKB1R*4", "9685", "75", "9284" '23
Data "*rnbqkb1rpppp1ppp4pn8B4P9N2PPP1PPPPRN1QKB1R*3", "8363" '23a

Data "*rnbqkbnrppp1pppp18pP9N2PP2PPPPRNBQKB1R*3", "9776" '27
Data "*rnbqkb1rppp1pppp5n12pP8PN2PP3PPPRNBQKB1R*4", "8575" '27a
Data "*rnbqkb1rppp2ppp4pn12BP8PN2PP3PPPRNBQK2R*5", "8171" '27b
Data "*rnbqkb1r1pp2pppp3pn12BP8PN2PP3PPPRNBQ1RK*6", "8262", "50", "8363" '27c
Data "*rnbqkbnrppp1pppp18pP8P3PP3PPPRNBQKBNR*3", "9776" '28
Data "*rnbqkb1rppp1pppp5n12BP8P3PP3PPPRNBQK1NR*4", "8575" '28a
Data "*rnbqkb1rppp2ppp4pn5P7P6N5PP2PPPPR1BQKBNR*4", "7564" '29
Data "*rnbqkb1rppp2ppp5n5p2B4P6N5PP2PPPPR2QKBNR*5", "9685" '29a
Data "*rnbqk2rppp1bppp5n5p2B4P42N1P3PP3PPPR2QKBNR*6", "8373" '29b
Data "*rnbqkbnrppp1pppp18pP8P3PP3PPPRNBQKBNR*3", "9776", "50", "8575" '30
Data "*rnbqkbnrpppppppp18P13PP1PPPPPRNBQKBNR*1", "9776", "50", "8575" ' 201
Data "*rnbqkbnrpppp1ppp4p13PP12PP2PPPPRNBQKBNR*2", "8464", "50", "9776" '202
Data "*rnbqkbnrpppp1ppp4p13P11P1PP1PPP1PRNBQKBNR*2", "8464" '204a
Data "*rnbqkbnrppp2ppp4p6p6P11P1PP1PPPBPRNBQK1NR*3", "9776", "50", "6453" '204b
Data "*rnbqkb1rppp2ppp4pn5p6P10NP1PP1PPPBPRNBQK2R*4", "6453", "50", "6454" '204c
Data "*rnbqkb1rpppp1ppp4pn12P7N2N2PP1PPPPPR1BQKB1R*3", "9652" '205a
Data "*rnbqkbnrpppp1ppp4p13P7N5PP1PPPPPR1BQKBNR*2", "8464" '206
Data "*rnbqkbnrppp2ppp4p6P14N5PP1PPPPPR1BQKBNR*3", "7564" '206a
Data "*rnbqkbnrpppp1ppp4p13P10N2PP1PPPPPRNBQKB1R*2", "9776" '207a
Data "*rnbqkb1rpppp1ppp4pn12P10NP1PP1PPP1PRNBQKB1R*3", "8464" '207b

Data "*rnbqkb1rpppppppp5n12P7N5PP1PPPPPR1BQKBNR*2", "8575", "50", "8363" '203

Data "*rnbqkb1rpppppppp5n12P11P1PP1PPP1PRNBQKBNR*2", "8373" '203p
Data "*rnbqkb1rpp1ppppp2p2n12P11P1PP1PPPBPRNBQK1NR*3", "8464" '203q
Data "*rnbqkb1rpp2pppp2p2n5p6P10NP1PP1PPPBPRNBQK2R*4", "6453" '203r
Data "*rnbqkb1rpp2pppp2p2n5P18P1PP1PPPBPRNBQK1NR*4", "7364" '203s
Data "*rnbqkb1rpp1ppppp2p2n12P10NP1PP1PPP1PRNBQKB1R*3", "8464" '203v
Data "*rnbqkb1rpp2pppp2p2n12p10NP1PP1PPPBPRNBQ1RK*5", "9284", "50", "8575" '203w


Data "*rnbqkbnrpppppppp29N2PPPPPPPPRNBQKB1R*1", "9776", "50", "8464" '301
Data "*rnbqkbnrppp1pppp11p7P9N2PPP1PPPPRNBQKB1R*2", "9776"    '302
Data "*rnbqkb1rppp1pppp5n5p6PP9N2PP2PPPPRNBQKB1R*3", "8575"  '303
Data "*rnbqkbnrppp1pppp11p6P10N2PP1PPPPPRNBQKB1R*2", "6454" ' 304
Data "*rnbqkbnrppp1pppp11p17NP1PPPPPP1PRNBQKB1R*2", "9357" '310
Data "*rn1qkbnrppp1pppp11p10b6NP1PPPPPPBPRNBQK2R*3", "8575" '310a
Data "*rn1qkbnrppp2ppp4p6p10b6NP1PPPPPPBPRNBQ1RK*4", "9284", "50", "9776" '310b
Data "*rn1qkbnrppp1pppp11pN9b7P1PPPPPP1PRNBQKB1R*3", "5766" '310g
Data "*rn1qkbnrppp1pppp11pNb16P1PPPPPPBPRNBQK2R*4", "9284", "50", "8373" '310h
Data "*rnbqkb1rpppppppp5n12P10N2PP1PPPPPRNBQKB1R*2", "8575"   '351

Data "*rnbqkb1rpppppppp5n23NP1PPPPPP1PRNBQKB1R*2", "8464" '352a
Data "*rnbqkb1rppp1pppp5n5p17NP1PPPPPPBPRNBQK2R*3", "8373", "50", "9366" '352b
Data "*rnbqkb1rpp2pppp2p2n5p17NP1PPPPPPBPRNBQ1RK*4", "9357" '352c
Data "*rn1qkb1rpp2pppp2p2n5p10b4P1NP1PPP1PPBPRNBQ1RK*5", "8575" '352d

Data "*rnbqkbnrpppppppp20P11PPPP1PPPRNBQKBNR*1", "8565", "50", "8373" '501
Data "*rnbqkbnrpppp1ppp12p7P8N2PPPP1PPPRNBQKB1R*2", "9273", "80", "9776" '502
Data "*rnbqkb1rpppp1ppp5n6N7P11PPPP1PPPRNBQKB1R*3", "8474" ' 503
Data "*rnbqkb1rppp2ppp3p1n14P8N2PPPP1PPPRNBQKB1R*4", "7655" ' 504
Data "*rnbqkb1rppp2ppp3p15Pn8N2PPP2PPPRNBQKB1R*5", "7464"'504a
Data "*rnbqkb1rppp2ppp11p7Pn6B1N2PPP2PPPRNBQK2R*6", "9273", "50", "9685" '504b
Data "*rnbqkb1rppp2ppp3p16n5N2N2PPPP1PPPR1BQKB1R*5", "5543", " 50", "5576" '504c
Data "*rnbqkb1rppp2ppp3p22P2N2PPP2PPPR1BQKB1R*6", "9685", "50", "9273" '504d
Data "*r1bqkb1rppp2ppp2np22P1BN2PPP2PPPR2QKB1R*7", "9685", "50", "9357" '504e
Data "*rnbqk2rppp1bppp3p17B4P2N2PPP2PPPR2QKB1R*7", "9273", "50", "9597" '504f

Data "*rnbqkb1rpppp1ppp5n6p6PP8N2PPP2PPPRNBQKB1R*3", "7655"  '510
Data "*rnbqkb1rpppp1ppp12p6Pn6B1N2PPP2PPPRNBQK2R*4", "8464" '510a
Data "*rnbqkb1rppp2ppp11pN6Pn6B4PPP2PPPRNBQK2R*5", "9284" '510a1
Data "*r1bqkb1rpppN1ppp11p7Pn6B4PPP2PPPRNBQK2R*6", "9384" '510a2
Data "*rnbqkb1rppp2ppp11pP7n6B1N2PPP2PPPRNBQK2R*5", "5563" '510a6

Data "*rnbqkb1rpppp1ppp5n6p5B1P8N2PPPP1PPPRNBQK2R*3", "9273" '510b
Data "*rnbqkb1rpppp1ppp5n6p7P5N2N2PPPP1PPPR1BQKB1R*3", "9273" '510c
Data "*r1bqkb1rpppp1ppp2n2n3B2p7P5N2N2PPPP1PPPR1BQK2R*4", "7354" '510d

Data "*rnbqkb1rpppp1ppp12P7n8N2PPP2PPPRNBQKB1R*4", "8464" '510t
Data "*rnbqkb1rppp2ppp11pP7n8N2PPPN1PPPR1BQKB1R*5", "5563" '510u

Data "*r1bqkb1rpppp1ppp2n2n6p6PP5N2N2PPP2PPPR1BQKB1R*4", "6554" '511
Data "*r1bqkb1rpppp1ppp2n2n13NP5N5PPP2PPPR1BQKB1R*5", "9652" '511a
Data "*r1bqk2rpppp1ppp2N2n11b2P5N5PPP2PPPR1BQKB1R*6", "8273" '511b

DATA "*r1bqkb1rpppp1ppp2n2n6p5BPP8N2PPP2PPPRNBQK2R*4", "6554" '513
DATA "*r1bqkb1rppp2ppp2n8B7pn8N2PPP2PPPRNBQR1K*7", "9464" '513a
DATA "*r1b1kb1rppp2ppp2n8q7pn5N2N2PPP2PPPR1BQR1K*8", "6468", "50", "6461" '513b

Data "*r1bqkbnrpppp1ppp2n9p5B1P8N2PPPP1PPPRNBQK2R*3", "9776" '530
Data "*r1bqkb1rpppp1ppp2n2n6p5B1P6P1N2PPP2PPPRNBQK2R*4", "9685" '530a
Data "*r1bqk2rppppbppp2n2n6p5B1P6P1N2PPP2PPPRNBQ1RK*5", "9597" '530b
Data "*r1bq1rk1ppppbppp2n2n6p5B1P6P1N2PPP2PPPRNBQR1K*6", "8878", "50", "8171" '530c
Data "*r1bq1rk1ppppbppp2n2n6p7P4B1P1N2PPP2PPPRNBQ1RK*6", "8878", "50", "8474" '530h

Data "*r1bqkb1rpppp1ppp2n2n6p1N3B1P11PPPP1PPPRNBQK2R*4", "8464", "50", "9663" '531
Data "*r1bqk2rpppp1Npp2n2n4b1p5B1P11PPPP1PPPRNBQK2R*5", "6336" '531a
Data "*r1bqk2rpppp1Bpp2n2n4b1p1N5P11PPPP1PPPRNBQK2R*5", "9585" '531b
Data "*r1bq3rppppk1pp2n2n4bBp1N5P11PPPP1PPPRNBQK2R*6", "8474", "50", "9896" '531c
Data "*r1bq1r2ppppk1pp2n2n4bBp1N5P11PPPP1PPPRNBQ1RK*7", "8474", "50", "8878" '531d
Data "*r1bqkb1rppp2ppp2n2n5Pp1N3B13PPPP1PPPRNBQK2R*5", "8262" '531p
Data "*r1bqkb1rp1p2ppp2n2n3p1Pp1N17PPPP1PPPRNBQKB1R*6", "8878" '531o
Data "*r1bqkb1rp1p2ppp2n2n3B1Pp1N17PPPP1PPPRNBQK2R*6", "9464" '531r
Data "*r1bqkb1rp1p2ppp2P2n3p2p1N3B13PPPP1PPPRNBQK2R*6", "6253" '531t

Data "*r1bqkbnrpppp1ppp2n6B2p7P8N2PPPP1PPPRNBQK2R*3", "8171" '550
Data "*r1bqkbnr1ppp1pppp1B9p7P8N2PPPP1PPPRNBQK2R*4", "8473" '550a
Data "*r1bqkbnr1pp2pppp1p9p7P8N2PPPP1PPPRNBQ1RK*5", "9674", "50", "9474" '550b
Data "*r1bqk1nr1pp2pppp1pb8p6PP8N2PPP2PPPRNBQ1RK*6", "6554" '550c
Data "*r1bqkbnr1ppp1pppp1n9p3B3P8N2PPPP1PPPRNBQK2R*4", "9776", "50", "8474" '551
Data "*r1bqkbnr1pp2pppp1np8p3B3P5P2N2PP1P1PPPRNBQK2R*5", "8666" '551a
Data "*r1bqkbnr1pp3ppp1np8pP2B9P2N2PP1P1PPPRNBQK2R*6", "9366" '551b
Data "*r1bqkbnr1pp2pppp1np8p3B3P8N2PPPP1PPPRNBQ1RK*5", "9776" '551c
Data "*2brkb3pp2pppp1np1n6p3B3P8N2PPPP1PPPRNBQR1K*6", "9357" '551d
Data "*r1bqkb1r1ppp1pppp1n2n6p3B2PP8N2PPP2PPPRNBQK2R*5", "6554" '551g ' changed 16/1/2010 - bug found in 0.5pr by Leo in Div 5c test
Data "*r1bqkb1r1ppp1pppp1n2n10B2pP8N2PPP2PPPRNBQ1RK*6", "9685" '551h
Data "*r1bqkb1r1ppp1pppp1n2n6p3B3P6P1N2PPP2PPPRNBQK2R*5", "8474" '551i
Data "*r1bqkb1r1pp2pppp1np1n6p3B3P5PP1N2PP3PPPRNBQK2R*6", "9384", "50", "9685" '551j
Data "*r1bqk2r1pp1bpppp1np1n6p1B1B3P5PP1N2PP3PPPRN1QK2R*7", "9597" '551k
Data "*r1bqkb1r1pp2pppp1np1n6p3B1P1P6P1N2PP3PPPRNBQK2R*6", "9685" '551l
Data "*r1bqk2r1pp1bpppp1np1n6p3B1P1P5NP1N2PP3PPPR1BQK2R*7", "9597" '551m
Data "*r1bqkb1r1ppp1pppp1n2n6p3B3P8N2PPPP1PPPRNBQ1RK*5", "9685", "50", "7655" '552
Data "*r1bqkb1r1ppp1pppp1n9p3B2Pn8N2PPP2PPPRNBQ1RK*6", "8262", "50", "6554" '553
Data "*r1bqkb1r1ppp1pppp1n13B2pn8N2PPP2PPPRNBQR1K*7", "8464" '553a
Data "*r1bqkb1r2pp1pppp1n6p2p6Pn4B3N2PPP2PPPRNBQ1RK*7", "8464" '553b
Data "*r1bqkb1r2p2pppp1n6p1pP7n4B3N2PPP2PPPRNBQ1RK*8", "9375" '553c
Data "*r1bqk2r1pppbpppp1B2n6p7P8N2PPPP1PPPRNBQ1RK*6", "8473" '553f
Data "*r1bqk2r1pppbpppp1n2n6p3B3P8N2PPPP1PPPRNBQR1K*6", "8262" '554
Data "*r1bqk2r2ppbpppp1n2n3p2p7P4B3N2PPPP1PPPRNBQR1K*7", "8474", "50", "9597" '555
Data "*r1bq1rk3ppbpppp1n2n3p2p7P4BP2N2PP1P1PPPRNBQR1K*8", "8464", "50", "8474" '556
Data "*r1bqk2r2p1bpppp1np1n3p2p7P4BP2N2PP1P1PPPRNBQR1K*8", "9597" '557
Data "*r1bqkbnr1pp2pppp1np8p3B3P5P2N2PP1P1PPPRNBQK2R*5", "9384", "50", "9776" '560
Data "*r2qkbnr1ppb1pppp1np8p3B3P5P2N2PP1P1PPPRNBQ1RK*6", "9785", "50", "9776" '561
Data "*r1bqkbnrpppp1ppp2n9p6PP8N2PPP2PPPRNBQKB1R*3", "6554" '568
Data "*r1bqkbnrpppp1ppp2n16NP11PPP2PPPRNBQKB1R*4", "9652", "50", "9776" '568a
Data "*r1bqk1nrpppp1ppp2n14b1NP5P5PP3PPPRNBQKB1R*5", "5263", "50", "5285" '568b
Data "*r1bqk1nrpppp1ppp2N7b9P5P5PP3PPPRNBQKB1R*6", "8273" '568c
Data "*r1bqk1nrp1pp1ppp2p7b9P5PB4PP3PPPRNBQK2R*7", "8474", "50", "9458" '568d
Data "*r1bqk1nrp1pp1ppp2p7b9P5P5PP1N1PPPR1BQKB1R*7", "8474", "50", "9476" '568e
Data "*r1bqkb1rpppp1ppp2N2n14P11PPP2PPPRNBQKB1R*5", "8273" '568f
Data "*r1bqkb1rp1pp1ppp2p2n6P19PPP2PPPRNBQKB1R*6", "9485", "50", "7664" '568g
Data "*r1bqkb1rpppp1ppp2n2n13NP5N5PPP2PPPR1BQKB1R*5", "9652", "50", "9663" ' 568h
Data "*r1bqkb1rp1pp1ppp2p2n14P6B4PPP2PPPRNBQK2R*6", "8464", "50", "8474" '568i
Data "*r1bqk2rpppp1ppp2N2n11b2P5N5PPP2PPPR1BQKB1R*6", "8273" ' 568j
Data "*r1bqk1nrppppbppp2n15BNP5P5PP3PPPRNBQK2R*6", "8474" '568k
Data "*r1bqk1nrppp1bppp2Np14B1P5P5PP3PPPRNBQK2R*7", "8273" '568k1
Data "*r1bqk1nrppppbppp2n16NP5P1B3PP3PPPRN1QKB1R*6", "9776" '568k7

Data "*r1bqk1nrppppbppp2N17P5P5PP3PPPRNBQKB1R*6", "8273" '568l
Data "*r1bqk1nrp1ppbpp2p15B1P5P5PP3PPPRNBQK2R*7", "9776" '568m
Data "*r1bqk2rp1ppbppp2p2n6P5B7P5PP3PPPRNBQK2R*8", "7655" '568n

Data "*r1bqkbnrpppp1ppp2n15BpP8N2PPP2PPPRNBQK2R*4", "9776" '568r1
Data "*r1bqkb1rpppp1ppp2n2n12BpP8N2PPP2PPPRNBQ1RK*5", "7655" '568s1
Data "*r1bqkb1rpppp1ppp2n15Bpn8N2PPP2PPPRNBQR1K*6", "8464" '568t1
Data "*r1bqkb1rpppp1ppp2n2n6P5Bp9N2PPP2PPPRNBQK2R*5", "7657", "50", "7655" '568v1
Data "*r1bqkb1rpppp1ppp2n2n8N3BpP11PPP2PPPRNBQK2R*5", "7365", "50", "8464" '568x1
Data "*r1bqkb1rpppp1ppp5n6n1N4pP4B6PPP2PPPRNBQK2R*6", "8464" '568x2
Data "*r1bqkb1rppp2ppp2n2n5P2N3Bp12PPP2PPPRNBQK2R*6", "9485" '568y1

Data "*r1bqk1nrpppp1ppp2n7b8NP5P1B3PP3PPPRN1QKB1R*6", "6372" '568s

Data "*r1bqk1nrpppp1ppp1bn10N6P5P1B3PP3PPPRN1QKB1R*7", "7245" '586t
Data "*r1bqk1nrpppp1ppp2n17P5P1N3PP3PPPRN1QKB1R*8", "9785", "50", "7385" '586u
Data "*r1bqk1nrpppp1ppp1bn16NP1Q3P1B3PP3PPPRN2KB1R*7", "9476" '586v
Data "*r1bqk1nrpppp1ppp1bn15BNP5P1B3PP3PPPRN1QK2R*7", "7365", "50", "9776" '568w

Data "*r1bqkbnrpppp1ppp2n9p7P5N2N2PPPP1PPPR1BQKB1R*3", "9776", "50", "9663" '569
Data "*rnbqkbnrpppp1ppp12p6PP11PPP2PPPRNBQKBNR*2", "6554" '570
Data "*rnbqkbnrpppp1ppp19QP11PPP2PPPRNB1KBNR*3", "9273" '570a
Data "*rnbqkbnrpppp1ppp12p5B1P11PPPP1PPPRNBQK1NR*2", "9776" '573
Data "*rnbqkb1rpppp1ppp5n6p5B1P6P4PPP2PPPRNBQK1NR*3", "9663" '573a
Data "*rnbqk2rpppp1ppp5n4b1p5B1P6P1N2PPP2PPPRNBQK2R*4", "9273", "50", "9597" '573b
Data "*rnbq1rk1pppp1ppp5n4b1N5B1P6P4PPP2PPPRNBQK2R*5", "8464" '573b1
Data "*rnbq1rk1pppp1ppp5n4b1p5B1P6P1N2PPP2PPPRNBQ1RK*5", "8474" '573b2

Data "*rnbqk2rpppp1ppp5n4b1p5B1P5NP4PPP2PPPR1BQK1NR*4", "8373" '573c
Data "*rnbqk2rpp1p1ppp2p2n4b1p5B1P5NP1N2PPP2PPPR1BQK2R*5", "8464" '573c1
Data "*rnbqk2rpp3ppp2p2n4bPp5B7NP1NPPP2PPPR1BQK2R*6", "7364" '573c2
Data "*rnbqk2rpp3ppp5n3Bbpp13NP1N2PPP2PPPR1BQK2R*7", "9384" '573c3

Data "*rnbqkb1rpppp1ppp5n6p5BPP11PPP2PPPRNBQK1NR*3", "6554" '573d
Data "*rnbqkb1rpppp1ppp5n12BpP8N2PPP2PPPRNBQK2R*4", "9273" '573e
Data "*rnbqkb1rpppp1ppp5n6p5B1P5N5PPPP1PPPR1BQK1NR*3", "9273" '573f
Data "*rnbqkbnrpppp1ppp12p7PP10PPPP2PPRNBQKBNR*2", "9663" '591
Data "*rnbqk1nrpppp1ppp10b1p7PP7N2PPPP2PPRNBQKB1R*3", "8474", "50", "8464" '591a
Data "*rnbqk1nrpppp1ppp10b1p7PP4N5PPPP2PPR1BQKBNR*3", "8474" '591b
Data "*rnbqk1nrppp2ppp3p6b1p7PP4N5PPPP2PPR1BQKBNR*4", "9776" '591c

Data "*rnbqk1nrppp2ppp3p6b1p7PP4P2N2PP1P2PPRNBQKB1R*4", "9776" '591g
Data "*rnbqk2rppp2ppp3p1n4b1p6PPP4P2N2PP4PPRNBQKB1R*5", "6554", "50", "6372" '591h

Data "*rnbqk1nrppp2ppp10bpN7PP10PPPP2PPRNBQKB1R*4", "6455" '591v
Data "*rnbqk1nrppp2ppp10bPp8P7N2PPPP2PPRNBQKB1R*4", "6555" '591w

Data "*rnbqkbnrpppp1ppp12p7P5N5PPPP1PPPR1BQKBNR*2", "9663" '592
Data "*rnbqk1nrpppp1ppp10b1p7P5N2N2PPPP1PPPR1BQKB1R*3", "8474" '592a
Data "*rnbqk1nrppp2ppp3p6b1p6PP5N2N2PPP2PPPR1BQKB1R*4", "6554" '592b
Data "*rnbqk1nrppp2ppp3p6b8NP5N5PPP2PPPR1BQKB1R*5", "9776" '592c
Data "*rnbqk1nrpppp1ppp10b1p5B1P5N5PPPP1PPPR1BQK1NR*3", "8474" '593
Data "*rnbqk1nrppp2ppp3p6b1p5B1P5NP4PPP2PPPR1BQK1NR*4", "9776", "50", "9273" '593a
Data "*rnbqkbnrpp1ppppp2p16PP11PPP2PPPRNBQKBNR*2", "8464" '601
Data "*rnbqkbnrpp2pppp2p8p7PP5N5PPP2PPPR1BQKBNR*3", "6455" '602
Data "*rnbqkbnrpp2pppp2p16PN11PPP2PPPR1BQKBNR*4", "9366", "50", "9776" '603
Data "*rn1qkbnrpp2pppp2p10b5P10N1PPP2PPPR1BQKBNR*5", "6677" '604
Data "*rn1qkbnrpp2pppp2p3b12P3P6N1PPP2PP1R1BQKBNR*6", "8878" '605
Data "*rn1qkbnrpp2ppp3p3bp7P3P10N1PPP2PP1R1BQKBNR*7", "7788" '607
Data "*rn1qkbnrpp2ppp3p3bp11P3P5NN1PPP2PP1R1BQKB1R*7", "8575", "50", "9776" '607a
Data "*rn1qkb1rpp2ppp3p2nbp4N6P3P6N1PPP2PP1R1BQKB1R*8", "7788" '607b
Data "*rn1qkb1rpp2ppp3p2nbp7P3P9NN1PPP2PP1R1BQKB1R*8", "7788" '607c
Data "*rn1qkbnrpp3pp3p1p1bp4N6P3P6N1PPP2PP1R1BQKB1R*8", "7788" '607d
Data "*rn1qkbnrpp3pp3p1p1bp7P3P9NN1PPP2PP1R1BQKB1R*8", "7788" '607e
Data "*rn1qkbnrpp2pppp2p3b11BP10N1PPP2PPPR1BQK1NR*6", "8575", "50", "9776" '608
Data "*rn1qkb1rpp2pppp2p2nb11BP10N1PPP1NPPPR1BQK2R*7", "8575" '608a
Data "*rn1qkbnrpp3ppp2p1p1b11BP10N1PPP1NPPPR1BQK2R*7", "9674", "50", "9776" '608b
Data "*rn1qkbnrpp2pppp2p3b12P10N1PPP1NPPPR1BQKB1R*6", "8575", "50", "9776" '608t
Data "*rn1qkbnrpp3ppp2p1p1b12P1N8N1PPP2PPPR1BQKB1R*7", "9674", "50", "9483" '608u

Data "*rn1qkbnrpp2pppp2p3b12P9NN1PPP2PPPR1BQKB1R*6", "9284", "75", "9776" '610
Data "*rn1qkb1rpp2pppp2p2nb12P7B1NN1PPP2PPPR1BQK2R*7", "8575" '610f

Data "*rnbqkb1rpp2pppp2p2N13P12PPP2PPPR1BQKBNR*5", "8576" '620
Data "*rnbqkb1rpp3ppp2p2p12BP12PPP2PPPR1BQK1NR*6", "9485" '620a
Data "*rnb1kb1rpp2qppp2p2p12BP12PPP1QPPPR1B1K1NR*7", "9357", "50","9375" '620b
Data "*rnbqkb1rpp3ppp2p2p13P6P5PP3PPPR1BQKBNR*6", "9375" '620c
Data "*rnbqkb1rpp3ppp2p2p13P9N2PPP2PPPR1BQKB1R*6", "9375" '620d
Data "*rnbqkb1rpp3ppp2p2p13P12PPP1BPPPR1BQK1NR*6", "9375" '620e

Data "*rnbqkb1rpp2pppp2p2n13P10N1PPP2PPPR1BQKBNR*5", "7363" '621
Data "*rnbqkb1rpp2pppp5n4p8P9NN1PPP2PPPR1BQKB1R*6", "9273", "50", "8575" '621a
Data "*rnbqkb1rpp2pppp5n4p8P8B1N1PPP2PPPR2QKBNR*6", "6354", "50", "7657" '621b

Data "*rnbqkbnrpp2pppp2p16Pp5N2P2PPP3PPR1BQKBNR*4", "5546" '625
Data "*rnbqkbnrpp2pppp2p16P6N2N2PPP3PPR1BQKB1R*5", "9357" '625a
Data "*rn1qkbnrpp2pppp2p15BP2b3N2N2PPP3PPR1BQK2R*6", "8575" '625b
Data "*rn1qkbnrpp3ppp2p1p13BP2b3N2N2PPP3PPR1BQ1RK*7", "9776" '625c
Data "*rn1qkbnrpp2pppp2p16P2b3N1BN2PPP3PPR2QKB1R*6", "9776" '625g

Data "*rnbqkbnrpp2pppp2p8p7PP6B4PPP2PPPRNBQK1NR*3", "6455" '629
Data "*rnbqkbnrpp2pppp2p16PB11PPP2PPPRNBQK1NR*4", "9776" '629a

Data "*rnbqkbnrpp2pppp2p8pP6P12PPP2PPPRNBQKBNR*3", "9366" '630
Data "*rn1qkbnrpp2pppp2p8pPb5P6N5PPP2PPPR1BQKBNR*4", "8575", "75", "9472" '631
Data "*rn1qkbnrpp3ppp2p1p6pPb5P2P3N5PPP2P1PR1BQKBNR*5", "6677" '631a
Data "*rn1qkbnrpp3ppp2p1p1b4pP6P2P3N5PPP1NP1PR1BQKB1R*6", "9458", "75", "9685" '631b
Data "*rn2kbnrpp3ppp2p1p1b4pP6P1NPq2N5PPP2P1PR1BQKB1R*7", "9652" '631b1
Data "*rn2kbnrpp3ppp2p1p1b4pP6P2Pq2N4PPPP1NP2R1BQKB1R*7", "9652" '631b2
Data "*rn2kbnrpp3ppp2p1p1b4pP6P2Pq2N3N1PPP2P1PR1BQKB1R*7", "9652" '631b3

Data "*rn1qk1nrpp2bppp2p1p1b4pP6P2P3N1B3PPP1NP1PR2QKB1R*7", "8567" '631c
Data "*rn1qkbnrpp3ppp2p1p1b4pP6P2PP2N5PPP2P2R1BQKBNR*6", "8878", "50", "8868" ' 631c1
Data "*rn1qkbnrpp3pp3p1p1bp3pP2P3P2P3N5PPP2P2R1BQKBNR*7", "7788" '631c2

Data "*rn1qkbnrpp3ppp2p1p6pPb5P6NB4PPP2PPPR1BQK1NR*5", "6644" '631d
Data "*rn1qkbnrpp3ppp2p1p6pPb5P6N2N2PPP2PPPR1BQKB1R*5", "7363", "50", "9284" '631e
Data "*rn1qkbnrpp3ppp4p4BppPb5P6N2N2PPP2PPPR1BQK2R*6", "9273" '631e1
Data "*r2qkbnrpp3ppp2n1p4BppPb5P6N2N2PPP2PPPR1BQ1RK*7", "9483", "50", "9472" '631e2

Data "*rn1qkbnrpp2pppp2p8pPb5P2P9PPP2P1PRNBQKBNR*4", "6655", "50", "6684"'631f
Data "*rn1qkbnrpp2pppp2p8pP6Pb1P6P2PPP4PRNBQKBNR*5", "5577" '631g
Data "*rn1qkbnrpp2pppp2p3b4pP6P2PP5P2PPP5RNBQKBNR*6", "8868", "50", "8878" '631h
Data "*rn1qkbnrpp2pppp2p8pPb5P9N2PPP2PPPRNBQKB1R*4", "8575", "50", "8878" '631i
Data "*rn1qkbnrpp3ppp2p1p6pPb5P9N2PPP1BPPPRNBQK2R*5", "9284", "50", "9652" '631m
Data "*rn1qkbnrpp2pppp2p8pPb5P12PPPN1PPPR1BQKBNR*4", "8575" '631p
Data "*rn1qkbnrpp3ppp2p1p6pPb5P2P9PPPN1P1PR1BQKBNR*5", "6677" '631q
Data "*rn1qkbnrpp3ppp2p1p1b4pP6P2PP8PPPN1P2R1BQKBNR*6", "8878", "50", "8868" '631r
Data "*rn1qkbnrpp3ppp2p1p1b4pP6P2P9PPPNNP1PR1BQKB1R*6", "7363" '631t

Data "*rn2kbnrpp2pppp1qp8pPb5P2P3N5PPP2P1PR1BQKBNR*5", "6684" '631x
Data "*rn2kbnrpp1bpppp1qp8pP3N2P2P9PPP2P1PR1BQKBNR*6", "7283" ' 631x1
Data "*rn2kbnrpp2pppp1qp8pPb5P6N2N2PPP2PPPR1BQKB1R*5", "8575" '631x5
Data "*rn2kbnrpp3ppp1qp1p6pPb5P6N2N2PPP1BPPPR1BQK2R*6", "6657" '631x6
Data "*rn2kbnrpp2pppp1qp8pPb5P6NB4PPP2PPPR1BQK1NR*5", "7254", "50", "6644" '631x7
Data "*rn2kbnrpp2pppp1qp8pP6P6NQ4PPP2PPPR1B1K1NR*6", "8575" ' 631x8
Data "*rn2kbnrpp3ppp1qp1p6pP6P6NQ4PPP1NPPPR1B1K2R*7", "9284" '631x9
Data "*rn2kbnrpp2pppp2p8pPb5q6NB1N2PPP2PPPR1BQK2R*6", "5457" '631x10
Data "*rn2kbnrpp2pppp2p8pPb8q3NB1N1PPPP2PP1R1BQK2R*7", "5768" '631x11

Data "*rnbqkbnrpp2pppp2p8P7P12PPP2PPPRNBQKBNR*3", "7364" '632
Data "*rnbqkbnrpp2pppp11p6PP12PP3PPPRNBQKBNR*4", "9776" '632a
Data "*rnbqkb1rpp2pppp5n5p6PP6N5PP3PPPR1BQKBNR*5", "9273", "50", "8575" '632b
Data "*rnbqkb1rpp3ppp4pn5p6PP6N2N2PP3PPPR1BQKB1R*6", "9685", "50", "9652" '632c
Data "*rnbqkbnrpp2pppp11p7P7B4PPP2PPPRNBQK1NR*4", "9273" '633
Data "*r1bqkbnrpp2pppp2n8p7P6PB4PP3PPPRNBQK1NR*5", "9483", "50", "9776" '633a
Data "*r1b1kbnrppq1pppp2n8p7P6PB4PP2NPPPRNBQK2R*6", "9357" '633b
Data "*r3kbnrppq1pppp2n8p7P2b3PB1P2PP2N1PPRNBQK2R*7", "5768", "50", "5784" '633c
Data "*r1bqkb1rpp2pppp2n2n5p7P1B4PB4PP3PPPRN1QK1NR*6", "9357" '633g
Data "*r2qkb1rpp2pppp2n2n5p7P1Bb2QPB4PP3PPPRN2K1NR*7", "9493" '633h

Data "*rnbqkbnrpp2pppp2p8p7PP11PPPN1PPPR1BQKBNR*3", "6455" '635
Data "*rnbqkbnrpp2pppp2p8p7PP8P2PPP3PPRNBQKBNR*3", "8575" '636
Data "*rnbqkbnrpp3ppp2p1p6p7PP5N2P2PPP3PPR1BQKBNR*4", "9652" '636a
Data "*rnbqk1nrpp3ppp2p1p6p5b1PPB4N2P2PPP3PPR2QKBNR*5", "9785" '637
Data "*rnbqk2rpp2nppp2p1p6p5b1PPB4NQ1P2PPP3PPR3KBNR*6", "9597" '637a
Data "*rnbqk2rpp2nppp2p1p6p5b1PPB4N2P2PPP1N1PPR2QKB1R*6", "9597" '637b
Data "*rnbqk1nrpp3ppp2p1p6p5b1PP5N2P2PPP1N1PPR1BQKB1R*5", "9785" '637e
Data "*rnbqk2rpp2nppp2p1p6p5b1PP3P1N2P21PP1N1PPR1BQKB1R*6", "5261" '637f
Data "*rnbqk1nrpp3ppp2p1p6p5b1PP5N2P2PPPB2PPR2QKBNR*5", "9785" '637i
Data "*rnbqk2rpp2nppp2p1p66p5b1PP3P1N2P3PPB2PPR2QKBNR*6", "5261", "50", "5243" '637j
Data "*rnbqk2rpp2nppp2p1p6p5b1PP8P2PPPBN1PPR2QKBNR*6", "5261", "50", "5234" '637m

Data "*rnbqkbnrpp1ppppp2p17P8N2PPPP1PPPRNBQKB1R*2", "8464" '639
Data "*rnbqkbnrpp2pppp2p8p8P5N2N2PPPP1PPPR1BQKB1R*3", "9357" ' 639a
Data "*rn1qkbnrpp2pppp2p8p8P1b3N2N1PPPPP1PP1R1BQKB1R*4", "5746", "50", "5768" '639b
Data "*rn1qkbnrpp2pppp2p8p8P5N2Q1PPPPP1PP1R1B1KB1R*5", "8575", "50", "9776" '639c

Data "*rnbqkbnrpp1ppppp2p17P5N5PPPP1PPPR1BQKBNR*2", "8464" '640
Data "*rnbqkbnrpp2pppp2p8p8P5N2Q2PPPP1PPPR1B1KBNR*3", "6455" '640a
Data "*rnbqkbnrpp2pppp2p17N8Q2PPPP1PPPR1B1KBNR*4", "9284" '640b
Data "*r1bqkbnrpp1npppp2p16PN8Q2PPP2PPPR1B1KBNR*5", "9776", "50", "8476" '640c
Data "*rnbqkbnrpp1ppppp2p17PP10PPPP2PPRNBQKBNR*2", "8464" '641
Data "*rnbqkbnrpp2pppp2p8pP8P10PPPP2PPRNBQKBNR*3", "9366" '641a
Data "*rn1qkbnrpp2pppp2p8pPb7P7N2PPPP2PPRNBQKB1R*4", "8575" '641b

Data "*rnbqkbnrpppppppp30P1PPPPPP1PRNBQKBNR*1", "8464", "75", "9776" '701
Data "*rnbqkb1rpppppppp5n24P1PPPPPPBPRNBQK1NR*2", "8464" '701a
Data "*rnbqkbnrppp1pppp11p18P1PPPPPPBPRNBQK1NR*2", "9776", "50", "8565" '702
Data "*rnbqkb1rppp1pppp5n5p17NP1PPPPPPBPRNBQK2R*3", "9366" '702a
Data "*rn1qkb1rppp1pppp5n5p1b15NP1PPPPPPBPRNBQ1RK*4", "8575" '702b
Data "*rn1qkb1rppp2ppp4pn5p1b13P1NP1PPP1PPBPRNBQ1RK*5", "9663", "50", "8878" ' 702c


Data "*rnbqkbnrpppppppp25P6P1PPPPPPRNBQKBNR*1", "8565", "50", "8464" '801
Data "*rnbqkbnrppp1pppp11p13P6PBPPPPPPRN1QKBNR*2", "9776", "50", "9273" '802
Data "*rnbqkbnrpppp1ppp12p12P6PBPPPPPPRN1QKBNR*2", "9273" '803
Data "*rnbqkbnrpppppppp24P8PPPPPPPRNBQKBNR*1", "8565", "50", "8464" '805
Data "*rnbqkbnrpppppppp16P16PPPPPPPRNBQKBNR*1", "8565", "50", "8464" '806
Data "*rnbqkbnrpppppppp26P5PP1PPPPPRNBQKBNR*1", "8565", "50", "8464" '807
Data "*rnbqkbnrpppppppp17P14P1PPPPPPRNBQKBNR*1", "8565", "50", "8464" '810
Data "*rnbqkbnrpppp1ppp12p4P14PBPPPPPPRN1QKBNR*2", "9652" '811
Data "*rnbqk1nrpppp1ppp12B4b14P1PPPPPPRN1QKBNR*3", "9776" '812
Data "*rnbqkbnrppp1pppp11p5P14PBPPPPPPRN1QKBNR*2", "9474", "50", "9357" '813
Data "*rn1qkbnrppp1pppp11p5P4b6N2PBPPPPPPRN1QKB1R*3", "5746" '814
Data "*rnb1kbnrppp1pppp3q7p5P6P8BPPPPPPRN1QKBNR*3", "8575" '815

Data "*rnbqkbnrpppppppp21P10PPPPP1PPRNBQKBNR*1", "8464", "75", "9776" ' 820
Data "*rnbqkbnrpppppppp29P2PPPPP1PPRNBQKBNR*1", "8565", "50", "8464" '821
Data "*rnbqkbnrpppppppp26N5PPPPPPPPR1BQKBNR*1", "8464" '830
Data "*rnbqkbnrppp1pppp11p8P5N5PPPP1PPPR1BQKBNR*2", "6455" '831

Data "*rnbqkbnrppp1pppp20N11PPPP1PPPR1BQKBNR*3", "9366" '831a

Data "*rn1qkbnrppp1pppp13b6N8Q2PPPP1PPPR1B1KBNR*4", "6677" '831b
Data "*rn1qkbnrppp1pppp13b16N1PPPP1PPPR1BQKBNR*4", "6677" '831c
DATA "*rn1qkbnrppp1pppp6b22NN1PPPP1PPPR1BQKB1R*5", "9284" ' 831d

Data "*rnbqkbnrpppppppp24N7PPPPPPPPR1BQKBNR*1", "8464", "50", "8565" '835
Data "*rnbqkbnrpppppppp31NPPPPPPPPRNBQKB1R*1", "8565", "50", "8464" '837
Data "*rnbqkbnrpppppppp22P9PPPPPP1PRNBQKBNR*1", "8565", "50", "8464" '840
Data "*rnbqkbnrppp1pppp11p10P9PPPPPPBPRNBQK1NR*2", "8373" '841
Data "*rnbqkbnrpp2pppp2p8p10P8PPPPPPPB1RNBQK1NR *3", "8565" '842
Data "*rnbqkbnrpppp1ppp12p9P9PPPPPPBPRNBQK1NR*2", "8464" '843
Data "*rnbqkbnrppp2ppp11pp5P3P9PP1PPPBPRNBQK1NR*3", "6454", "50", "6453" '844
Data "*rnbqkbnrpppppppp28P3PPPP1PPPRNBQKBNR*1", "8565", "50", "8464" '850
Data "*rnbqkbnrpppppppp27P4PPP1PPPPRNBQKBNR*1", "8565", "50", "8464" '855
Data "*rnbqkbnrpppppppp31PPPPPPPP1RNBQKBNR*1", "8565", "50", "8464" '860
Data "*rnbqkbnrpppppppp23P8PPPPPPP1RNBQKBNR*1", "8565", "50", "8464" '861

Data "END"

'////////////////////////////////////////////////////////////////////////


'White book moves
'1 - 1 d4/e4
'2 - 1 d4 d5 2 ..
'3 - 1 d4 d5 2 c4 e6 3...
'4 - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4...
'5 - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5..
'5_1- 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5 Nf3 O-O 6...
'5_2- 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5 Nf3 O-O 6 Qc2 Ndb7 7...
'5_3- 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5 Nf3 O-O 6 Qc2 Ndb7 7 cd ed 8...
'5_4- 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5 Nf3 O-O 6 Qc2 h6   7...

'5a - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5 e3 O-O 6...
'5b - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5 e3 O-O 6 Nf3 h6 7...
'5c - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Be7 5 e3 O-O 6 Nf3 h6 7 Bh4 b6 8...
'6 - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5..
'6a - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5 cd ed 6...
'6a1 - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5 cd ed 6 Nf3 Qa5 7...
'6b - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5 e3 c6 6...
'6c - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5 e3 Be7 6...
'6d - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5 e3 Be7 6 Nf3 O-O 7...
'6e - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5 e3 Bb4 6...
'6f - 1 d4 d5 2 c4 e6 3 Nc3 Nf6 4 Bg5 Nbd7 5 e3 Bb4 6 cd ed 7...

'7  - 1 d4 d5 2 c4 e6 3 Nc3 c6 4...
'7a  - 1 d4 d5 2 c4 e6 3 Nc3 c6 4 e4 de  5...
'7b  - 1 d4 d5 2 c4 e6 3 Nc3 c6 4 e4 Bb4 5...
'7c  - 1 d4 d5 2 c4 e6 3 Nc3 c6 4 e4 Bb4 5 dc ed 6...

'43a - 1 d4 d5 2 c4 e6 3 Nf3 c6 4...
'43b - 1 d4 d5 2 c4 e6 3 Nf3 c6 4 Qc2 Nf6 5...
'43c - 1 d4 d5 2 c4 e6 3 Nf3 c5 4...
'43d - 1 d4 d5 2 c4 e6 3 Nf3 c5 4 cd ed 5...

'11 - 1 d4 d5 2 Nf3 Nf6 3...
'11a - 1 d4 d5 2 Nf3 e6  3...
'11b - 1 d4 d5 2 Nf3 Nf6 3 Bg5 e6  4...
'11c - 1 d4 d5 2 Nf3 Nf6 3 Bg5 Bg4 4...
'11d - 1 d4 d5 2 Nf3 Nf6 3 Bg5 Ne4 4...
'11d1- 1 d4 d5 2 Nf3 Nf6 3 Bg5 Ne4 4 Bh4 c5 5...
'11d2- 1 d4 d5 2 Nf3 Nf6 3 Bg5 Ne4 4 Bh4 c5 5 dc Nc6 6...
'11d3- 1 d4 d5 2 Nf3 Nf6 3 Bg5 Ne4 4 Bh4 c5 5 dc Nc6 6 Nbd2 Nxc5 7...
'11d7- 1 d4 d5 2 Nf3 Nf6 3 Bg5 Ne4 4 Bh4 c6 5...
'11d8- 1 d4 d5 2 Nf3 Nf6 3 Bg5 Ne4 4 Bh4 c6 5 c3 Qb6 6...

'13a - 1 d4 d5 2 c4 c6 3...
'13b - 1 d4 d5 2 c4 c6 3 Nf3 Nf6 4...
'13c - 1 d4 d5 2 c4 c6 3 Nf3 Nf6 4 Nc3 e6 5...
'13d - 1 d4 d5 2 c4 c6 3 Nf3 Nf6 4 Nc3 e6 5 Bg5 dc 6...
'13p - 1 d4 d5 2 c4 c6 3 Nf3 Nf6 4 Nc3 dc 5..
'13q - 1 d4 d5 2 c4 c6 3 Nf3 Nf6 4 Nc3 dc 5 a4 Bf5 6..

'17 - 1 d4 f5 2..
'17a - 1 d4 f5 2 Nf3 Nf6 3...
'17b - 1 d4 f5 2 Nf3 Nf6 3 Bg5 e6 4...
'17c - 1 d4 f5 2 Nf3 Nf6 3 Bg5 e6 4 Nbd2 Be7 5...
'17d - 1 d4 f5 2 Nf3 Nf6 3 Bg5 e6 4 Nbd2 d5  5...
'17e - 1 d4 f5 2 Nf3 Nf6 3 Bg5 Ne4 4...

'b1 - 1 d4 c5 2...
'b1a - 1 d4 c5 2 d5 e5 3...
'b1b - 1 d4 c5 2 d5 e5 3 e4 d6  4...
'b2 - 1 d4 c5 2 d5 e5 3 Nc3 d6 4...
'b3 - 1 d4 c5 2 d5 Nf6 3...
'b3a - 1 d4 c5 2 d5 Nf6 3 Nc3 d6 4...

'21 - 1 d4 e6 2..

'22 - 1 d4 d6 2..
'22a- 1 d4 d6 2 e4 Nf6 3..
'22b- 1 d4 d6 2 e4 Nf6 3 Nc3 g6 4..
'22c- 1 d4 d6 2 e4 Nf6 3 Nc3 e5 4..

'23 - 1 d4 g6 2..
'23a- 1 d4 g6 2 e4 d6 3..
'23b- 1 d4 g6 2 e4 d6 3 Nc3 Bg7 4...

' 25 - 1 d4 d5 2 c4 dc 3..
'25a - 1 d4 d5 2 c4 dc 3 Nf3 Nf6 4...
'25b - 1 d4 d5 2 c4 dc 3 Nf3 Nf6 4 e3 e6 5 Bxc4
'25c - 1 d4 d5 2 c4 dc 3 Nf3 a6 4...
'25d - 1 d4 d5 2 c4 dc 3 Nf3 a6 4 e4 b5 5...
'25e - 1 d4 d5 2 c4 dc 3 Nf3 a6 4 e4 b5 5 a4 Bb7 6...
'25f - 1 d4 d5 2 c4 dc 3 Nf3 a6 4 e4 b5 5 a4 Bb7 6 ab ab 7...
'25g - 1 d4 d5 2 c4 dc 3 Nf3 a6 4 e4 b5 5 a4 Bb7 6 ab ab 7 Rxa8 Bxa8 8...
'25h - 1 d4 d5 2 c4 dc 3 Nf3 c5 4..
'25i - 1 d4 d5 2 c4 dc 3 Nf3 c5 4 d5 e6 5..
'25j - 1 d4 d5 2 c4 dc 3 Nf3 c5 4 d5 e6 5 Nc3 ed 6...
'25k - 1 d4 d5 2 c4 dc 3 Nf3 c5 4 d5 e6 5 Nc3 ed 6 Qxd4 Qxd4 7...

' 26 - 1 d4 d5 2 c4 e5 3..
'26a - 1 d4 d5 2 c4 e5 3 de d4 4...
'26b - 1 d4 d5 2 c4 e5 3 de d4 4 Nf3 Nc6 5...
'26c - 1 d4 d5 2 c4 e5 3 de d4 4 Nf3  c5 5...
'26d - 1 d4 d5 2 c4 e5 3 de d4 4 Nf3  c5 5 e3 Nc6 6...

'31 - 1 d4 Nf6 2...
'33 - 1 d4 Nf6 2 Bg5 Ne4 3...
'34 - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5
'34a - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5 Qb6 5 Nd2....
'34b - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5 Qb6 5 Nd2 Nxd2 6...
'34c - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5 Qb6 5 Nd2 Nxd2 6 Bxd2 Qxb2 7 e4 ...
'34c1- 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5 Qb6 5 Nd2 Nxd2 6 Bxd2 Qxb2 7 e4 g6  8...
'34c2- 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5 Qb6 5 Nd2 Nxd2 6 Bxd2 Qxb2 7 e4 Qe5 8...

'34d - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5 d6  5 ...
'34e - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 c5 4 d5 d6  5 f3 Nf6 6...
' 35 - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 d5 4..
'35a - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 d5 4 e3 c5 5...
'35b - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 d5 4 e3 c5 5 Bd3 Nf6 6...
' 36 - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 d6 4...
'36a - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 d6 4 Nd2 Nf6 5...
' 37 - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 e6 4...
'37a - 1 d4 Nf6 2 Bg5 Ne4 3 Bf4 e6 4 f3 Nf6 5...
' 38 - 1 d4 Nf6 2 Bg5 e6  3...
'38a - 1 d4 Nf6 2 Bg5 e6  3 e4 h6 4...
'38b - 1 d4 Nf6 2 Bg5 e6  3 e4 h6 4 Bxf6 Qxf6 5...
'38b1- 1 d4 Nf6 2 Bg5 e6  3 e4 h6 4 Bxf6 Qxf6 5 Nf3 d5 6...
'38c - 1 d4 Nf6 2 Bg5 e6  3 e4 h6 4 Bxf6 Qxf6 5 Nf3 d6 6...
'38d - 1 d4 Nf6 2 Bg5 e6  3 e4 Be7 4...
'38e - 1 d4 Nf6 2 Bg5 e6  3 e4 Be7 4 Bd3 d5 5...
'38e1- 1 d4 Nf6 2 Bg5 e6  3 e4 Be7 4 Bd3 d5 5 e5 Ne4 6...
'38e2- 1 d4 Nf6 2 Bg5 e6  3 e4 Be7 4 Bd3 d5 5 e5 Ne4 6 Bxe7 Qxe7 7...
'38e4- 1 d4 Nf6 2 Bg5 e6  3 e4 Be7 4 Bd3 d5 5 e5 Nfd7 6...
'38e5- 1 d4 Nf6 2 Bg5 e6  3 e4 Be7 4 Bd3 d5 5 e5 Nfd7 6 Bxe7 Qxe7 7...

'38f - 1 d4 Nf6 2 Bg5 e6  3 e4 Be7 4 Bd3 c5 5...
'38g - 1 d4 Nf6 2 Bg5 e6  3 e4 c5  4...
'38h - 1 d4 Nf6 2 Bg5 e6  3 e4 c5  4 d5 d6 5...
'38i - 1 d4 Nf6 2 Bg5 e6  3 e4 c5  4 d5 d6 5 Nc3 Be7 6...
' 39 - 1 d4 Nf6 2 Bg5 d5  3...
'39a - 1 d4 Nf6 2 Bg5 d5  3 Bxf6 ef 4...
'39b - 1 d4 Nf6 2 Bg5 d5  3 Bxf6 ef 4 e3 Bd6 5...
'39c - 1 d4 Nf6 2 Bg5 d5  3 Bxf6 ef 4 e3 Bd6 5 c4 dc 5 Bxc4 0-0 6....
'39d - 1 d4 Nf6 2 Bg5 d5  3 Bxf6 ef 4 e3 c6  5...
'39e - 1 d4 Nf6 2 Bg5 d5  3 Bxf6 ef 4 e3 c6  5 Bd3 Bd6 6...
'39f - 1 d4 Nf6 2 Bg5 d5  3 Bxf6 gf 4...
'39g - 1 d4 Nf6 2 Bg5 d5  3 Bxf6 gf 4 e3 c5 5...
'39h - 1 d4 Nf6 2 Bg5 c5  3...
'39i - 1 d4 Nf6 2 Bg5 c5  3 Bxf6 gf 4...
'39j - 1 d4 Nf6 2 Bg5 c5  3 d5 Qb6  4...
'39k - 1 d4 Nf6 2 Bg5 c5  3 d5 Qb6  4 Bxf6 gf 5...

'41 - 1 d4 Nf6 2 c4 e6 3..
'42 - 1 d4 Nf6 2 c4 e6 3 Nf3 d5 4...

'50 - 1 d4 Nf6 2 c4 e6 3 Nc3 Bb4 4...
'53 - 1 d4 Nf6 2 c4 e6 3 Nc3 Bb4 4 Qb3 c5 5...
'55 - 1 d4 Nf6 2 c4 e6 3 Nc3 Bb4 4 Qb3 c5 5 dc Nc6 6....

'60 - 1 d4 Nf6 2 c4 e6 3 Nc3 Bb4 4 Qc2 O-O 5...
'63 - 1 d4 Nf6 2 c4 e6 3 Nc3 Bb4 4 Qc2 c5 5...
'64 - 1 d4 Nf6 2 c4 e6 3 Nc3 Bb4 4 Qc2 c5 5 dc O-O 6...
'66 - 1 d4 Nf6 2 c4 e5 3..
'66a - 1 d4 Nf6 2 c4 e5 3 de Ng4 4..
'66b - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Bf4 Nc6 5..
'66c - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Bf4 Nc6 5 Nf3 Bb4+ 6..
'66d - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Bf4 Nc6 5 Nf3 Bb4+ 6 Nbd2 Qe7 7..
'66e - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Bf4 Bb4+ 5..
'66f - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Bf4 Bb4+ 5 Nd2 Nc6 6..
'66g - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Nf3 Bc5 5..
'66h - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Nf3 Bc5 5 e3 Nc6 6..
'66i - 1 d4 Nf6 2 c4 e5 3 de Ng4 4 Nf3 Bc5 5 e3 Nc6 6 Be2 Ngxe5 7..
'66j - 1 d4 Nf6 2 c4 e5 3 de Ne4 4..
'66k - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 Nf3 Bb4+ 5..
'66l - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 Nf3 Bb4+ 5 Nbd2 Nc6 6..
'67a - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 Nf3 Nc6 5..
'67b - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 Nf3 Nc6 5 Bf4 Bb4+ 6..
'67c - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 Nf3 Nc6 5 Bf4 Bb4+ 6 Nbd2 Qe7 7..


'66m - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 a3 Nc6 5 ..
'66n - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 a3 Nc6 5 Nf3 d6 6..
'66o - 1 d4 Nf6 2 c4 e5 3 de Ne4 4 a3 Nc6 5 Nf3 d6 6 Qc2 Bf5 7..

'75 - 1 d4 Nf6 2 c4 g6 3...
'77 - 1 d4 Nf6 2 c4 g6 3 Nc3 Bg7 4...
'78 - 1 d4 Nf6 2 c4 g6 3 Nc3 Bg7 4 e4 d6 5...
'80 - 1 d4 Nf6 2 c4 g6 3 Nc3 Bg7 4 e4 d6 5 Nf3 O-O 6...
'83 - 1 d4 Nf6 2 c4 g6 3 Nc3 Bg7 4 Bg5 O-O 5 Nf3 d6 6...
'85 - 1 d4 Nf6 2 c4 g6 3 Nc3 Bg7 4 Nf3 O-O 5 Bf4 d6 6...
'86 - 1 d4 Nf6 2 c4 g6 3 Nc3 d5  4...
'86a- 1 d4 Nf6 2 c4 g6 3 Nc3 d5  4 cd Nxd5 5...
'86b- 1 d4 Nf6 2 c4 g6 3 Nc3 d5  4 cd Nxd5 5 Bd2 Bg7 6...
'86c- 1 d4 Nf6 2 c4 g6 3 Nc3 d5  4 cd Nxd5 5 Bd2 Bg7 6 e4 Nb6 7...
'86d- 1 d4 Nf6 2 c4 g6 3 Nc3 d5  4 cd Nxd5 5 Bd2 Nb6 6...

'88  - 1 d4 Nf6 2 c4 c5 3...
'88a - 1 d4 Nf6 2 c4 c5 3 d5 b5 4...
'88b - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 g6 5...
'88c - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 g6 5 Qc2 bc 6...
'88d - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 g6 5 Qc2 Bg7 6...

'88g - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 Bb7 5...
'88h - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 Bb7 5 Qc2 bc 6...
'88i - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 Bb7 5 Qc2 bc 6 e4 e6 7...
'88k - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 bc 5...
'88l - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 bc 5 Nc3 d6 6...
'88m - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 d6 5...
'88n - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 d6 5 bc a6 6...
'88o - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 d6 5 bc a6 6 e3 g6 7...
'88p - 1 d4 Nf6 2 c4 c5 3 d5 b5 4 Nf3 d6 5 bc a6 6 e3 g6 7 Nc3 Bg7 8...


'88t - 1 d4 Nf6 2 c4 c5 3 d5 e6 4...
'88u - 1 d4 Nf6 2 c4 c5 3 d5 e6 4 Nf3 ed 5...
'88v - 1 d4 Nf6 2 c4 c5 3 d5 e6 4 Nf3 ed 5 cd d6 6...
'88w - 1 d4 Nf6 2 c4 c5 3 d5 e6 4 Nf3 ed 5 cd d6 6 Nc3 g6 7...

'101 - 1 e4 e5 2..
'102 - 1 e4 e5 2 Bc4 Nf6 3...
'103 - 1 e4 e5 2 Bc4 Nf6 3 d3 Nc6 4...
'104 - 1 e4 e5 2 Bc4 Nf6 3 d3 Nc6 4 Nf3 Be7 5...
'105 - 1 e4 e5 2 Bc4 Nf6 3 d3 Nc6 4 Nf3 Be7 5 O-O O-O 6...
'106 - 1 e4 e5 2 Bc4 Nf6 3 d3 Nc6 4 Nf3 Bc5 5...
'107 - 1 e4 e5 2 Bc4 Nf6 3 d3 Nc6 4 Nc3 Bb4 5...
'108 - 1 e4 e5 2 Bc4 Nf6 3 d3 c6  4...
'109 - 1 e4 e5 2 Bc4 Nf6 3 d3 c6  4 Nf3 d5 5..
'109a- 1 e4 e5 2 Bc4 Nf6 3 d3 c6  4 Nf3 d5 5 Bb3 de 6...

'110 - 1 e4 e5 2 Bc4 Nf6 3 d3 c6  4 Nf3 Be7 5..

'110a- 1 e4 e5 2 Nf3 d6 3...
'110b- 1 e4 e5 2 Nf3 d6 3 d4 ed 4...
'110c- 1 e4 e5 2 Nf3 d6 3 d4 ed 4 Nxd4 Nf6 5...
'110d- 1 e4 e5 2 Nf3 d6 3 d4 ed 4 Nxd4 Nf6 5 Nc3 Be7 6...

'110k- 1 e4 e5 2 Nf3 d6 3 d4 Nf6 4...
'110l- 1 e4 e5 2 Nf3 d6 3 d4 Nf6 4 de Nxe4 5...
'110m- 1 e4 e5 2 Nf3 d6 3 d4 Nf6 4 de Nxe4 5 Qd5 Nc5 6...

'111 - 1 e4 e5 2 Nf3 Nc6 3...
'111a- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Be7 4...
'111b- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Be7 4 d4 d6 5...
'111c- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Be7 4 d4 d6 5 de de 6...  
'111d- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Be7 4 d4 d6 5 de de 6 Qxd8+ Bxd8 7...

'111t- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Be7 4 d4 ed 5...
'111u- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Be7 4 d4 ed 5 c3 d6 6...

'112 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4...
'114 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3 Be7 5...
'115 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3 Bc5 5 O-O d6 6...
'116 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3 Be7 5 O-O O-O 6..
'116c- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3 Be7 5 O-O d6 6..

'117 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4...
'118 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5..
'119 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5 d3 d6 6...
'120 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5 d3 d6 6 O-O O-O 7...
'121 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Qe7 5..
'122 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 d6 5..
'123 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5 d3 a6 6...
'124 - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5 d3 a6 6 O-O d6 7...
'124_1- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5 d3 a6 6 O-O d6 7 Bb3 Ba7 8...
'124_2- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5 d3 a6 6 Bb3 Ba7 7...
'124_3- 1 e4 e5 2 Nf3 Nc6 3 Bc4 Bc5 4 c3 Nf6 5 d3 a6 6 O-O Ba7 7...

'124a- 1 e4 e5 2 Nf3 Nf6 3..
'124b- 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4..
'124c- 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4 Bd3 d5 5..
'124f- 1 e4 e5 2 Nf3 Nf6 3 d4 ed 4..
'124g- 1 e4 e5 2 Nf3 Nf6 3 d4 ed 4 e5 Ne4 5..
'124h- 1 e4 e5 2 Nf3 Nf6 3 d4 ed 4 Bc4 Nxe4 5..
'124i- 1 e4 e5 2 Nf3 Nf6 3 d4 ed 4 Bc4 Nxe4 5 Qxd4 Nf6 6..

'125 - 1 e4 c5 2..
'131 - 1 e4 c5 2 Nf3 d6 3...
'132 - 1 e4 c5 2 Nf3 d6 3 Bb5 Bd7 4...
'133 - 1 e4 c5 2 Nf3 d6 3 Bb5 Bd7 4 Bd7 Qd7 5...
'134 - 1 e4 c5 2 Nf3 d6 3 Bb5 Bd7 4 Bd7 Qd7 5 O-O Nf6 6...
'135 - 1 e4 c5 2 Nf3 d6 3 Bb5 Bd7 4 Bd7 Qd7 5 O-O Nf6 6 e5 de 7...

'137 - 1 e4 c5 2 Nf3 d6 3 Bb5 Bd7 4 Bd7 Nd7 5...
'138 - 1 e4 c5 2 Nf3 d6 3 Bb5 Bd7 4 Bd7 Nd7 5 O-O Ngf6 6...
'139 - 1 e4 c5 2 Nf3 d6 3 Bb5 Bd7 4.Bd7 Nd7 5 O-O Ngf6 6 d3 e6 7...
'139a- 1 e4 c5 2 Nf3 e6 3...
'139b- 1 e4 c5 2 Nf3 e6 3 Nc3 a6 4...
'139c- 1 e4 c5 2 Nf3 e6 3 Nc3 a6 4 d4 cd 5...
'139d- 1 e4 c5 2 Nf3 e6 3 Nc3 a6 4 d4 cd 5 Nxd4 Qc7 6...
'139e- 1 e4 c5 2 Nf3 e6 3 Nc3 Nc6 4 ...
'139f- 1 e4 c5 2 Nf3 e6 3 Nc3 Nc6 4 d4 cd 5...
'139g- 1 e4 c5 2 Nf3 e6 3 Nc3 Nc6 4 d4 cd 5 Nxd4 Qc7 6...
'139h- 1 e4 c5 2 Nf3 e6 3 Nc3 Nc6 4 Bg5 Nge7 5...
'139j- 1 e4 c5 2 Nf3 e6 3 Nc3 Nc6 4 Bg5 Nge7 5 O-O a6 6...

'140 - 1 e4 c5 2 Nf3 Nc6 3...
'145 - 1 e4 c5 2 Nf3 Nc6 3 Nc3 e6 4...

'150 - 1 e4 c5 2 Nf3 Nc6 3 Nc3 d6 4...
'156 - 1 e4 c5 2 Nf3 Nc6 3 Nc3 d6 4 Bb5 Bd7 5 O-O Nf6 6...

'180 - 1 e4 c5 2 Nc3 Nc6 3...
'181 - 1 e4 c5 2 Nc3 d6 3..
'181a- 1 e4 c5 2 Nc3 d6 3 Nf3 Nf6 4...
'181b- 1 e4 c5 2 Nc3 d6 3 Nf3 Nf6 4 d4 cd 5...
'181c- 1 e4 c5 2 Nc3 d6 3 Nf3 Nf6 4 d4 cd 5 Qxd4 Nc6 6...

'200 - 1 e4 c6...
'220 - 1 e4 c6 2 Nc3 d5 3...
'222 - 1 e4 c6 2 Nc3 d5 3 Nf3 Bg4
'223 - 1 e4 c6 2 Nc3 d5 3 Nf3 Bg4 4 h3 Bf3 5 Qf3 e6 6..
'224 - 1 e4 c6 2 Nc3 d5 3 Nf3 Bg4 4 Be2 e6 5...
'225 - 1 e4 c6 2 Nc3 d5 3 Nf3 Bg4 4 Be2 e6 5 O-O Nf6 6...

'227 - 1 e4 c6 2 Nc3 d5 3 d4 de 4...
'228 - 1 e4 c6 2 Nc3 d5 3 d4 de 4 Nc3 Bf5 5...
'229 - 1 e4 c6 2 Nc3 d5 3

'230 - 1 e4 c6 2 c4 d5 3..
'231 - 1 e4 c6 2 c4 d5 3 ed cd 4...
'232 - 1 e4 c6 2 c4 d5 3 cd cd 4...
'234 - 1 e4 c6 2 c4 d5 3 cd cd 4 ed Nf6 5..
'236 - 1 e4 c6 2 c4 d5 3 cd cd 4 ed Nf6 5 Nc3 Nd5 6...
'238 - 1 e4 c6 2 c4 d5 3 cd cd 4 ed Nf6 5 Nc3 Nd5 6 Nf3 Nc6 7....

'245 - 1 e4 c6 2 c4 d5 3 cd cd 4 ed Nf6 5 Bb5 Nd7 6...

'300 - 1 e4 Nf6 2..
'301 - 1 e4 Nf6 2 e5 Nd5 3..
'303 - 1 e4 Nf6 2 e5 Nd5 3 Nf3 d6 4..
'305 - 1 e4 Nf6 2 e5 Nd5 3 d4 d6 4..
'307 - 1 e4 Nf6 2 e5 Nd5 3 d4 d6 4 Nf3 Bg4 5...

'320 - 1 e4 e6 2...
'322 - 1 e4 e6 2 d4 d5 3....
'322a- 1 e4 e6 2 d4 d5 3 Nc3 Nf6 4...
'322b- 1 e4 e6 2 d4 d5 3 Nc3 Nf6 4 Bg5 Be7 5...
'322c- 1 e4 e6 2 d4 d5 3 Nc3 Nf6 4 Bg5 Be7 5 e5 Nfd7 6...
'322d- 1 e4 e6 2 d4 d5 3 Nc3 Nf6 4 Bg5 Be7 5 e5 Nfd7 6 Bxe7 Qxe7 7...

'325 - 1 e4 e6 2 d4 d5 3 Nc3 Bb4 4...
'330 - 1 e4 e6 2 d4 d5 3 Nc3 Bb4 4 ed ed 5....
'331 - 1 e4 e6 2 d4 d5 3 Nc3 Bb4 4 ed ed 5 Bd3 Nc6 6....

'335 - 1 e4 e6 2 d4 d5 3 Nc3 Bb4 4 e5 c5 5 ...
'337 - 1 e4 e6 2 d4 d5 3 Nc3 Bb4 4 e5 c5 5 a3 Ba5 6...
'338 - 1 e4 e6 2 d4 d5 3 Nc3 Bb4 4 e5 c5 5 a3 Ba5 6 b4 cd 7...
'339a- 1 e4 e6 2 d4 d5 3 Nc3 Nc6 4...
'339b- 1 e4 e6 2 d4 d5 3 Nc3 Nc6 4 e5 f6 5...
'340 - 1 e4 e6 2 d4 d5 3 e5 c5 4...
'340a- 1 e4 e6 2 d4 d5 3 e5 c5 4 c3 Nc6 5...
'340b- 1 e4 e6 2 d4 d5 3 e5 c5 4 c3 Nc6 5 Nf3 Qb6 6...
'340c- 1 e4 e6 2 d4 d5 3 e5 c5 4 c3 Nc6 5 Nf3 Bd7 6...

'350 - 1 e4 Nc6 2..
'351 - 1 e4 Nc6 2 Nf3 d6 3...
'353 - 1 e4 Nc6 2 Nf3 d6 3 d4 Nf6 4...
'354 - 1 e4 Nc6 2 Nf3 d6 3 d4 Nf6 4 Nc3 Bg4 5...

'360 - 1 e4 d5 2...
'362 - 1 e4 d5 2 ed Qd5 3...
'364 - 1 e4 d5 2 ed Qd5 3 Nc3 Qa5 4....
'366 - 1 e4 d5 2 ed Qd5 3 Nc3 Qa5 4 d4 Nf6 5...
'368 - 1 e4 d5 2 ed Qd5 3 Nc3 Qa5 4 Nf3 Nf6 5...
'370 - 1 e4 d5 2 Nc3 d4 3..
'371 - 1 e4 d5 2 Nc3 d4 3 Nce2 e5 4..

'390 - 1 e4 b6 2..
'391 - 1 e4 b6 2 d4 Bb7 3...
'392 - 1 e4 b6 2 d4 Bb7 3 Bd3 e6 4...
'393 - 1 e4 b6 2 d4 Bb7 3 Bd3 e6 4 Nf3 c5 5...
'394 - 1 e4 b6 2 d4 Bb7 3 Bd3 e6 4 Nf3 c5 5 c3 Nf6 6...

'395 - 1 e4 g6 2..
'396 - 1 e4 g6 2 d4 Bg7 3 ..
'397 - 1 e4 g6 2 d4 Bg7 3 Nc3 d6 4...
'397a- 1 e4 g6 2 d4 Bg7 3 Nc3 d6 4 Nf3 Nf6 5...
'397b- 1 e4 g6 2 d4 Bg7 3 Nc3 d6 4 Nf3 Nf6 5 Be2 O-O 6...
'397c- 1 e4 g6 2 d4 Bg7 3 Nf3 d6 4...

'//////////////////////////////////////////////////////////////////////

'Black book moves

'1 - ' 1 d4...
'3 - ' 1 d4 d5  2 c4...
'5 - ' 1 d4 d5  2 c4 e6 3 Nc3...
'5a- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 e3...
'5b- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 e3 Nf6 5 Nf3...
'5c- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 e3 Nf6 5 Nf3 Nbd7 6 Bd3...
'5d- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 e3 Nf6 5 Nf3 Nbd7 6 Bd3 dc 7 Bxc4...
'5m- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 e3 Nf6 5 Nf3 Nbd7 6 Qc2...
'5n- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 e3 Nf6 5 Nf3 Nbd7 6 Qc2 Bd6 7 Bd3...
'5t- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 Nf3...
'5u- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 Nf3 dc 5 a4...
'5v- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 Nf3 dc 5 a4 Bb4 6 e3...
'5w- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 Nf3 dc 5 e3...
'5x- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 Nf3 dc 5 e3 b5 6 a4...
'5z- ' 1 d4 d5  2 c4 e6 3 Nc3 c6 4 Nf3 dc 5 e4...

'8 - ' 1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 Bg5...
'15 -  1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 Bg5 Be7 5 e3...
'18 -  1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 Bg5 Be7 5 e3 0-0  6 Nf3...

'19 -  1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 Bg5 Be7 5 e3 Nbd7 6 Nf3...
'19a -  1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 Nf3 Be7 5 Bg5 ...

'29 -  1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 cd ...
'29a -  1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 cd ed  5 Bg5 ...
'29b -  1 d4 d5  2 c4 e6 3 Nc3 Nf6  4 cd ed  5 Bg5 Be7 6 e3....

'16 -  1 d4 d5  2 c4 e6 3 Nf3...
'16a-  1 d4 d5  2 c4 e6 3 Nf3 c6 4 e3...
'16b-  1 d4 d5  2 c4 e6 3 Nf3 c6 4 e3 Bd6 5 Bd3...
'16h-  1 d4 d5  2 c4 e6 3 Nf3 c6 4 Nc3...
'16i-  1 d4 d5  2 c4 e6 3 Nf3 c6 4 Nc3 dc 5 a4...
'16p-  1 d4 d5  2 c4 e6 3 Nf3 c6 4 Qc2...

'9 - ' 1 d4 d5  2 c4 dc 3 e4....
'9a- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5...
'9b- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5 Nf6 5 Nc3...
'9c- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5 Nf6 5 Nc3 b5 6 e5...
'9d- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5 Nf6 5 Nc3 b5 6 e5 b4 7 ef...
'9e- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5 Nf6 5 Nc3 b5 6 e5 b4 7 ef bc 8 bc...
'9m- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5 Nf6 5 Nc3 b5 6 Bf4...
'9n- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5 Nf6 5 Nc3 b5 6 Bf4 a6 7 e5...
'9o- ' 1 d4 d5  2 c4 dc 3 e4 c5 4 d5 Nf6 5 Nc3 b5 6 Bf4 a6 7 e5 b4 8 ef....

'12 -  1 d4 d5  2 c4 dc 3 e4 e5 4 Nf3....
'22 -  1 d4 d5  2 c4 dc 3 e4 e5  4 Nf3 Bb4+ 5 Bd2...

'27 -  1 d4 d5  2 c4 dc 3 Nf3...
'27a -  1 d4 d5  2 c4 dc 3 Nf3 Nf6 4 e3 ...
'27b -  1 d4 d5  2 c4 dc 3 Nf3 Nf6 4 e3 e6 5 Bxc4 ....
'27c -  1 d4 d5  2 c4 dc 3 Nf3 Nf6 4 e3 e6 5 Bxc4 a6 6 O-O ...
'28 -  1 d4 d5  2 c4 dc 3 e3....
'28a -  1 d4 d5  2 c4 dc 3 e3 Nf6  4 Bxc4 ....

'290a -  1 d4 d5  2 e4...
'290b -  1 d4 d5  2 e4 ed 3 Nc3 ...
'290c -  1 d4 d5  2 e4 ed 3 Nc3 e5 4 Nxd4 ...
'290d -  1 d4 d5  2 e4 ed 3 Nc3 e5 4 Qh5 ...

'293a -  1 d4 d5 2 Nf3 Nf6 3 e3...
'293b -  1 d4 d5 2 Nf3 Nf6 3 e3 c5 4 c3...

'2 - ' 1 d4 Nf6 2 c4...
'2a -   1 d4 Nf6 2 c4 e5 3 d5...
'2b -   1 d4 Nf6 2 c4 e5 3 d5 Bc5 4 Nc3...

'4a -   1 d4 Nf6 2 c4 e5 3 e4...
'4b -   1 d4 Nf6 2 c4 e5 3 e4 Nxe5 4 f4 ...
'4 - ' 1 d4 Nf6 2 c4 e5 3 de...
'10a -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Bf4...
'10b -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Bf4 Nc6 5 Nf3...
'10c -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Bf4 Nc6 5 Nf3 Bb4+ 6 Nbd2 ...
'10d -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Bf4 Nc6 5 Nf3 Bb4+ 6 Nbd2 Qe7 7 e3 ...
'10e -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Bf4 Nc6 5 Nf3 Bb4+ 6 Nbd2 Qe7 7 a3 ...

'10 -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3...
'20d -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Bc5 5 e3 ...

'20 - 1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Nc3...
'20_1 - 1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Nc3 Nge5 6 Nxe5 ...
'20_2 - 1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Nc3 Nge5 6 Nxe5 Nxe5 7 Qd5 ...
'20_3 - 1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Nc3 Nge5 6 Nxe5 Nxe5 7 Qa4 ...
'20_4 - 1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Nc3 Nge5 6 Nxe5 Nxe5 7 e3 ...
'20_5 - 1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Nc3 Nge5 6 Nxe5 Nxe5 7 Qd4 ...

'20a -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bf4...
'20b -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bf4 Bb4+ 6 Nbd2 ...
'20b1-  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bf4 Bb4+ 6 Nc3 ...
'20b2-  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bf4 Bb4+ 6 Nc3 Qe7 7 Qd4...
'20b3-  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bf4 Bb4+ 6 Nc3 Qe7 7 Qd4 f6 8 ef...
'20b4-  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bf4 Bb4+ 6 Nc3 Qe7 7 Qd4 Bxc3+ 8 bc...

'20c -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 e3 ...
'20x -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bg5 ...
'20y -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Nf3 Nc6 5 Bg5 Be7 6 Bf4...

'20f -  1 d4 Nf6 2 c4 e5 3 de Ng4  4 Qd4..

'6 - ' 1 d4 Nf6 2 c4 e6 3 Nc3 ....
'7 - ' 1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Qc2...
'7a -   1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Qc2 O-O 5 a3 ...
'7b -   1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Qc2 O-O 5 a3 Bxc3 6 Qxc2 ...
'7c -   1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Qc2 O-O 5 a3 Bxc3 6 Qxc2 b6 7 Bg5 ...
'7d -   1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Qc2 O-O 5 a3 Bxc3 6 Qxc2 b6 7 Nf3 ...
'7e -   1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Qc2 O-O 5 a3 Bxc3 6 Qxc2 b6 7
'7f -   1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Qc2 O-O 5 a3 Bxc3 6 Qxc2 b6 7

'14 -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 e3....
'14a -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 e3 O-O 5 Bd3 ...
'14b -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 e3 O-O 5 Bd3 d5 6 Nf3 ...
'14c -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 e3 O-O 5 Nge2 ...
'14d -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 e3 O-O 5 Nge2 d5 6 a3 ...
'14e -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3....
'14f -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 Bg5 ...
'14g -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 Bg5 dc 6 e4 ...
'14h -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 cd ...
'14i -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 cd ed 6 Bg5 ...
'14h -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 cd ...
'14j -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 e3 ...
'14k -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 e3 c5 6 Bd3 ...
'14l -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 Qa4+ ...
'14m -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 Nf3 d5 5 Qa4+ Nc6 6 Ne5 ...

'17 -  1 d4 Nf6 2 c4 e6 3 Nc3 Bb4  4 e3 d5   5 Nf3...

' 11 -  1 d4 Nf6 2 c4 e6 3 Nf3...
'13 -  1 d4 Nf6 2 c4 e6 3 Nf3 Bb4+ 4 Bd2....
'13a -  1 d4 Nf6 2 c4 e6 3 Nf3 Bb4+ 4 Bd2 Qe7 5 g3 ...
'13b -  1 d4 Nf6 2 c4 e6 3 Nf3 Bb4+ 4 Bd2 Qe7 5 g3 Nc6 6 Bg2 ....
'13c -  1 d4 Nf6 2 c4 e6 3 Nf3 Bb4+ 4 Nbd2....
'13d -  1 d4 Nf6 2 c4 e6 3 Nf3 Bb4+ 4 Nbd2 b6 5 a3 ...
'13e -  1 d4 Nf6 2 c4 e6 3 Nf3 Bb4+ 4 Nbd2 b6 5 a3 Bxd2 6 Bxd2 ...
'13f -  1 d4 Nf6 2 c4 e6 3 Nf3 Bb4+ 4 Nbd2 b6 5 a3 Bxd2 6 Qxd2 ...

'21 -  1 d4 Nf6 2 Nf3...
'23 -  1 d4 Nf6 2 Nf3 e6  3 c4 d5 4 Nc3.....
'23a -  1 d4 Nf6 2 Nf3 e6  3 Bg5...

'24 -  1 d4 Nf6 2 Bg5...
'24a -  1 d4 Nf6 2 Bg5 d5 3 Bxf6 ...
'24b -  1 d4 Nf6 2 Bg5 d5 3 Bxf6 ef 4 e3...
'24c -  1 d4 Nf6 2 Bg5 d5 3 Bxf6 ef 4 e3 Bd6 5 c4...
'25 -  1 d4 Nf6 2 Bg5 d5 3 e3...
'25a -  1 d4 Nf6 2 Bg5 d5 3 e3 c5 4 Bxf6...
'25b -  1 d4 Nf6 2 Bg5 d5 3 e3 c5 4 Bxf6 gf 5 c3...
'25c -  1 d4 Nf6 2 Bg5 d5 3 e3 c5 4 c3..
'25d -  1 d4 Nf6 2 Bg5 d5 3 Nd2...
'26  -  1 d4 Nf6 2 Bg5 c5 3 Bxf6...
'26a -  1 d4 Nf6 2 Bg5 c5 3 Bxf6 gf 4 d5...
'26b -  1 d4 Nf6 2 Bg5 c5 3 Bxf6 gf 4 d5 Qb6 5 Qc1...
'26c -  1 d4 Nf6 2 Bg5 c5 3 d5...
'26d -  1 d4 Nf6 2 Bg5 c5 3 d5 Ne4 4 Bf4...
'26e -  1 d4 Nf6 2 Bg5 c5 3 d5 Ne4 4 Bf4 d6 5 f3...
'26d -  1 d4 Nf6 2 Bg5 c5 3 d5 Ne4 4 h4...

'201 - 1 c4...
'202 - 1 c4 e6  2 d4...
'203 - 1 c4 Nf6 2 Nc3 ...

'203p- 1 c4 Nf6 2 g3...
'203q- 1 c4 Nf6 2 g3 c6 3 Bg2...
'203r- 1 c4 Nf6 2 g3 c6 3 Bg2 d5 4 Nf3...
'203s- 1 c4 Nf6 2 g3 c6 3 Bg2 d5 4 cd...

'203v- 1 c4 Nf6 2 g3 c6 3 Nf3...
'203w- 1 c4 Nf6 2 g3 c6 3 Nf3 d5 4 Bg2 dc 5 O-O...

'204a- 1 c4 e6  2 g3 ...
'204b- 1 c4 e6  2 g3 d5 3 Bg2...
'204c- 1 c4 e6  2 g3 d5 3 Bg2 Nf6 4 Nf3...
'205a- 1 c4 Nf6 2 Nf3 e6 3 Nc3...
'206 - 1 c4 e6  2 Nc3...
'206a- 1 c4 e6  2 Nc3 d5 3 cd...
'207a- 1 c4 e6  2 Nf3...
'207b- 1 c4 e6  2 Nf3 Nf6 3 g3...

'301 - 1 Nf3...
'302 - 1 Nf3 d5 2 d4 ....
'303 - 1 Nf3 d5 2 d4 Nf6 3 c4 ...
'304 - 1 Nf3 d5 2 c4...

'310 - 1 Nf3 d5 2 g3...
'310a- 1 Nf3 d5 2 g3 Bg4 3 Bg2 ...
'310b- 1 Nf3 d5 2 g3 Bg4 3 Bg2 e6 4 O-O ...
'310g- 1 Nf3 d5 2 g3 Bg4 3 Ne5...
'310h- 1 Nf3 d5 2 g3 Bg4 3 Ne5 Bf5 4 Bg2...

'351 - 1 Nf3 Nf6 2 c4...

'352a - 1 Nf3 Nf6 2 g3...
'352b - 1 Nf3 Nf6 2 g3 d5 3 Bg2...
'352c - 1 Nf3 Nf6 2 g3 d5 3 Bg2 c6 4 O-O...
'352d - 1 Nf3 Nf6 2 g3 d5 3 Bg2 c6 4 O-O Bg4 5 d3...

'501  - 1 e4...
'502  - 1 e4 e5 2 Nf3....
'503  - 1 e4 e5 2 Nf3 Nf6 3 Nxe5...
'504  - 1 e4 e5 2 Nf3 Nf6 3 Nxe5 d6 4 Nf3...
'504a - 1 e4 e5 2 Nf3 Nf6 3 Nxe5 d6 4 Nf3 Nxe4 5 d4...
'504b - 1 e4 e5 2 Nf3 Nf6 3 Nxe5 d6 4 Nf3 Nxe4 5 d4 d5 6 Bd3...

'504c - 1 e4 e5 2 Nf3 Nf6 3 Nxe5 d6 4 Nf3 Nxe4 5 Nc3...
'504d - 1 e4 e5 2 Nf3 Nf6 3 Nxe5 d6 4 Nf3 Nxe4 5 Nc3 Nxc3 6 dc...
'504e - 1 e4 e5 2 Nf3 Nf6 3 Nxe5 d6 4 Nf3 Nxe4 5 Nc3 Nxc3 6 dc Nc6 7 Be3...
'504f - 1 e4 e5 2 Nf3 Nf6 3 Nxe5 d6 4 Nf3 Nxe4 5 Nc3 Nxc3 6 dc Be7 7 Bf4...

'510  - 1 e4 e5 2 Nf3 Nf6 3 d4...
'510a - 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4 Bd3...
'510a1 - 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4 Bd3 d5 5 Nxe5...
'510a2 - 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4 Bd3 d5 5 Nxe5 Nbd7 6 Nx5...
'510a6 - 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4 Bd3 d5 5 de...

'510t - 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4 de...
'510u - 1 e4 e5 2 Nf3 Nf6 3 d4 Nxe4 4 de d5 5 Nbd2...

'510b - 1 e4 e5 2 Nf3 Nf6 3 Bc4...
'510c - 1 e4 e5 2 Nf3 Nf6 3 Nc3...
'510d - 1 e4 e5 2 Nf3 Nf6 3 Nc3 Nc6 4 Bb5...
'511  - 1 e4 e5 2 Nf3 Nf6 3 Nc3 Nc6 4 d4...
'511a - 1 e4 e5 2 Nf3 Nf6 3 Nc3 Nc6 4 d4 ed 5 Nxd4...
'511b - 1 e4 e5 2 Nf3 Nf6 3 Nc3 Nc6 4 d4 ed 5 Nxd4 Bb4 6 Nxc6...

'513  - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d4...
'513a - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d4 ed 5 O-O Nxe4 6 Re1 d5 7 Bxd5...
'513b - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d4 ed 5 O-O Nxe4 6 Re1 d5 7 Bxd5 Qxd5 8 Nc3...

'530  - 1 e4 e5 2 Nf3 Nc6 3 Bc4..
'530a - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3...
'530b - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3 Be7 5 O-O...
'530c - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3 Be7 5 O-O O-O 6 Re1...
'530h - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 d3 Be7 5 O-O O-O 6 Bb3...

'531  - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5...
'531a - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 Bc5 5 Nxf7...
'531b - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 Bc5 5 Bxf7...
'531c - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 Bc5 5 Bxf7 Ke7 6 Bd5...
'531d - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 Bc5 5 Bxf7 Ke7 6 Bd5 Rf8 7 0-0...
'531o  - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 d5 5 ed...
'531p  - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 d5 5 ed b5 6 Bf1...
'531r  - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 d5 5 ed b5 6 Bxb5...
'531t  - 1 e4 e5 2 Nf3 Nc6 3 Bc4 Nf6 4 Ng5 d5 5 ed b5 6 dc...

'550  - 1 e4 e5 2 Nf3 Nc6 3 Bb5...
'550a - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Bxc6...
'550b - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Bxc6 dc 5 0-0...
'550c - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Bxc6 dc 5 0-0 Bd6 6 d4...

'551  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4..
'551a - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 d6 5 c3...
'551b - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 d6 5 c3 f5 6 ef...
'551c - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 d6 5 0-0...
'551d - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 d6 5 0-0 Nf6 6 Re1...

'551g - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 d4...
'551h - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 d4 ed 6 0-0...
'551i - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 d3...
'551j - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 d3 d6 6 c3..
'551k - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 d3 d6 6 c3 Be7 7 Bg5...

'551l - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 d3 d6 6 c4...
'551m - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 d3 d6 6 c4 Be7 7 Nc3...

'552  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0...

'553  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Nxe4 6 d4...
'553a - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Nxe4 6 d4 ed 7 Re1...
'553b - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Nxe4 6 d4 b5 7 Bb3...
'553c - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Nxe4 6 d4 b5 7 Bb3 d5 8 de...

'553f - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Be7  6 Bxc6...

'554  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Be7 6 Re1...
'555  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Be7 6 Re1 b5 7 Bb3...
'556  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Be7 6 Re1 b5 7 Bb3 0-0 8 c3...
'557  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 Nf6 5 0-0 Be7 6 Re1 b5 7 Bb3 d6  8 c3...

'560  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 d6 5 c3...
'561  - 1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 d6 5 c3 Bd7 6 0-0...

'568  - 1 e4 e5 2 Nf3 Nc6 3 d4...
'568a - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4...
'568b - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3...
'568c - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Nxc6...
'568d - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Nxc6 bc 7 Bd3...
'568e - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Nxc6 bc 7 Nd2...

'568s - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Be3...
'568t - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Be3 Bb6 7 Nf5...
'568u - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Be3 Bb6 7 Nf5 Bxe3 8 Nxe3...
'568v - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Be3 Bb6 7 Qg4...
'568w - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Bc5 6 Be3 Bb6 7 Bc4...

'568f - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Nf6 5 Nxc6 ..
'568g - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Nf6 5 Nxc6 bc 6 e5 ...
'568i - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Nf6 5 Nxc6 bc 6 Bd3 ...
'568h - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Nf6 5 Nc3...
'568j - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Nf6 5 Nc3 Bb4 6 Nxc3 ...
'568k - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Be7 6 Bc4...
'568k1- 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Be7 6 Bc4 d6 7 Nxc6...
'568k7- 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Be7 6 Be3...

'568l - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Be7 6 Nxc6...
'568m - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Be7 6 Nxc6 bc 7 Bc4...
'568n - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Nxd4 Bb4+ 5 c3 Be7 6 Nxc6 bc 7 Bc4 Nf6 8 e5...

'568r1 - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Bc4...
'568s1 - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Bc4 Nf6 5 O-O...
'568t1 - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Bc4 Nf6 5 O-O Nxe4 6 Re1...d5
'568v1 - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Bc4 Nf6 5 e5...Ng4/Ne4
'568x1 - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Bc4 Nf6 5 Ng5..Ne4/d5
'568x2 - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Bc4 Nf6 5 Ng5 Ne4 6 Bb3...
'568y1 - 1 e4 e5 2 Nf3 Nc6 3 d4 ed 4 Bc4 Nf6 5 Ng5 d5 6 ed...Qe7+

'569  - 1 e4 e5 2 Nf3 Nc6 3 Nc3...

'570  - 1 e4 e5 2 d4...
'570a - 1 e4 e5 2 d4 ed 3 Qxd4...

'573  - 1 e4 e5 2 Bc4...
'573a - 1 e4 e5 2 Bc4 Nf6 3 d3...
'573b - 1 e4 e5 2 Bc4 Nf6 3 d3 Bc5 4 Nf3...
'573b1- 1 e4 e5 2 Bc4 Nf6 3 d3 Bc5 4 Nf3 O-O 5 Nxe5...
'573b2- 1 e4 e5 2 Bc4 Nf6 3 d3 Bc5 4 Nf3 O-O 5 O-O...

'573c - 1 e4 e5 2 Bc4 Nf6 3 d3 Bc5 4 Nc3...
'573c1- 1 e4 e5 2 Bc4 Nf6 3 d3 Bc5 4 Nc3 c6 5 Nf3...
'573c2- 1 e4 e5 2 Bc4 Nf6 3 d3 Bc5 4 Nc3 c6 5 Nf3 d5 6 ed...
'573c3- 1 e4 e5 2 Bc4 Nf6 3 d3 Bc5 4 Nc3 c6 5 Nf3 d5 6 ed cd 7 Bb5+...


'573d - 1 e4 e5 2 Bc4 Nf6 3 d4...
'573e - 1 e4 e5 2 Bc4 Nf6 3 d4 ed 4 Nf3...
'573f - 1 e4 e5 2 Bc4 Nf6 3 Nc3...

'591  - 1 e4 e5 2 f4...
'591a - 1 e4 e5 2 f4 Bc5 3 Nf3...
'591g - 1 e4 e5 2 f4 Bc5 3 Nf3 d6 4 c3...
'591h - 1 e4 e5 2 f4 Bc5 3 Nf3 d6 4 c3 Nf6 5 d4...


'591v - 1 e4 e5 2 f4 Bc5 3 Nf3 d5 4 Nxe5...
'591w - 1 e4 e5 2 f4 Bc5 3 Nf3 d5 4 ed...

'591b - 1 e4 e5 2 f4 Bc5 3 Nc3...
'591d - 1 e4 e5 2 f4 Bc5 3 Nc3 d6 4 Nf3...
'592  - 1 e4 e5 2 Nc3...
'592a - 1 e4 e5 2 Nc3 Bc5 3 Nf3...
'592b - 1 e4 e5 2 Nc3 Bc5 3 Nf3 d6 4 d4...
'592c - 1 e4 e5 2 Nc3 Bc5 3 Nf3 d6 4 d4 ed 5 Nxd4...
'593  - 1 e4 e5 2 Nc3 Bc5 3 Bc4...
'593a - 1 e4 e5 2 Nc3 Bc5 3 Bc4 d6 4 d3...

'601  - 1 e4 c6 2 d4..
'602  - 1 e4 c6 2 d4 d5 3 Nc3...
'603  - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4....
'604  - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3...
'605  - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 h4...
'607  - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 h4 h6 7 h5...
'607a - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 h4 h6 7 Nf3...
'607b - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 h4 h6 7 Nf3 Nf6 8 Ne5...
'607c - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 h4 h6 7 Nf3 Nf6 8 h5...
'607d - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 h4 h6 7 Nf3 e6 8 Ne5...
'607e - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 h4 h6 7 Nf3 e6 8 h5...
'608  - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 Bc4 ...
'608a - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 Bc4 Nf6 7 N1e2 ...
'608b - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 Bc4  e6 7 N1e2 ...
'608t - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 N1e2...
'608u - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 N1e2 e6 7 Nf4...

'610  - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 Nf3...
'610f - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Bf5 5 Ng3 Bg6 6 Nf3 Nf6 7 Bd3...

'620  - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Nxf6...
'620a - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Nxf6 ef 6 Bc4...
'620b - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Nxf6 ef 6 Bc4 Qe7+ 7 Qe2...
'620c - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Nxf6 ef 6 c3...
'620d - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Nxf6 ef 6 Nf3...
'620e - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Nxf6 ef 6 Be2...

'621 - 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Ng3...
'621a- 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Ng3 c5 6 Nf3...
'621b- 1 e4 c6 2 d4 d5 3 Nc3 de 4 Nxe4 Nf6  5 Ng3 c5 6 Be3...

'625 - 1 e4 c6 2 d4 d5 3 Nc3 de 4 f3...
'625a- 1 e4 c6 2 d4 d5 3 Nc3 de 4 f3 ef 5 Nxf3...
'625b- 1 e4 c6 2 d4 d5 3 Nc3 de 4 f3 ef 5 Nxf3 Bg4 6 Bc4...
'625c- 1 e4 c6 2 d4 d5 3 Nc3 de 4 f3 ef 5 Nxf3 Bg4 6 Bc4 e6 7 O-O...
'625g- 1 e4 c6 2 d4 d5 3 Nc3 de 4 f3 ef 5 Nxf3 Bg4 6 Be3...

'629  - 1 e4 c6 2 d4 d5 3 Bd3...
'629a - 1 e4 c6 2 d4 d5 3 Bd3 de 4 Bxd4...

'630  - 1 e4 c6 2 d4 d5 3 e5...
'631  - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3...
'631a - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4...
'631b - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4 Bg6 6 Nge2...
'631b1- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4 Bg6 6 Nge2 Qh4 7 Nf4...
'631b2- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4 Bg6 6 Nge2 Qh4 7 h3...
'631b3- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4 Bg6 6 Nge2 Qh4 7 Ng3...

'631c - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4 Bg6 6 Nge2 Be7 7 Be3...
'631c1- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4 Bg6 6 h4...
'631c2- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 g4 Bg6 6 h4 h6 7 h5...

'631d - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 Bd3...
'631e - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 Nf3...
'631e1- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 Nf3 c5 6 Bb5+ ...
'631e2- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 e6 5 Nf3 c5 6 Bb5+ Nc6 7 O-O

'631x  - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 g4...
'631x1 - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 g4 Bd7 6 Na4...
'631x5 - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 Nf3...
'631x6 - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 Nf3 e6 6 Be2...
'631x7 - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 Bd3...
'631x8 - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 Bd3 Bxd3 6 Qxd3...
'631x9 - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 Bd3 Bxd3 6 Qxd3 e6 7 Nge2...
'631x10- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 Bd3 Qxd4 6 Nf3...
'631x11- 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nc3 Qb6 5 Bd3 Qxd4 6 Nf3 Qg4 7 h3...

'631f - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 g4..
'631g - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 g4 Be4 5 f3...
'631h - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 g4 Be4 5 f3 Bg6 6 h4...
'631i - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nf3..
'631m - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nf3 e6 5 Be2...
'631p - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nd2..
'631q - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nd2 e6 5 g4..
'631r - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nd2 e6 5 g4 Bg6 6 h4..
'631t - 1 e4 c6 2 d4 d5 3 e5 Bf5 4 Nd2 e6 5 g4 Bg6 6 Ne2..

'632  - 1 e4 c6 2 d4 d5 3 ed...
'632a - 1 e4 c6 2 d4 d5 3 ed cd 4 c4...
'632b - 1 e4 c6 2 d4 d5 3 ed cd 4 c4 Nf6 5 Nc3...
'632c - 1 e4 c6 2 d4 d5 3 ed cd 4 c4 Nf6 5 Nc3 e6 6 Nf3...
'633  - 1 e4 c6 2 d4 d5 3 ed cd 4 Bd3...
'633a - 1 e4 c6 2 d4 d5 3 ed cd 4 Bd3 Nc6 5 c3...
'633b - 1 e4 c6 2 d4 d5 3 ed cd 4 Bd3 Nc6 5 c3 Qc7 6 Ne2...
'633c - 1 e4 c6 2 d4 d5 3 ed cd 4 Bd3 Nc6 5 c3 Qc7 6 Ne2 Bg4 7 f3...
'633g - 1 e4 c6 2 d4 d5 3 ed cd 4 Bd3 Nc6 5 c3 Nf6 6 Bf4...
'633h - 1 e4 c6 2 d4 d5 3 ed cd 4 Bd3 Nc6 5 c3 Nf6 6 Bf4 Bg4 7 Qb3...

'635  - 1 e4 c6 2 d4 d5 3 Nd2 ...
'636  - 1 e4 c6 2 d4 d5 3 f3 ...
'636a - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 ...
'637  - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Bf4...
'637a - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Bf4 Ne7 6 Qd3...
'637b - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Bf4 Ne7 6 Nge2...
'637e - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Nge2...
'637f - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Nge2 Ne7 6 a3...
'637i - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Bd2...
'637j - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Bd2 Ne7 6 a3..
'637m - 1 e4 c6 2 d4 d5 3 f3 e6 4 Nc3 Bb4 5 Bd2 Ne7 6 Nce2..

'639  - 1 e4 c6 2 Nf3...
'639a - 1 e4 c6 2 Nf3 d5 3 Nc3...
'639b - 1 e4 c6 2 Nf3 d5 3 Nc3 Bg4 4 h3...
'639c - 1 e4 c6 2 Nf3 d5 3 Nc3 Bg4 4 h3 Bxf3 5 Qxf3...

'640  - 1 e4 c6 2 Nc3...
'640a - 1 e4 c6 2 Nc3 d5 3 Qf3...
'640b - 1 e4 c6 2 Nc3 d5 3 Qf3 de 4 Nxe4...
'640c - 1 e4 c6 2 Nc3 d5 3 Qf3 de 4 Nxe4 Nd7 5 d4...

'641  - 1 e4 c6 2 f3...
'641a - 1 e4 c6 2 f3 d5 3 e5...
'641b - 1 e4 c6 2 f3 d5 3 e5 Bf5 4 Nf3...

'701  - 1 g3...
'701a - 1 g3 Nf6 2 Bg2...
'702  - 1 g3 d5 2 Bg2...
'702a - 1 g3 d5 2 Bg2 Nf6 3 Nf3...
'702b - 1 g3 d5 2 Bg2 Nf6 3 Nf3 Bf5 4 d3 ...
'702c - 1 g3 d5 2 Bg2 Nf6 3 Nf3 Bf5 4 d3 e6 5 O-O...

'801  - 1 b3...
'802  - 1 b3 d5 2 Bb2 ...
'803  - 1 b3 e5 2 Bb2 ...

'805  - 1 a3...
'806  - 1 a4...
'807  - 1 c3...

'810  - 1 b4...
'811  - 1 b4 e5 2 Bb2...
'812  - 1 b4 e5 2 Bb2 Bxb4 3 Bxe5...
'813  - 1 b4 d5 2 Bb2...
'814  - 1 b4 d5 2 Bb2 Bg4 3 Nf3...
'815  - 1 b4 d5 2 Bb2 Qd6 3 a3..

'820  - 1 f4...
'821  - 1 f3...

'830  - 1 Nc3...
'831  - 1 Nc3 d5 2 e4..
'831a - 1 Nc3 d5 2 e4 de 3 Nxe3 ...
'831b - 1 Nc3 d5 2 e4 de 3 Nxe3 Bf5 4 Qf3 ..
'831c - 1 Nc3 d5 2 e4 de 3 Nxe3 Bf5 4 Ng3 ..
'831d - 1 Nc3 d5 2 e4 de 3 Nxe4 Bf5 4 Ng3 Bg6 5 Nf3...

'835  - 1 Na3...
'837  - 1 Nh3...

'840 -  1 g4...
'841 -  1 g4 d5 2 Bg2..
'842 -  1 g4 d5 2 Bg2 c6 3 h3..

'843 -  1 g4 e5 2 Bg2..
'844 -  1 g4 e5 2 Bg2 d5 3 c4..

'850 -  1 e3...
'855 -  1 d3...

'860 -  1 h3...
'861 -  1 h4...
'------------------------------------------------------------------------------

' next 3 lines up (above the data block)

'If Side = White And BookHit = FALSE Then MoveNo = MoveNo - 1 'subtract moveno if no move has been played
'Return BookHit
'End Function

'-------------------------------------------------------------------------------

Sub help

  Dim As UInteger hitspace

  '************************************
  '               HELP!
  '   Display help options - called by
  '   user typing 'help'
  '************************************

  Cls
  Print "         *************************************************"
  Print "         *                    H E L P !!                 *"
  Print "         *                                               *"
  Print "         *  save      - save game                        *"
  Print "         *  load      - load game                        *"
  Print "         *  perft     - run perft move generator test    *"
  Print "         *  listgame  - show move list                   *"
  Print "         *  level     - select level (search depth)      *"
  Print "         *  white     - NOmega plays with black          *"
  Print "         *  black     - NOmega plays with white          *"
  Print "         *  go        - make computer play               *"
  Print "         *  setboard  - set board by reading a FEN       *"
  Print "         *                                               *"
  Print "         *  end       - exit                             *"
  Print "         *                                               *"
  Print "         *                                               *"
  Print "         *     all entries must be made in lowercase     *"
  Print "         *                                               *"
  Print "         *             hit SPACE to continue             *"
  Print "         *************************************************"

  Do
    hitspace = GetKey
  Loop Until hitspace = 32
  Cls

End Sub

'------------------------------------------------------------------------------

Sub SaveGame

  '****************************************
  ' Writes game details to 'SaveNOmega.DAT'
  ' routine saves:
  '   - board position
  '   - move list
  '****************************************
  Dim As Integer x, piece
  'Dim As UInteger y, ff = FreeFile
  Dim As UInteger y, ff = 99
  /'
  'OPEN "O", 1, "SaveNomega.DAT"

  Open "SaveNumpty.DAT" For Output As #ff

  Print #ff, cmpclr                              ' ### computer is ???
  Print #ff, side                                ' ### witch side is to play
  Print #ff, bookhit                             ' ### out of book ???
  Print #ff, w_cas(1)                            ' ### is castling allowed
  Print #ff, w_cas(2)
  Print #ff, b_cas(1)
  Print #ff, b_cas(2)
  Print #ff, mat_left
  Print #ff, endgame

  For X = 90 To 20 Step -10
    For Y = X+1 To X+8
      Print #ff, B(Y)
    Next Y
  Next X
  If moveno = 0 Then moveno = 1                  '###
  Print #ff, MoveNo

  For y = 1 To MoveNo
    Print #ff, move_hist(y, 1), move_hist(y, 2), move_hist(y, 3), move_hist(y, 4)
  Next y
'/
  ff = 99
  Print #ff,
  Print #ff, "//////////////////////////////////////////"
  Print #ff,

  For X = 90 To 20 Step -10
    For Y = X+1 To X+8
      piece = B(y)
      If piece < 0 Then piece += 20
      Print #ff, "   "; b_str(piece);            ' u_str(Y) ;
    Next Y
    Print #ff, "   ***  ";(X \ 10) - 1
    Print #ff, "   -----------------------------"
  Next X

  Print #ff, "   *********************************** "
  Print #ff, "   *********************************** "

  Print #ff, "   a   b   c   d   e   f   g   h   "

  Print #ff,
  Print #ff, "Move   White    Black"

  For y = 1 To MoveNo
    Print #ff, Using " ###"; y;
    Print #ff, "   "; move_2_str(move_hist(y, 1)); "-"; move_2_str(move_hist(y, 2));
    If move_hist(y, 3) = 0 Then Exit For
    Print #ff, "    "; move_2_str(move_hist(y, 3)); "-"; move_2_str(move_hist(y, 4))
  Next y

  Close #ff

End Sub

'-------------------------------------------------------------------------------

Sub LoadGame

  '**************************************
  ' Loads previously saved game
  ' routine reads:
  '   - board position
  '   - move list
  ' from 'SaveNOmega.DAT'
  '**************************************
  Dim As Integer x
  Dim As UInteger y, ff = FreeFile

  'OPEN "I", 1, "SaveNomega.DAT"
  Open "SaveNumpty.DAT" For Input As #ff

  Input #ff, cmpclr                              ' ### computer is ???
  Input #ff, side
  Input #ff, bookhit
  Input #ff, w_cas(1)                            ' ### is castling allowed
  Input #ff, w_cas(2)
  Input #ff, b_cas(1)
  Input #ff, b_cas(2)
  Input #ff, mat_left
  Input #ff, endgame

  For X = 90 To 20 Step -10
    For Y = X+1 To X+8
      Input #ff, B(Y)
    Next Y
  Next X

  Input #ff, MoveNo

  For y = 1 To MoveNo
    Input #ff, move_hist(y, 1), move_hist(y, 2), move_hist(y, 3), move_hist(y, 4)
  Next y
  If move_hist(y, 1) = 0 And move_hist(y, 2) = 0 Then moveno = 1

  Close #ff

  'Cls
  display
  If side = white Then
    Print "white ";
  Else
    Print "black ";
  End If
  Print "to move"
  Print "white castling possible:  ";
  If w_cas(1) = 0 Then Print "short (0-0) ";
  If w_cas(2) = 0 Then Print "long (0-0-0)" Else Print
  Print "black castling possible:  ";
  If b_cas(1) = 0 Then Print "short (0-0) ";
  If b_cas(2) = 0 Then Print "long (0-0-0)" Else Print
  Print
  Print "hit any key"

  ' ### find wkloc and bkloc
  For X = 20 To 90 Step 10
    For Y = X + 1 To X + 8
      If b(y) = 7 Then
        wkloc = y
      ElseIf b(y) = -7 Then
        bkloc = y
      End If
    Next Y
  Next X

  Sleep

End Sub

'-------------------------------------------------------------------------------\\
