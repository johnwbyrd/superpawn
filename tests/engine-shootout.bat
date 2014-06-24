rem engine-shootout
set TARGETDIR=..\build\tests
if exist %TARGETDIR%\nul goto setdate
mkdir %TARGETDIR%
:setdate
set mydate=
for /f "skip=1" %%x in ('wmic os get localdatetime') do if not defined mydate set mydate=%%x
SET PGN_DATABASE=%TARGETDIR%\engine-test-%mydate%.pgn
..\tools\win32\cutechess-cli\cutechess-cli.exe -pgnout %PGN_DATABASE% -engine name=superpawn cmd=superpawn.exe dir=..\build\win\x64\release -engine name=superpawn cmd=superpawn.exe dir=..\build\win\x86\release -each proto=uci tc=60/180 -games 10 -wait 1