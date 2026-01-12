cmake_minimum_required(VERSION 3.10)
project(deckplayer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# Enable synth support
add_definitions(-DMMD_SYNTH_SUPPORT)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Source files
set(DECKPLAYER_SOURCES
    deckplayer.c
    ../../synth/deck_player.c
    ../../synth/mod_player.c
    ../../synth/mmd_player.c
    ../../synth/ahx_player.c
    ../../synth/tracker_mixer.c
    ../../synth/tracker_voice.c
    ../../synth/tracker_modulator.c
    ../../synth/tracker_sequence.c
    ../../synth/synth_oscillator.c
    ../../synth/synth_envelope.c
    ../../synth/synth_lfo.c
)

# Create executable
add_executable(deckplayer ${DECKPLAYER_SOURCES})

# Link libraries
target_link_libraries(deckplayer m ${SDL2_LIBRARIES})

# Install target
install(TARGETS deckplayer DESTINATION bin)
