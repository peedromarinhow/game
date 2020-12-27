@echo off
call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x86 > NUL 2> NUL
REM I hate pink, so no .cpp files on the github repo are allowed
pushd build\debug
copy ..\..\src\*.c *.cpp > NUL 2> NUL
copy ..\..\src\*.h *.h > NUL 2> NUL
cl -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4701 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -FC -Z7 -Fmwin32_main.map win32_main.cpp -link -opt:ref -subsystem:windows,5.1 user32.lib gdi32.lib
popd
