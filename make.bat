set CC="C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc"
set INCLUDE_DIR=sdl2\include
set LIB_DIR=sdl2\lib\x64
set LIBS=-lSDL2main -lSDL2
%CC% -I%INCLUDE_DIR% -L%LIB_DIR% -o engine.exe map.c engine.c -Wall %LIBS%
