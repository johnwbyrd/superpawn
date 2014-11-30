rem gauntlet-test
rem Run a tournament of the most recent Superpawn build versus a 
rem bunch of engines
:parse_command_line
IF NOT "%1"=="" (
    IF "%1"=="--BUILD_ID" (
        SET BUILD_ID=%2
        SHIFT
    )
    IF "%1"=="--BUILD_NUMBER" (
        SET BUILD_NUMBER=%2
        SHIFT
    )
	IF "%1"=="--BUILD_TAG" (
        SET BUILD_TAG=%2
        SHIFT
    )
    SHIFT
    GOTO :parse_command_line
)

..\..\tools\win32\lua\lua52.exe gauntlet-test.lua BUILD_ID=%BUILD_ID% BUILD_NUMBER=%BUILD_NUMBER% BUILD_TAG=%BUILD_TAG%