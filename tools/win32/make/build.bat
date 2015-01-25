echo Run me in the root directory of the project to build win32 releases.
echo Use --TESTS argument to run gauntlet tests on engine.

set BUILD_ID=UnknownBuildID
set BUILD_NUMBER=Unknown
set BUILD_TAG=UnknownBuildTag
set BUILD_BRANCH=UnknownBuildBranch

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
	if "%1"=="--TESTS" (
		SET TESTS=1
	)
    SHIFT
    GOTO :parse_command_line
)

call "%VS120COMNTOOLS%\vsvars32.bat"
set ZIP_EXE=tools\win32\zip\zip.exe
set ENGINE_NAME=superpawn
set BUILDINFO=BuildInfo.txt
set CHANGELOG=ChangeLog.txt
rem Get current branch
git rev-parse --abbrev-ref HEAD > git-branch.txt
set /p BUILD_BRANCH=<git-branch.txt
del git-branch.txt
rem
mkdir build\win\x86
pushd build\win\x86
call :create_ancillary_files
rem
cmake -G "Visual Studio 12" ..\..\..
rem
msbuild superpawn.sln /p:Configuration=Debug
if errorlevel 1 goto :fail
msbuild superpawn.sln /p:Configuration=Release
if errorlevel 1 goto :fail
call :create_ancillary_files
copy release\Superpawn.exe release\Superpawn-%BUILD_NUMBER%-x86.exe
copy %BUILDINFO% release /y
copy %CHANGELOG% release /y
cd release
..\..\..\..\%ZIP_EXE% ..\superpawn-windows-x32.zip *.* -x superpawn.exe
popd

mkdir build\win\x64
pushd build\win\x64
call :create_ancillary_files
rem
cmake -G "Visual Studio 12 Win64" ..\..\..
msbuild superpawn.sln /p:Configuration=Debug
if errorlevel 1 goto :fail
msbuild superpawn.sln /p:Configuration=Release
if errorlevel 1 goto :fail
copy release\Superpawn.exe release\Superpawn-%BUILD_NUMBER%-x64.exe
copy %BUILDINFO% release /y
copy %CHANGELOG% release /y
cd release
..\..\..\..\%ZIP_EXE% ..\superpawn-windows-x64.zip *.* -x superpawn.exe
popd

if "%TESTS%" == "" goto no_tests
pushd tests\gauntlet
call gauntlet-test.bat --BUILD_ID %BUILD_ID% --BUILD_NUMBER %BUILD_NUMBER% --BUILD_TAG %BUILD_TAG% --BUILD_BRANCH %BUILD_BRANCH%

:no_tests

goto done

:create_ancillary_files
git log -n 50 > ChangeLog.txt
if exist %BUILDINFO% del %BUILDINFO%
echo This information uniquely identifies the current build of Superpawn. >> %BUILDINFO%
echo Please reference this information when reporting bugs or other issues. >> %BUILDINFO%
echo Current build number: %BUILD_NUMBER% >> %BUILDINFO%
echo Current build ID: %BUILD_ID% >> %BUILDINFO%
exit /b

:fail
echo Build FAILED.
goto die

:done
echo Build completed successfully.

:die
