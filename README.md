<link rel="stylesheet" href="http://jasonm23.github.io/markdown-css-themes/foghorn.css">
</link>
Superpawn
=========

Superpawn is a trivial, slow and weak [chess engine](http://en.wikipedia.org/wiki/Chess_engine).  Superpawn
uses the [Universal Chess Interface](http://en.wikipedia.org/wiki/Universal_Chess_Interface)
protocol in order to communicate with a [compatible graphical user interface](http://www.playwitharena.com/) of 
your choice.

The latest build of [Superpawn](http://chess.johnbyrd.org) can always be downloaded
from [http://chess.johnbyrd.org](http://chess.johnbyrd.org) .

Downloads
---------

[Windows x86 executable](http://chess.johnbyrd.org/build/win/x86/superpawn-windows-x32.zip).  If
you don't know which file to download, this one is probably it.

[Windows x64 executable](http://chess.johnbyrd.org/build/win/x64/superpawn-windows-x64.zip).  Slightly 
faster for x64 machines.

[Source code](http://www.github.com/johnwbyrd/superpawn).  For building on arbitrary Macintosh and 
Linux boxes.  The [Travis](https://travis-ci.org/johnwbyrd/superpawn) build system currently reports
Superpawn's build status as:
[![Build Status](https://travis-ci.org/johnwbyrd/superpawn.svg?branch=master)](https://travis-ci.org/johnwbyrd/superpawn)

Description
-----------

Superpawn is capable of making many fascinating chess moves, most
of which are actually legal.

As of this writing, it is capable of only playing
a pseudo-legal game of chess, as it doesn't handle moves like
check and castling correctly.

Superpawn is an excellent example of the "objects gone wild" style of
programming, in which Everything Is An Object.  Even the pieces themselves
are objects; they know how to move, capture, etc.  This of course slows 
down the move generation and evaluation process immensely, making this 
program irredeemably slow in tournament conditions.

Superpawn requires a [C++11](http://en.wikipedia.org/wiki/C%2B%2B11) compiler
with support for threading.  It builds and runs on Windows, Linux and MacOS systems, 
and  compiles under Microsoft, gcc and clang compilers.  A [CMake](http://www.cmake.org/)
implementation is provided to ease compilation on arbitrary targets.

Features
--------

- ANSI C++11 code
- Compiles under Microsoft Visual Studio 2013, gcc 3.8.2, AppleClang 5.1.0,
  and clang 3.3
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
- Transposition tables to help speed up end game
- Simple test framework based on [cutechess-cli](http://cutechess.com/)

Things it doesn't do
--------------------

- Castling
- Draw by repetition
- Time controls
- The fifty move rule
- Play chess well

License
-------

Source code is provided under the [Creative Commons 3.0 Attribution 
Unported](http://creativecommons.org/licenses/by/3.0/deed.en_US) license.  Please
don't pass off this chess engine as your own work.