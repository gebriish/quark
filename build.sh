#!/bin/bash

MODE="debug"

WARN_FLAGS=(-Wconversion -Wsign-conversion -Wshorten-64-to-32 -Wimplicit-int-conversion -Wfloat-conversion)

FLAGS=(-O0 -g2 "${WARN_FLAGS[@]}")

if [ "$1" = "release" ]; then
	MODE="release"
	FLAGS=(-O3 -DNDEBUG "${WARN_FLAGS[@]}")
fi

echo "[clang compile]"
echo "[$MODE mode]"
echo "compiling quark..."

mkdir -p bin

if clang ./src/quark/main.c -o ./bin/quark \
	-I ./src -I /usr/include/freetype2 \
	-lm -lfreetype -lX11 -lXext -lGL \
	"${FLAGS[@]}"; then
echo "@info: quark build complete."
else
	echo "@error: quark build failed."
	exit 1
fi
