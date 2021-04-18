@echo off

set application_name= app
set BuildOptions= -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1
set CompileFlags= -nologo -FC -FS -Zi -MTd -Gm- -GR- -EHa- -Od -Oi -Ob1 -WX -W4 -wd4201 -wd4100 -wd4189 -wd4701 -wd4244 -wd4245 -wd4505 -I ../src/ -I D:/freetype-2.10.4/include
set CommonLinkerFlags= -incremental:no -opt:ref opengl32.lib
set AppLinkerFlags= %CommonLinkerFlags% D:\code\platform-layer\src\libs\freetype.lib
set PlatformLinkFlags= %CommonLinkerFlags% gdi32.lib user32.lib winmm.lib

call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x86 > NUL 2> NUL

if not exist build_text mkdir build_text

echo AAAAAAAAAAAAAAAAAAAAA

pushd build_text
del *.pdb > NUL 2> NUL
cl.exe %BuildOptions% %CompileFlags% ../src/text_editor/app.c  -Fmapp.map /LD    /link %AppLinkerFlags%    /PDB:"app_%random%.pdb"
cl.exe %BuildOptions% %CompileFlags% ../src/win32/win32_main.c -Fmwin32_main.map /link %PlatformLinkFlags% /PDB:"main.pdb"
popd
