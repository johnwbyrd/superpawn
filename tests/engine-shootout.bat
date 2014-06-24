rem engine-shootout
SET varYYYY=%DATE:~10,4%
SET varMM=%DATE:~4,2%
SET varDD=%DATE:~7,2%
SET varTodaysDate=%varYYYY%%varMM%%varDD%
SET PGN_DATABASE=%varTodaysDate%.pgn
..\tools\win32\cutechess-cli\cutechess-cli.exe -pgnout %PGN_DATABASE% -engine name=superpawn cmd=superpawn.exe dir=..\build\win\x64\release -engine name=superpawn cmd=superpawn.exe dir=..\build\win\x86\release -each proto=uci tc=60/180 -games 10 -wait 1