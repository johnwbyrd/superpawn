Piranha chess engine
version 0.5 (April 11th 2003)


General
-------------------
Piranha is a chess engine for Windows.
This version (0.5) has a rating of about 1800 on schach.de and one of about 2000 on ICC.
Piranha supports the UCI protocol and can be loaded into various GUIs such as Arena (recommendable free interface, see playwitharena.com) or Fritz 8. Piranha does not come with an own GUI. Winboard protocol is not supported.
Piranha is an alpha-beta searcher. It uses a transposition table as well as history and killer heuristics. 
The move generator is based on bitboards, but is slow nevertheless. 
Piranha is capable of pondering.
Piranha is not able to use Nalimov endgame table bases.


Transposition table size
------------------------------
The size of the transposition table may be changed by the user. 
Each hash entry takes 15 bytes, so actual hash table sizes are: 15MB, 30MB, 60MB, ... .
However, if you chose values like 16MB these will be cut down to the next lower appropriate value, in this case 15MB. This goes especially for small requested table sizes like 8MB, 4MB, 2MB and 1MB.
By default, table size is 15MB.


perft
-----------------
There is a command "perft <x>" in console mode, eg. type "perft 5". Then the complete game tree up to depth <x> plies will be computed from the momentary position. No evaluations or tree-cuts will be done. This is just for testing the correctness and speed of the move generator. The perft command was introduced by Crafty and is also supported by other programs.


Known bugs/problems
------------------------
When there is only one legal move in the root position, Piranha returns a score of 0 and plays the move immediately without any search. Fritz 8 appears to handle scores of 0.00 as draw offers.


Contact
-------------
If you have any comments, suggestions or find a bug, please send a
mail to: martin.villwock@uni-dortmund.de


License
------------------
Copyright (C) 2003 Martin Villwock. All rights reserved.
Piranha is distributed free of charge.
See below for warranty information.
Piranha may not be distributed as part of any software package,
service or web site without prior written permission from the author.


Warranty
-------------------
1. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
REPAIR OR CORRECTION.

2. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER BE LIABLE TO YOU FOR DAMAGES,
INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.


Thanks
------------
I want to thank Robert Hyatt (Crafty), Gian-Carlo Pascutto (Sjeng), Tom Kerrigan (TSCP) and Bruce Moreland (Gerbil) for publishing their sources.
Special thanks go to Martin Bauer (DelphiMax) and Steffen Basting (Mooboo) for playing test games and discussing computer chess related topics.

