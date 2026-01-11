#!/bin/bash
# Build MOD player test tool
set -e
echo "Building MOD player test tool..."
gcc -o mod_player_test mod_player_test.c \
    ../../synth/mod_player.c \
    ../../synth/tracker_voice.c \
    -I../../synth -lSDL2 -lm -O2 -Wall
echo "Build complete: mod_player_test"
echo ""
echo "Usage: ./mod_player_test <file.mod> [-o output.wav]"
