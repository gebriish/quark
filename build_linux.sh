#!/usr/bin/env bash

BUILD="./bin"
SRC="src/main.c"
BIN="quark"

CC="clang"
CSTD="-std=gnu11"

CFLAGS="$CSTD -Wconversion -Wsign-conversion -Wfloat-conversion -Wno-override-init"
INCLUDES="-I/usr/include/freetype2"
LIBS="-lm -lGL -lfreetype -lglfw3"

rm -rf "$BUILD"
mkdir -p "$BUILD"

echo "Compiling $SRC -> $BUILD/$BIN"
$CC $SRC -o "$BUILD/$BIN" $CFLAGS $INCLUDES $LIBS
