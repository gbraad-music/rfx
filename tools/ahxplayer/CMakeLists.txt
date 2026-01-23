cmake_minimum_required(VERSION 3.10)
project(AHXPlayerTest VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../players
    ${CMAKE_CURRENT_SOURCE_DIR}/../../synth
)

# AHX Player Test executable
add_executable(ahx_player_test
    ahx_player_test.c
    ../../players/ahx_player.c
    ../../synth/ahx_synth_core.c
    ../../synth/ahx_waves.c
    ../../synth/ahx_plist.c
    ../../players/tracker_modulator.c
    ../../players/tracker_sequence.c
    ../../players/tracker_voice.c
    ../../players/tracker_mixer.c
    ../../players/pattern_sequencer.c
)

# Link libraries
if(CMAKE_CROSSCOMPILING)
    target_link_libraries(ahx_player_test ${SDL2_LIBRARIES} -lm)
else()
    target_link_libraries(ahx_player_test ${SDL2_LIBRARIES} m)
endif()

# Windows-specific settings
if(WIN32)
    # Link MinGW standard libraries statically
    set_target_properties(ahx_player_test PROPERTIES
        LINK_FLAGS "-static-libgcc -mconsole"
    )
endif()

# Installation
install(TARGETS ahx_player_test DESTINATION bin)
