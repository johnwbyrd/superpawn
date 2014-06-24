EPD2WB - AN EPD TEST HARNESS FOR WINBOARD ENGINES

Bruce Moreland (brucemo@seanet.com)
20-Jun-2001
Additions by Thomas Mayer (thomas.j.mayer@t-online.de)
23-May-2005

CONTENTS:

0) SOME NOTES REGARDING COPYRIGHT AND LICENSING
1) WHAT IS EPD?
2) WHERE CAN I GET EPD FILES?
3) WHY IS EPD2WB USEFUL?
4) DOES IT WORK PERFECTLY?
5) RUNNING EPD2WB
6) HOW IT WORKS
7) TROUBLESHOOTING - SOME GENERAL TIPS
  7A) ENGINES THAT DON'T OUTPUT ANALYSIS FOR SEVERAL SECONDS
  7B) ENGINES WITH EXTRA CRUD IN THE ANALYSIS LINE
  7C) ENGINES THAT OUTPUT MOVES IN NON-ENGLISH
  7D) ENGINES WITH NO "ANALYZE" COMMAND
  7E) ENGINES WITH NO "PING" COMMAND
  7F) ENGINES THAT WILL NOT STOP THINKING
  7G) ENGINES THAT DO NOT PRODUCE ANALYSIS
  7H) ENGINES THAT DON'T HAVE "EDIT" or "SETBOARD"
  7I) AUGH
8) COMMAND-LINE ARGUMENTS
9) EPD FIELDS SUPPORTED
10) COMPATIBILITY LIST
11) NOTES ABOUT UCI ENGINES
12) WINBOARD ENGINES THAT DO NOT WORK
13) HISTORY
14) PLANNED IMPROVEMENTS
15) FILES IN THE PACKAGE
16) THANKS TO


0) SOME NOTES REGARDING COPYRIGHT AND LICENSING

All of the sources and executables contained within this project are covered
by the GNU GPL, a copy of which is also included along with this project.

The basic idea is that if you can't sell the project executables without
providing source code, even if you modify the source code, and the source code
must be more or less freely provided and be redistributable.  So you can't
turn this project into proprietary software, even if you make a lot of cool
changes to it.  The copyright on the software is retained by me.

If you have any comments regarding free software, the GPL, my interpretation
of the GPL, or if you have anything to ask or suggest regarding my plans for
this software, please drop me an email.


1) WHAT IS EPD?

http://www.very-best.de/pgn-spec.htm

The EPD standard is defined in section 16.2 of the above document.  The basic
idea is that you can make a file that contains chess positions, plus
information about each position, such as one or more "best" (solution) moves
in the position.  You can teach a chess program to eat an EPD file, set up the
positions, and test its moves against the moves specified in the EPD string.

This is useful if you want to get an indication of how strong an engine is, or
if you want to see if a change you made to an engine has changed the score on
the test.


2) WHERE CAN I GET EPD FILES?

I have no idea.  If someone knows a good permanent download location, please
let me know.

I included one EPD file (ECM, "Encyclopedia of Chess Middlegames"), along with
Gerbil.  This file is easy to find on the net.  I don't think I am violating
anyone's copyright by distributing it.  It's a fairly tough tactical suite.


3) WHY IS EPD2WB USEFUL?

When I was building my Gerbil Winboard engine, I wanted to test it against an
EPD suite.  I couldn't, because EPD suites aren't supported by Winboard.  I
could have built a command into the engine, but I didn't like that idea
because there's no Winboard command for running suites either, so I would have
had to add a command, and this did not appeal to me.

I hit upon the idea of writing a program that would pretend to be Winboard.
This program would load an engine and an EPD, and pump the EPD positions
through the engine one at a time.

The thing about Winboard is that it is a standard, so if EPD2WB works with my
engine it should work with others.  I thought this was a pretty cool idea, so
I decided to try to generalize this utility.

A problem is that various engines implement different parts of the the
Winboard standard, and they implement these parts in different ways.

I started with a vanilla new protocol (protover 2) implementation, but since
I have received some favorable response regarding this app, I have expanded
it to handle old (protover 1) engines.


4) DOES IT WORK PERFECTLY?

Not in a million years.  Since all apps are different, and everyone has their
own bugs and weird behaviors, and some parts of the Winboard protocol have
historically be difficult to implement and/or understand, it is likely that
there will always be bugs.

And since the program depends upon understanding analysis output by each
engine, including parts of the analysis for which there is *no* standard,
the program will always be half-baked.  It does, however, work at least
vaguely with most of the engines that I have tried.


