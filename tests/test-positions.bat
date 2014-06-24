rem test-position
set TARGETDIR=..\build\tests
set EPDDIR=..\tools\win32\epd2wb
set ENGINE=..\build\win\x64\release\superpawn.exe
if exist %TARGETDIR%\nul goto setdate
mkdir %TARGETDIR%
:setdate
set mydate=
for /f "skip=1" %%x in ('wmic os get localdatetime') do if not defined mydate set mydate=%%x
SET PGN_DATABASE=%TARGETDIR%\position-test-%mydate%.pgn
%EPDDIR%\epd2wb.exe -c%EPDDIR%\uci.txt %ENGINE% %EPDDIR%\wacnew.epd 120