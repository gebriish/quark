BUILD="./bin"
BIN="quark"

rm -rf $BUILD
mkdir $BUILD

clang src/main.c -o "$BUILD/$BIN" -std=gnu11 -lm -lX11 -lXrandr -lGL

