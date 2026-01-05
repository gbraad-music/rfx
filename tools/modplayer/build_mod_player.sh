#!/bin/bash
# Build MOD player test for Windows using MinGW

set -e

CC=x86_64-w64-mingw32-gcc
CFLAGS="-O2 -Wall -I../.. -I../../synth"
LDFLAGS="-lmingw32 -lSDL2main -lSDL2 -lm -static-libgcc -mconsole"

# Check for SDL2 (adjust path if needed)
SDL2_INCLUDE="/usr/x86_64-w64-mingw32/include/SDL2"
SDL2_LIB="/usr/x86_64-w64-mingw32/lib"

if [ -d "$SDL2_INCLUDE" ]; then
    CFLAGS="$CFLAGS -I$SDL2_INCLUDE"
fi

if [ -d "$SDL2_LIB" ]; then
    LDFLAGS="-L$SDL2_LIB $LDFLAGS"
fi

echo "Building MOD player test for Windows..."
echo "CC: $CC"
echo "CFLAGS: $CFLAGS"
echo "LDFLAGS: $LDFLAGS"
echo

$CC $CFLAGS -o mod_player_test.exe \
    mod_player_test.c \
    ../../synth/mod_player.c \
    $LDFLAGS

if [ $? -eq 0 ]; then
    echo "✓ Built mod_player_test.exe"
    echo
    echo "Usage: mod_player_test.exe <filename.mod>"
else
    echo "✗ Build failed"
    exit 1
fi
