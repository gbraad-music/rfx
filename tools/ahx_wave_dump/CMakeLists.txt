cmake_minimum_required(VERSION 3.10)
project(ahx_wave_dump VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../synth
    ${CMAKE_CURRENT_SOURCE_DIR}/../../players
)

# AHX Wave Dump Tool executable
add_executable(ahx_wave_dump
    ahx_wave_dump.c
    ../../synth/ahx_preset.c
    ../../synth/ahx_instrument.c
    ../../synth/ahx_plist.c
    ../../synth/ahx_synth_core.c
    ../../synth/ahx_waves.c
    ../../players/tracker_voice.c
    ../../players/tracker_modulator.c
)

# Link libraries
target_link_libraries(ahx_wave_dump m)

# Windows-specific settings
if(WIN32)
    # Link MinGW standard libraries statically
    set_target_properties(ahx_wave_dump PROPERTIES
        LINK_FLAGS "-static-libgcc -mconsole"
    )
endif()

# Installation
install(TARGETS ahx_wave_dump DESTINATION bin)