5) RUNNING EPD2WB

To run the app, type:

    epd2wb <engine.exe> <suite.epd> <seconds to think>

If you need to pass parameters to the engine, you can do this:

    epd2wb "<engine.exe> params" <suite.epd> <seconds to think>

For example, to run the app with Gerbil, on the EPD test suite, for 20 seconds
per move, you would do:

    epd2wb gerbil.exe ecm.epd 20

If the program is running correctly you will see lots of output scrolling down
your screen.  You can redirect this output to a file if you want to keep it.

If the program doesn't start spitting out analysis immediately, it could be
due to a bug in my utility, or the engine could be doing something weird.  The
utility should be able to work with all conformant Winboard engines.


6) HOW IT WORKS

The utility uses standard Winboard commands to send a position to the engine,
then it tells it to think.  The engine should be spewing out analysis while
it is thinking.  The utility captures this analysis and tries to make sense
out of it, in order to figure out when the engine has achieved a correct
answer.

Newer versions of Winboard try to figure out what kids of commands the engine
is capable of handling.  This utility does the same thing.

The utility works best with engines that can handle the "setboard", "analyze",
and "ping" commands.  "ping" is used for synchronization, "setboard" is how
positions are given to the engine, and the utilty puts the engine into
infinite analysis mode with "analyze".  When time runs out on the test
position, the utility spits out a short summary of what happened and sends
the next position.

Older engines, and some newer engines, can't support some of these commands.
If "setboard" is missing, the utililty will attempt to use "edit" to set up
the board.  This has the disadvantage that the en-passant square and castling
flags may not be set properly.

If the engine can't support "analyze", the utilty will set a very long time
control and tell the engine it has a lot of time to think.  This isn't that
bad, but it's cleaner to use "analyze".

If the engine can't support "ping", the utility will wait a couple of
seconds between positions, so the potential for synchronization problems is
reduced.


7) TROUBLESHOOTING - SOME GENERAL TIPS

If an engine implements a good chunk of the Winboard command set, it should
work with no problems.  However, many engines selectively implement parts of
the Winboard command set, and even those engines that do implement the set
completely according the spec may do some weird things when they emit
analysis, which is completely legal according to the spec.  This program has
a lot of its features only because *most* engines emit analysis lines in
approximately the same way.

What you should try first is a simple command line such as:

    epd2wb gerbil.exe ecm.epd 20

This will cause Gerbil to output a lot of stuff on your screen, and you can
see that everything works properly.

If you ever get so confused when trying to get an engine working, like for
instance the utility produces *no* output, you can check to see if the engine
is behaving like a reasonable Winboard engine.

To see exactly what the engine is outputting, and to see what the utility is
telling the engine, use the "-d" parameter, as in:

    epd2wb gerbil.exe ecm.epd 20 -d

This will dump a lot of stuff on your screen, and you might see error messages
or stuff obviously happening when it shouldn't happen.

I'm going to go through some obvious problems in order of increasing severity,
so you can get an idea how to handle some common problems.


7A) ENGINES THAT DON'T OUTPUT ANALYSIS FOR SEVERAL SECONDS

A lot of engines have a delay built in, during which they won't emit any
analysis, presumably because they don't want to be bad at blitz chess because
they're spending all their time writing crap to Winboard rather than thinking
about moves.

If you can't figure out a way to get an engine to emit analysis during the
first few seconds, you have two possibilities:

a) Run your tests for a longer time, so it emits analysis which can be use
to score each problem.

b) Do the stuff listed in section 7G.  You can use the "-t" command to cause
the engine to produce a move, which can be used to score the problem after as
little as one second.  You won't get the variation it was thinking about, but
you will get the move.


7B) ENGINES WITH EXTRA CRUD IN THE ANALYSIS LINE

Most engines emit an analysis line that goes something like "1. e2e4 e7e5" or
"e2e4 e7e5" or "1. e4 e5", etc.  Sometimes you get move numbers and sometimes
you don't, and sometimes the stuff is in standard algebraic, and sometimes it
is in some other form of algebraic.

Some engines add something extra.  Mint will give you a little smiley face at
the beginning of its line, as in ":-) e4 e5".  Epd2Wb can't parse that, so it
will not recognize any correct answers coming out of Mint.

If you pass the "-s1" parameter to the utility, it will always skip one token
at the start of the analyis line, which will discard Mint's smiley face, and
everything will work fine.

Engines that have this problem won't crash or anything, they just won't get
any correct answers on tests.


7C) ENGINES THAT OUTPUT MOVES IN NON-ENGLISH

