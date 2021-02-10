@echo off

REM call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x86 > NUL 2> NUL
REM 
REM set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4701 -wd4244 -wd4505 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1 -FC -Z7
REM set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib
REM 
REM pushd build\debug
REM 
REM cl %CommonCompilerFlags% ../../newsrc/app.c        -Fmapp.map -LD    -link -incremental:no opengl32.lib -PDB:"app_%random%.pdb" -EXPORT:AppGetSoundSamples -EXPORT:AppUpdateAndRender
REM cl %CommonCompilerFlags% ../../newsrc/win32/main.c -Fmwin32_main.map -link %CommonLinkerFlags%  /SUBSYSTEM:WINDOWS
REM 
REM popd

@echo off

set application_name=app
set build_options= -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1
set compile_flags= /nologo /Zi /FC /I ../src/
set common_link_flags= opengl32.lib -opt:ref -incremental:no /Debug:fastlink
set platform_link_flags= gdi32.lib user32.lib winmm.lib %common_link_flags%

call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x86 > NUL 2> NUL

if not exist build mkdir build

pushd build
del *.pdb > NUL 2> NUL
cl.exe %build_options% %compile_flags% ../src/win32/main.c /link %platform_link_flags% /out:main.exe
cl.exe %build_options% %compile_flags% ../src/app.c /LD /link %common_link_flags% /out:app.dll /PDB:"app_%random%.pdb"
popd