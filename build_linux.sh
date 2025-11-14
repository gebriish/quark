#!/usr/bin/env bash

BUILD="./bin"
SRC="src/main.c"
BIN="quark"

CC="clang"
CSTD="-std=c11"

CFLAGS="$CSTD -Wno-override-init -g"
INCLUDES="-I/usr/include/freetype2"
LIBS="-lm -lGL -lfreetype -lglfw3"

rm -rf "$BUILD"
mkdir -p "$BUILD"

echo "Compiling $SRC -> $BUILD/$BIN"
$CC $SRC -o "$BUILD/$BIN" $CFLAGS $INCLUDES $LIBS
