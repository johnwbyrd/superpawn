echo Run me in the root directory of the project to build win32 releases.
echo Use -tests argument to run gauntlet tests on engine.

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
        SET JOB_NAME=%2
        SHIFT
    )
	if "%1"=="--tests" (
		SET TESTS=1
	)
    SHIFT
    GOTO :parse_command_line
)

call "%VS120COMNTOOLS%\vsvars32.bat"
set ZIP_EXE=tools\win32\zip\zip.exe
set ENGINE_NAME=superpawn
rem
mkdir build\win\x86
pushd build\win\x86
rem
cmake -G "Visual Studio 12" ..\..\..
rem
msbuild superpawn.sln /p:Configuration=Debug
if errorlevel 1 goto :fail
msbuild superpawn.sln /p:Configuration=Release
if errorlevel 1 goto :fail
git log -n 50 > release\ChangeLog.txt
..\..\..\%ZIP_EXE% superpawn-windows-x32.zip release\*.*
popd

mkdir build\win\x64
pushd build\win\x64
rem
cmake -G "Visual Studio 12 Win64" ..\..\..
msbuild superpawn.sln /p:Configuration=Debug
if errorlevel 1 goto :fail
msbuild superpawn.sln /p:Configuration=Release
if errorlevel 1 goto :fail
git log -n 50 > release\ChangeLog.txt
..\..\..\%ZIP_EXE% superpawn-windows-x64.zip release\*.*
popd

if "%TESTS%" == "" goto no_tests
pushd tests\gauntlet
call gauntlet-test.bat

:no_tests

goto done

:fail
echo Build FAILED.
exit 1

:done
echo Build completed successfully.