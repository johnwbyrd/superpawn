echo Run me in the root directory of the project to build win32 releases.
echo Use -tests argument to run gauntlet tests on engine.

set BUILD_ID=UnknownBuildID
set BUILD_NUMBER=Unknown
set BUILD_TAG=UnknownBuildTag

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
set BUILDINFO=release\BuildInfo.txt
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
call :create_ancillary_files
move release\Superpawn.exe release\Superpawn-%BUILD_NUMBER%-x86.exe
cd release
..\..\..\..\%ZIP_EXE% ..\superpawn-windows-x32.zip *.*
popd

mkdir build\win\x64
pushd build\win\x64
rem
cmake -G "Visual Studio 12 Win64" ..\..\..
msbuild superpawn.sln /p:Configuration=Debug
if errorlevel 1 goto :fail
msbuild superpawn.sln /p:Configuration=Release
if errorlevel 1 goto :fail
call :create_ancillary_files
move release\Superpawn.exe release\Superpawn-%BUILD_NUMBER%-x64.exe
cd release
..\..\..\..\%ZIP_EXE% ..\superpawn-windows-x64.zip *.*
popd

if "%TESTS%" == "" goto no_tests
pushd tests\gauntlet
call gauntlet-test.bat --BUILD_ID %BUILD_ID% --BUILD_NUMBER %BUILD_NUMBER% --BUILD_TAG %BUILD_TAG%

:no_tests

goto done

:create_ancillary_files
git log -n 50 > release\ChangeLog.txt
if exist %BUILDINFO% del %BUILDINFO%
echo This information uniquely identifies this build of Superpawn. >> %BUILDINFO%
echo Please reference this information when reporting bugs or other issues. >> %BUILDINFO%
echo --- >> %BUILDINFO%
echo Build number: %BUILD_NUMBER% >> %BUILDINFO%
echo Build ID:     %BUILD_ID% >> %BUILDINFO%
echo Build tag:    %BUILD_TAG% >> %BUILDINFO%
exit /b

:fail
echo Build FAILED.
goto die

:done
echo Build completed successfully.

:die