Some engines, such as Bringer, output some of their analysis in German or
Italian or Spanish.  Epd2Wb can handle this, via the "-l" command-line
parameter.

To get Bringer working, you pass it "-l German", and suddenly "e7-e8D" is
taken as a correct answer when the solution is "e8=Q+".

To see a list of languages, type "epd2wb" at the command-line.


7D) ENGINES WITH NO "PING" COMMAND

To see if an engine supports "ping", or any of the other commands that Epd2Wb
uses, type in a normal Epd2Wb command-line, but append the "-i" parameter.
Epd2wb will stop right after it prints out information about the engine.

"ping" is used for synchronization, so if an engine doesn't have it, it's
pretty serious.  What ends up happening is that you get analysis for this
position spilling into the next position, and the utility can't keep them
seperate.  So instead it waits several seconds between positions, by default.

If you get the idea that it's not waiting long enough, try the "-x" switch,
as in "-x 5", which will cause it to wait five seconds between positions.

The "-x" switch is overriden by the "-t" switch (explained later), since there
aren't any synchronization issues if you use "-t".


7E) ENGINES WITH NO "ANALYZE" COMMAND

The "analyze" command is very important for Epd2Wb.  It tells the engine to
think forever.  When the test period is over, the utility sends "exit" to tell
the program to stop analyzing, and goes on to the next test.

If an engine doesn't have analyze, I have to do things differently.  What I do
is tell the engine it's playing a game at very long time controls, and then
just proceed as above, without the "exit" command.  What I try to do to get
it to stop thinking is send it a "new", in the hopes that it will see that,
stop thinking, and initialize itself.  Most importantly that it will stop
thinking.

This is not as clean as using "analyze", but it works in some cases.  If the
engine emits a move, the utility can handle it.  It will stop the test and
move on to the next position.


7F) ENGINES THAT WILL NOT STOP THINKING

Some engines such as Gullydeckel 2 don't want to stop thinking when Epd2Wb
passes them the "new" command.  This is extremely hard to deal with, but there
is something you can try.

The "-t" parameter tells the utilty to change the way it tries to control the
engine.  It does two things:

a) It tries to use the "st" command to tell the engine to move at the end of
the test interval.

b) It tells the engine that it only has the test interval in which to think.

If the engine implements "st", it should see that it only has a limited time
to think, and when it's done it will move.  The problem is that "st" is not a
very useful command, so someone who didn't do the work to let the program be
interrupted during a search probably didn't implement it.

If the engine can't accept "st", it will think it is playing a game with a
long sudden-death time control, and it only has a few seconds *for the rest
of the game*.  So in this case, it will tend to move very quickly.

You can surive this case by adjusting the test time until the engine makes
its moves in the proper amount of time.

For instance, to get Gullydeckel 2 working, I have to use the "-t" argument,
but the engine then makes moves in about 1/30 of the test time.  So I simply
multiply my test times by 30 and the engine works essentially perfectly.

If you specify "-t", the utilty will not even attempt to abort the search in
order to run another test -- it will wait until the engine moves, and the
combination of the "st" command and the "time" command should assure that it
does move eventually.


7G) ENGINES THAT DO NOT PRODUCE ANALYSIS

Some engines just move -- they don't produce any analysis.  The amount of
information that you can get when an engine doesn't honor the "post" command
is pretty minimal, but it's still possible to get some information out of
Epd2Wb.

The utility will figure out when the program moved, will output as much data
as it has, and it will check the move produced against the correct answer.

In normal cases, I rely on the engine to tell the utility how long it took
to solve a problem, but in this the utility has to keep track of time itself.


7H) ENGINES THAT DON'T HAVE "EDIT" or "SETBOARD"

Tough luck.  If there is no sensible way to tell the engine what position to
think about, this utility will not work.


7I) AUGH

If you've read all the above, and can't get anything to work, please drop me
an email and I'll try to help you, if I can get ahold of the engine you are
using.


8) COMMAND-LINE ARGUMENTS

    epd2wb <engine> <epd> <seconds> [switches]

    epd2wb "<engine> [engine args]" <epd> <seconds> [switches]

Switches:

    -?      Usage.  This displays command-line arguments.

    -d      Dumps input and output to the console.

    -i      Dumps engine information and then quits (no test run).

    -l<L>   Sets the analysis language to L.  Some program emit their analysis
            in languages other than English, and this will attempt to
            translate the piece abbreviations. Many languages are supported.

    -s<D>   Skips D fields at the beginning of analysis lines.  If you don't
            specify this switch, the engine will skip zero fields.

    -t      Uses "st" command to tell the engine how long to think.  Try this
            switch if an engine 1) can't handle the "analyze" command, and 2)
            refuses to do anything (it just keeps looking at the first EPD
            forever).  This might kick it in the head and cause it to do
            something.  However, what is likely to happen is that the program
            will make a move before the search period has expired.

    -w<S>   This is modified. Now it specifies the protover 1 wait time for
            initialization after "xboard" was sent. Default is 4 seconds.
            
