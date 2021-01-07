@echo off

call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x86 > NUL 2> NUL

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4701 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1 -FC -Z7
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

pushd build\debug

REM I hate pink, so no .cpp files on the github repo are allowed
copy ..\..\src\*.c *.cpp > NUL 2> NUL
copy ..\..\src\*.h *.h > NUL 2> NUL

REM 32bit build
REM cl %CommonCompilerFlags% win32_main.cpp -link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64bit build
cl %CommonCompilerFlags% game.cpp -Fmgame.map -LD -link -PDB:"game_%date:~-4,4%%date:~-10,2%%date:~-7,2%%time:~0,2%%time:~3,2%%time:~6,2%.pdb" -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% win32_main.cpp -Fmwin32_main.map -link %CommonLinkerFlags%
popd
