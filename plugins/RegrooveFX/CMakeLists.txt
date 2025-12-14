cmake_minimum_required(VERSION 3.15)
project(RegrooveFX VERSION 1.0.0)

# Options
option(RFX_BUILD_UI "Build with UI (ImGui)" ON)
option(RFX_BUILD_LV2 "Build LV2 plugin" ON)
option(RFX_BUILD_VST3 "Build VST3 plugin" ON)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# DPF path
set(DPF_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dpf")

# Include DPF
add_subdirectory(${DPF_ROOT_DIR} dpf EXCLUDE_FROM_ALL)

# Plugin metadata
set(PLUGIN_NAME "RegrooveFX")
set(PLUGIN_BRAND "Regroove")
set(PLUGIN_URI "https://github.com/gbraad/rfx")

# DSP source files (always needed)
set(DSP_SOURCES
    RegrooveFXPlugin.cpp
    ../../regroove_effects.c
    ../../effects/fx_distortion.c
    ../../effects/fx_filter.c
    ../../effects/fx_eq.c
    ../../effects/fx_compressor.c
    ../../effects/fx_delay.c
)

# UI source files (only if UI enabled)
set(UI_SOURCES
    RegrooveFXUI.cpp
)

# Include directories
set(PLUGIN_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../effects
)

if(RFX_BUILD_UI)
    list(APPEND PLUGIN_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../DearImGui
        ${CMAKE_CURRENT_SOURCE_DIR}/../DearImGuiKnobs
        ${CMAKE_CURRENT_SOURCE_DIR}/../DearImGuiToggle
    )

    # Additional UI sources for DPF-Widgets
    list(APPEND UI_SOURCES
        ../DearImGui.cpp
    )
endif()

# Compile definitions
set(PLUGIN_DEFINITIONS
    DISTRHO_PLUGIN_BRAND="${PLUGIN_BRAND}"
    DISTRHO_PLUGIN_NAME="${PLUGIN_NAME}"
    DISTRHO_PLUGIN_URI="${PLUGIN_URI}"
)

if(RFX_BUILD_UI)
    list(APPEND PLUGIN_DEFINITIONS RFX_BUILD_UI=1)
else()
    list(APPEND PLUGIN_DEFINITIONS RFX_BUILD_UI=0)
endif()

# Windows-specific flags
if(WIN32 OR MINGW)
    add_compile_options(-fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f)
    if(MINGW)
        add_link_options(-static-libgcc -static-libstdc++)
    endif()
endif()

# Create plugin target using DPF
dpf_add_plugin(${PLUGIN_NAME}
    TARGETS lv2 vst3
    FILES_DSP ${DSP_SOURCES}
    FILES_UI ${UI_SOURCES}
)

# Set include directories
target_include_directories(${PLUGIN_NAME} PRIVATE ${PLUGIN_INCLUDES})

# Set compile definitions
target_compile_definitions(${PLUGIN_NAME} PRIVATE ${PLUGIN_DEFINITIONS})

# Link math library
target_link_libraries(${PLUGIN_NAME} PRIVATE m)

# Windows OpenGL for UI
if(RFX_BUILD_UI AND WIN32)
    target_link_libraries(${PLUGIN_NAME} PRIVATE opengl32 gdi32)
elseif(RFX_BUILD_UI AND UNIX AND NOT APPLE)
    target_link_libraries(${PLUGIN_NAME} PRIVATE GL)
endif()

message(STATUS "RegrooveFX Configuration:")
message(STATUS "  Build UI: ${RFX_BUILD_UI}")
message(STATUS "  Target: ${CMAKE_SYSTEM_NAME}")
