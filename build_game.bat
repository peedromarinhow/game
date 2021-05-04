@echo off

set application_name= app
set BuildOptions= -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1
set CompileFlags= -nologo -FC -FS -Zi -MTd -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4701 -wd4244 -wd4505 -I ../src/
set CommonLinkerFlags= -incremental:no -opt:ref opengl32.lib
set PlatformLinkFlags= %CommonLinkerFlags% gdi32.lib user32.lib winmm.lib

call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x86 > NUL 2> NUL

if not exist build_game mkdir build_game

pushd build_game
del *.pdb > NUL 2> NUL
cl.exe %BuildOptions% %CompileFlags% ../src/game/app.cpp       -Fmapp.map /LD    /link %CommonLinkerFlags% /PDB:"app_%random%.pdb"
cl.exe %BuildOptions% %CompileFlags% ../src/win32/win32_main.c -Fmwin32_main.map /link %PlatformLinkFlags% /PDB:"main.pdb"
popd
