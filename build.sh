#!/bin/bash

MODE="debug"
FLAGS=(-O0 -g2)

if [ "$1" = "release" ]; then
	MODE="release"
	FLAGS=(-O3 -DNDEBUG)
fi

echo "[clang compile]"
echo "[$MODE mode]"
echo "compiling quark..."

mkdir -p bin

if clang ./src/quark/quark_main.c -o ./bin/quark \
	-I ./src -I /usr/include/freetype2 \
	-lm -lfreetype -lX11 -lXext -lGL \
	"${FLAGS[@]}"; then
echo "@info: quark build complete."
else
	echo "@error: quark build failed."
	exit 1
fi
