#!/bin/bash
# Build MED player test natively (Linux) using GCC and SDL2

set -e

CC=gcc
CFLAGS="-O2 -Wall -I../.. -I../../synth $(pkg-config --cflags sdl2)"
LDFLAGS="$(pkg-config --libs sdl2) -lm"

echo "Building MED player test for Linux..."
echo "CC: $CC"
echo "CFLAGS: $CFLAGS"
echo "LDFLAGS: $LDFLAGS"
echo

$CC $CFLAGS -o med_player_test \
    med_player_test.c \
    ../../synth/med_player.c \
    $LDFLAGS

if [ $? -eq 0 ]; then
    echo "✓ Built med_player_test"
    echo
    echo "Usage: ./med_player_test <filename.med>"
else
    echo "✗ Build failed"
    exit 1
fi
