<link href="http://kevinburke.bitbucket.org/markdowncss/markdown.css" rel="stylesheet"></link>
Superpawn
=========

Superpawn is a trivial, slow, pedantic, and silly UCI chess engine.  

As of this writing, it is capable of only playing
a pseudo-legal game of chess, as it doesn't yet understand the notion that 
checkmate constitutes the end of the game.  Superpawn is capable of making
many fascinating chess moves, some of which are actually even legal.

Superpawn is an excellent example of the "objects gone wild" style of
programming, in which Everything Is An Object.  Even the pieces themselves
are objects; they know how to move, capture, etc.  This of course slows 
down the move generation and evaluation process immensely, making this 
program irredeemably slow in tournament conditions.

Superpawn requires a C++11 compiler with support for threading.  It runs
on Windows and Linux systems, and compiles under Microsoft, gcc and clang
compilers.  A CMake implementation is provided to ease compilation.

Features
--------

- ANSI C++11 code
- Compiles under Microsoft Visual Studio 2012, gcc 3.8.2, and clang 3.3
- Implements a subset of UCI protocol sufficient to permit play 
  with Arena 3.0+
- Pluggable architecture permits easy experimentation with 
  new algorithms for search and evaluation  
- Principal variation search
- Basic material evaluator
- Basic mobility evaluator
- Gratuitous functional programming
- All the code exists within a single C++ source file
- Vaguely sort of const-correct
- Compiles cleanly in 32-bit and 64-bit modes
- Compatible with cmake build systems


Things it doesn't do
--------------------

- Check
- Checkmate
- Stalemate
- En passant
- Castling
- Time controls
- The fifty move rule
- Transposition tables
- Play well

License
-------

Source code is provided under the Creative Commons 3.0 Attribution 
Unported license.  Full details are available at
<http://creativecommons.org/licenses/by/3.0/deed.en_US>.  I'm sure that if
you steal this work and paste your name on it as though you had written
it, you're a complete idiot in at least three major ways.