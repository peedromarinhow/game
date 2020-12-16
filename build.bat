@echo off
call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x64

set COMPILATION_FLAGS=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4189 -wd4100 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DGAME_WIN32 -FC -Z7
set LINK_FLAGS= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib
ren *.c *.cpp

pushd build\debug
del *.pdb > NUL 2> NUL
cl.exe %COMPILATION_FLAGS% ..\..\win32_main.cpp -Fmwin32_main -link %LINK_FLAGS%
popd

ren *.cpp *.c