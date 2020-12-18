@echo off
call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x64
REM I hate pink, so no .cpp files on the github repo are allowed
copy src\*.c *.cpp

pushd build\debug
del *.pdb > NUL 2> NUL
cl /Zi ..\..\win32_main.cpp user32.lib gdi32.lib
popd