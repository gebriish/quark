BUILD="./bin"
BIN="editor"

rm -rf $BUILD
mkdir $BUILD

clang src/main.c -o "$BUILD/$BIN" -std=gnu11 -lm -lX11 -lXrandr -lGL

