@echo off
setlocal

set BUILD=bin
set BIN=quark.exe

if exist %BUILD% (
    rmdir /s /q %BUILD%
)


mkdir %BUILD%

zig cc -g -o bin\quark.exe src\main.c -lopengl32 -lgdi32

endlocal
