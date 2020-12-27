@echo off
call "D:\ProgramData\VisualStudio\VC\Auxiliary\Build\vcvarsall.bat" x64 > NUL 2> NUL
REM I hate pink, so no .cpp files on the github repo are allowed
pushd build\debug
copy ..\..\src\*.c *.cpp > NUL 2> NUL
copy ..\..\src\*.h *.h > NUL 2> NUL

del *.pdb > NUL 2> NUL
cl -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 /nologo /Zi win32_main.cpp user32.lib gdi32.lib
popd