Additional Switches by Thomas Mayer

    -a      Turns usage of "analyze" on by default. There are several
            protover 1 engines that support analyze. With that switch you can
            use that option
            
    -b<B>   With that switch you can turn on the "database"-function of
            epd2wb. It will save then the results in a file specified by <B>.
            If the file already exists the results will be appended. In the
            file is the following information saved: FEN of each position, name
            of the engine, time to solution and depth for solution. You can
            load that file later in the spreadsheet-program of your choice.
            
    -c<E>   Turns uci-mode on. With that you can test also UCI engines with
            epd2wb. In the file <E> you can specify the UCI-options which
            should be set different from the default settings. For an example
            take just a look in the uci.txt which is part of the epd2wb
            package. Mate announcements in UCI are transfered to a score
            fitting the 32767-system.
            
    -e<S>   Specifies how many seconds epd2wb should wait after starting the
            engine until it sends the first command to the engine.
            
    -f<F>   Specifies a file where epd2wb should output a logfile. With older
            epd2wb it was necessary to use a pipe in the commandline to save
            the log. Problem was that you could not see anything then during
            the testset is running. This solves this problem.
    
    -g<x>   Specifies a number when the analysis has to be stopped after the
            right solution did occur for <x> plys
            
    -h<y>   Specifies a score where the analysis is stopped if the found move
            is correct and the score is above <y>
    
    -j      skips any UCI 2 commands. This is useful for some engines that do
            not work properly if they receive e.g. "ucinewgame". E.g. Glaurung
            
    -n<x>   Multiplys the time sent with the st command with x. Some move too
            fast with a normal st. This solves this issue for several engines
    
    -o      skips any protover 2 commands. This is useful for some engines that
            do not work properly if they receive "protover 2". E.g. the old
            winboard SOS.
    
    -p<P>   specifies the path to the engine. With old epd2wb it was always
            necessary to copy epd2wb in the folder of the engine or the
            complete engine with all it's files in the epd2wb folder. Now you
            can specify where the engine and it's files are found.
            
    -u      turns usage of setboard on by default. Some protover 1 engines
            support setboard as well. The old version wouldn't have recognized
            it. With this switch you can tell epd2wb that it should use it
            anyway.
            
    -v<T>   Forth to interpret the time in a way specified with <T>. s=seconds,
            t=tenthseconds, c=centiseconds, m=milliseconds.
            
    -x<S>   specifies the wait time between tests in seconds for engines which
            do not support ping/pong.
    
    -y      use internal clock for time measurement. Some engines send strange
            values as time, with this you can get results anyway.
    
    -z      ignore "feature done=0" and "feature done=1". This is useful if the
            engine is somewhat buggy in it's usage of the "done"-feature. E.g.
            an older Crafty version. (18.something)

Example:

        take a look in the stapeltest.bat which is part of the package. There
        are solutions for several engines which did work this way on my PC.
        I hope that in the future some might help me with new settings for
        other engines.
        
9) EPD FIELDS SUPPORTED

The utility supports the FEN, of course.  It also supports "bm" (best move),
which is the most common field aside from the FEN.  Additionally, it supports
"am" (avoid move), "id" (name of this test), "fmvn" (first move number), and
"hmvc" (50-move counter).


10) COMPATIBILITY LIST

I based my evaluations upon the first few positions of ECM.  If the program
ate them, I considered it to have worked.  If you find that it can't do the
whole suite, or that it crashes on some particular position, please let me
know and also try to isolate the problem so you can give the engine author
a bug report.

If I say you have to pass an argument to the engine itself, once again, how
you do that is:

    epd2wb "bringer.exe /winboard" ecm.epd 20 -lGerman

Note the quotes, which are totally important.


Engine         Version  Switches        Notes
-------------- -------- --------------- --------------------------------------

Bringer        1.8      -lGerman        It emits promotion pieces in German.
                                        A longer wait time might be good to
                                        try with this engine if you have
                                        problems, since it doesn't have
                                        analyze, and its UI is kind of slow.

                                        You need to pass it:

                                        "/winboard", like "bringer /winboard".

Gerbil         Any      None            Works fine.

