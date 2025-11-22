#!/usr/bin/env bash

BUILD="./bin"
SRC="src/main.c"
BIN="quark"

CC="clang"
CSTD="-std=c11"

CFLAGS="$CSTD -Wno-override-init -g -O2"
GLFW_LIB="src/thirdparty/glfw/build/src/libglfw3.a"

SYSLIBS="-lm -lGL -ldl -lpthread -lX11 -lXcursor"

rm -rf "$BUILD"
mkdir -p "$BUILD"

echo "Compiling $SRC -> $BUILD/$BIN"
$CC $SRC -o "$BUILD/$BIN" $CdLAGS $GLFW_LIB $SYSLIBS
