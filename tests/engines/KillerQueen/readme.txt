*-------------------------------------------*
|------ KillerQueen UCI Chess Engine -------|
*-------------------------------------------*

*-------------------------------------------*
|version....: 2 beta 3                      |
|					    |
|author.....: Lorenzo Stella		    |
|					    |
|homepage...: killerqueen.altervista.org    |
|					    |
|e-mail.....: lore_star@libero.it           |
*-------------------------------------------*



KillerQueen2 is a UCI chess engine, and it is best experienced if played under Arena GUI or other UCI-compatibile 
GUIs. Anyway, if you want to play it under the command line these are the main commands:

----------------------------------------------------------------------------------------------------------------
go ..........: computer will play the next move
----------------------------------------------------------------------------------------------------------------
d ...........: draw the chessboard in ASCII format
----------------------------------------------------------------------------------------------------------------
position ....: this command is to set a position on the chessboard:
   position startpos .......: set the board to the classic chess start position
   position fen <string> ...: set the board to the position specified in the fen string
   position [startpos | fen <string>] moves <move1> <move2> ...: set the board to a position and make the moves
----------------------------------------------------------------------------------------------------------------
quit ........: exit the program
----------------------------------------------------------------------------------------------------------------
Moves string format is the standard coordinate notation:
   
   d2d4, g1f3, b7d5 ....: normal moves or captures
   g7g8q, c2c1r ........: move and promotion (to the specified kind)
----------------------------------------------------------------------------------------------------------------

For any bug report or communication please contact me via e-mail at lore_star@libero.it
for more informations and details visit KillerQueen homepage at killerqueen.altervista.org

Lorenzo Stella