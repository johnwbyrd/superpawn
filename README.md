Ippon
=====

Ippon is a trivial, pedantic, and somewhat silly chess
engine.  As of this writing, it is capable of only playing
a pseudo-legal game of chess. 

Current features
----------------

- Implements a subset of UCI protocol sufficient to permit play 
  with Arena 3.0
- Pluggable architecture permits easy experimentation with 
  new algorithms for search and evaluation  
- Alpha-beta search of fixed depth
- Material evaluator
- Uses C++11 features, as well as some gratuitous functional
  programming concepts
- All the code exists within a single C++ source file
- Vaguely const-correct


Anti-features (things the engine doesn't do)
--------------------------------------------

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
<http://creativecommons.org/licenses/by/3.0/deed.en_US>.