GnuChess       4?       None            Works fine.

Green          2.13     None            Works fine.
Light
Chess

GullyDeckel    2        -t -s1          This engine has problems because it
                                        won't interrupt itself, and it doesn't
                                        have the "st" command either.  You can
                                        pass the "-t" switch, and then tell it
                                        that it has about 30 times longer per
                                        position than you want it to think.

                                        You need to pass the engine:

                                        "-p --transref 20".

Mint           2.1      -s1             Works fine.

Sjeng          10.0     None            Works fine.

Yace           0.99.01  -s1             Works fine.

If you have anything you want to add to this list, or if your results differ
from mine, or if I have made some stupid mistake, please let me know.

Additionally take a look at the stapeltest.bat

11) NOTES ABOUT UCI ENGINES

The UCI implementation does only consider informations that are sent with the
PV. There are some Engines that only send a score and the pv or even only
the pv. To get results with them anyway, you might add -y to the option list.
Also when you want to specify a time interpretation with -v then you must
add the -v option AFTER the -c option, else it will be overwritten. The only
engine I know which needs this is Tornado, because it sends the time in
centiseconds and not according to the specification in milliseconds.

Following UCI-Engines do not work at all:

Alfil v5.04             - crashes
DarkFusch v0.9          - does not send any PV information
Eagle v.068             - does not react on 'stop' command
Eden v.006              - crashes
Gibbon v1.02            - crashes
Philemon                - crashes
Piranha v.05            - crashes

List to be continued

12) WINBOARD ENGINES THAT DO NOT WORK

I had once a list, but it is gone... You may help me here ? There are still
several issues and it seems to differ on which system you use them, so e.g.
Gandalf 4.32f is working fine for me, but seems to cause troubles for Gabor
Szots. I have also noticed different behaviour on my Dual compared to my Single
systems.

13) HISTORY

22nd MAY 2005
        Added UCI compatibility
        Added hopefully successful compatibility to DanaSah
        (due to an eMail of Pedro Castro)

23rd MAY 2005
        Written the first version of the readme.txt
        Added two new feature
        -g <x> to stop after x plys of correct solution
        -h <y> to stop when score reaches y and solution is correct
        Written second version of the readme.txt
        fixed a bug in the database function
        adjustments for Armageddon
        
25th MAY 2005
	Volker Pittlik pointed out that epd2wb crashes when he did use e.g.
	-cuci.txt instead -c uci.txt -> that is solved, epd2wb would understand
	now both correctly.
	Added a new option:
	-j you can switch off any UCI 2 commands send by epd2wb. With that
	Glaurung works now finally fine.

14) PLANNED IMPROVEMENTS

Dann did ask me once if I can't implement an output that can be used for his
CAP project. Well, I will try it at least.
Also I will try to improve compatibility to even more xboard and uci engines.
Must I mention bugfixing here ? :)

15) FILES IN THE PACKAGE

epd2wb.exe      - well, the executeable
epd2wb.cpp      - C++ Source
epd2wb.h        - C++ Header File
gpl.txt         - GNU General Public License description
uci.txt         - sample UCI option definition
wacnew.epd      - epd file
wacnew_tornado0817_5_P1200_N5a.log
                - sample log file of Tornado in wacnew.epd
                  on my P1200 laptop with 32 MB hash, all
                  5 men Nalimov TBs available.
                  Testtime 5 seconds per position
wacnew_5_P1200_N5a.txt
                - sample database file - just try to load
                  that in your favorite spreadsheet software.
                  You will now immediately what this could be
                  good for.
Stapeltests.bat - currently working engines with correct settings.
                  That's how they work on MY machine
readme.txt      - the file you are currently reading

16) THANKS TO

In no particular order - except maybe Bruce - he deserves place one... :)

Bruce Moreland          - for creating initially this fine tool
Uri Blass               - for giving many hints for new options and output
Leen Ameraal            - well, he spent part of the code (the routine that
                          discovers how many positions are in the EPD) and
                          also gave me several ideas for new options
George Lyapko           - for testing and some other option proposal
Roger Brown             - for the good spirit
Dann Corbit             - also for giving some new ideas for options
Manfred Meiler          - for testing and somehow he is to be called guilty
                          that the database feature was born
Andreas Herrmann        - for some testing and additional tools
Gabor Szots             - for testing and hints
Volker Pittlik          - for several tests and hints

--
Copyright (C) Bruce Moreland, 2001.  All rights reserved.
Modified  (C) Thomas Mayer, 2005.
Please look in "gpl.txt" for information on the GNU General Public License.

