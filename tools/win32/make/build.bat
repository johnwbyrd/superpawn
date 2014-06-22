echo Run me in the root directory of the project to build win32 releases.

call "%VS120COMNTOOLS%\vsvars32.bat"
set ZIP_EXE=tools\win32\zip\zip.exe
set ENGINE_NAME=superpawn
rem
mkdir build\win\x86
cd build\win\x86
rem
cmake -G "Visual Studio 12" ..\..\..
rem
msbuild superpawn.sln /p:Configuration=Debug
if errorlevel 1 goto :fail
msbuild superpawn.sln /p:Configuration=Release
if errorlevel 1 goto :fail
git log -n 50 > release\ChangeLog.txt
..\..\..\%ZIP_EXE% superpawn-windows-x32.zip release\*.*
cd ..\..\..

mkdir build\win\x64
cd build\win\x64
rem
cmake -G "Visual Studio 12 Win64" ..\..\..
msbuild superpawn.sln /p:Configuration=Debug
if errorlevel 1 goto :fail
msbuild superpawn.sln /p:Configuration=Release
if errorlevel 1 goto :fail
git log -n 50 > release\ChangeLog.txt
..\..\..\%ZIP_EXE% superpawn-windows-x64.zip release\*.*


cd ..\..\..
goto done

:fail
echo Build FAILED.
exit 1

:done
echo Build completed successfully.