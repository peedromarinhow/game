@echo off
call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x64

set COMPILATION_FLAGS=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4189 -wd4100 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DGAME_WIN32 -FC -Z7
set LINK_FLAGS=-I"C:\raylib\raylib\src" -link -incremental:no -opt:ref -MAP "C:\raylib\raylib\src\lib\raylib.lib" kernel32.lib user32.lib shell32.lib winmm.lib gdi32.lib opengl32.lib

pushd build\debug
del *.pdb > NUL 2> NUL
cl.exe %COMPILATION_FLAGS% ..\..\game.cpp -Fmgame -I"C:\raylib\raylib\src" -LD -link -incremental:no -PDB:"game_%date:~-4,4%%date:~-10,2%%date:~-7,2%%time:~0,2%%time:~3,2%%time:~6,2%.pdb" "C:\raylib\raylib\src\lib\raylib.lib" -EXPORT:GameUpdateAndRender
cl.exe %COMPILATION_FLAGS% ..\..\win32_main.cpp -Fmwin32_main %LINK_FLAGS%
popd