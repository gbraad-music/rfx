#\!/bin/bash
# Build AHX player test tool
set -e
echo "Building AHX player test tool..."
gcc -o ahx_player_test ahx_player_test.c \
    ../../synth/ahx_player.c \
    ../../synth/tracker_modulator.c \
    ../../synth/tracker_sequence.c \
    ../../synth/tracker_voice.c \
    -I../../synth -lSDL2 -lm -O2 -Wall
echo "Build complete: ahx_player_test"
echo ""
echo "Usage: ./ahx_player_test <file.ahx> [-o output.wav] [-c channels] [-s subsong]"
