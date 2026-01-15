cmake_minimum_required(VERSION 3.10)
project(MEDPlayerTest VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optional features
# Enable OctaMED synth instruments (SYNTHETIC/HYBRID types)
# To enable: cmake -DMMD_SYNTH_SUPPORT=ON ..
option(MMD_SYNTH_SUPPORT "Enable OctaMED synthetic instruments" OFF)

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
)

# MMD Player Test executable sources
set(MEDPLAYER_SOURCES
    med_player_test.c
    ../../players/mmd_player.c
    ../../players/tracker_voice.c
    ../../players/tracker_mixer.c
    ../../players/pattern_sequencer.c
)

# Add synth sources if MMD_SYNTH_SUPPORT is enabled
if(MMD_SYNTH_SUPPORT)
    message(STATUS "MMD_SYNTH_SUPPORT enabled - including synth components")
    list(APPEND MEDPLAYER_SOURCES
        ../../synth/synth_oscillator.c
        ../../synth/synth_envelope.c
        ../../synth/synth_lfo.c
    )
    add_definitions(-DMMD_SYNTH_SUPPORT)
endif()

# Choose executable name based on synth support
if(MMD_SYNTH_SUPPORT)
    set(EXECUTABLE_NAME med_player_synth)
else()
    set(EXECUTABLE_NAME med_player_test)
endif()

add_executable(${EXECUTABLE_NAME} ${MEDPLAYER_SOURCES})

# Link libraries
if(CMAKE_CROSSCOMPILING)
    target_link_libraries(${EXECUTABLE_NAME} ${SDL2_LIBRARIES} -lm)
else()
    target_link_libraries(${EXECUTABLE_NAME} ${SDL2_LIBRARIES} m)
endif()

# Windows-specific settings
if(WIN32)
    # Link MinGW standard libraries statically
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        LINK_FLAGS "-static-libgcc -mconsole"
    )
endif()

# Installation
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
