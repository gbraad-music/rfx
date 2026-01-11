#!/bin/bash
# Build MMD player test natively (Linux) using GCC and SDL2
# Builds with synth support enabled (SYNTHETIC/HYBRID instruments)

set -e

CC=gcc
CFLAGS="-O2 -Wall -I../.. -I../../synth -DMMD_SYNTH_SUPPORT $(pkg-config --cflags sdl2)"
LDFLAGS="$(pkg-config --libs sdl2) -lm"

echo "Building MMD player test for Linux (with synth support)..."
echo "CC: $CC"
echo "CFLAGS: $CFLAGS"
echo "LDFLAGS: $LDFLAGS"
echo

$CC $CFLAGS -o med_player_test \
    med_player_test.c \
    ../../synth/mmd_player.c \
    ../../synth/tracker_voice.c \
    ../../synth/synth_oscillator.c \
    ../../synth/synth_envelope.c \
    ../../synth/synth_lfo.c \
    $LDFLAGS

if [ $? -eq 0 ]; then
    echo "✓ Built med_player_test (MMD player with synth support)"
    echo
    echo "Usage: ./med_player_test <filename.med>"
else
    echo "✗ Build failed"
    exit 1
fi
