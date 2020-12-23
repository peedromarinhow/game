@echo off
call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x64
REM I hate pink, so no .cpp files on the github repo are allowed
pushd build\debug
copy ..\..\src\*.c *.cpp
copy ..\..\src\*.h *.h

del *.pdb > NUL 2> NUL
cl /Zi win32_main.cpp user32.lib gdi32.lib
popd
