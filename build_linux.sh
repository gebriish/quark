#!/usr/bin/env bash

build="bin"
bin="quark"
src="src/main.c"


GLFW_LIB="src/thirdparty/glfw/build/src/libglfw3.a"
SYSLIBS="-lm -lGL -ldl -lpthread -lX11 -lXcursor"

rm -rf "$build"
mkdir -p "$build"

echo "Compiling $src -> $build/$bin"
clang $src -o $build/$bin $GLFW_LIB $SYSLIBS -std=c11 -g -Winitializer-overrides
