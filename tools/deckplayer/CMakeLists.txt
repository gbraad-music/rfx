cmake_minimum_required(VERSION 3.10)
project(deckplayer VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable synth support
add_definitions(-DMMD_SYNTH_SUPPORT)

# For cross-compilation, manually specify SDL2
if(CMAKE_CROSSCOMPILING)
    # MinGW SDL2 paths
    set(SDL2_INCLUDE_DIRS "${CMAKE_FIND_ROOT_PATH}/include/SDL2")
    set(SDL2_LIBRARIES "-lmingw32 -lSDL2main -lSDL2")

    include_directories(${SDL2_INCLUDE_DIRS})
else()
    # Native compilation - use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)

    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# Source files
set(DECKPLAYER_SOURCES
    deckplayer.c
    # Player components (moved to players/)
    ../../players/deck_player.c
    ../../players/mod_player.c
    ../../players/mmd_player.c
    ../../players/ahx_player.c
    ../../players/sid_player.c
    ../../players/pattern_sequencer.c
    ../../players/tracker_mixer.c
    ../../players/tracker_voice.c
    ../../players/tracker_modulator.c
    ../../players/tracker_sequence.c
    # Synth components (remain in synth/)
    ../../synth/synth_oscillator.c
    ../../synth/synth_envelope.c
    ../../synth/synth_lfo.c
    ../../synth/synth_sample_player.c
    ../../synth/synth_sid.c
    ../../synth/ahx_synth_core.c
    ../../synth/ahx_instrument.c
    ../../synth/ahx_preset.c
    ../../synth/ahx_plist.c
    # CPU emulator for SID
    ../../common/cpu_6502.c
)

# Create executable
add_executable(deckplayer ${DECKPLAYER_SOURCES})

# Link libraries
if(CMAKE_CROSSCOMPILING)
    target_link_libraries(deckplayer ${SDL2_LIBRARIES} -lm)
else()
    target_link_libraries(deckplayer ${SDL2_LIBRARIES} m)
endif()

# Windows-specific settings
if(WIN32)
    # Link MinGW standard libraries statically
    set_target_properties(deckplayer PROPERTIES
        LINK_FLAGS "-static-libgcc -mconsole"
    )
endif()

# Install target
install(TARGETS deckplayer DESTINATION bin)
