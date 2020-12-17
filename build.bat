@echo off
call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x64

REM set COMPILATION_FLAGS=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4189 -wd4100 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DGAME_WIN32 -FC -Z7
REM set LINK_FLAGS= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM I hate pink, so no .cpp files on the github repo are allowed
ren *.c *.cpp

pushd build\debug
del *.pdb > NUL 2> NUL
REM cl.exe %COMPILATION_FLAGS% ..\..\win32_main.cpp -Fmwin32_main -link %LINK_FLAGS%
cl /Zi ..\..\win32_main.cpp user32.lib gdi32.lib
popd

ren *.cpp *.c