#!/usr/bin/env bash
set -e

build="bin"
bin="quark"
src="src/main.c"

mode="${1:-debug}"

mkdir -p "$build"

CFLAGS="-std=c11 -lX11 -lXrandr -lGL -lm"

case "$mode" in
    debug)
        CFLAGS="$CFLAGS -O0 -g"
        ;;
    release)
        CFLAGS="$CFLAGS -O2"
        ;;
    *)
        echo "unknown build mode: $mode"
        echo "usage: ./build.sh [debug|release]"
        exit 1
        ;;
esac

echo "compiling ($src) -> $build/$bin [$mode]"
clang $src -o $build/$bin $CFLAGS
