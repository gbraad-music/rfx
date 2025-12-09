# RegrooveFX Plugin CMake Build
# Uses DPF (DISTRHO Plugin Framework)

cmake_minimum_required(VERSION 3.15)

# DPF uses a Makefile-based build system, so we'll wrap it with CMake
# For a CMake-native approach, we need to manually configure DPF

# Get DPF root directory from parent
if(NOT DEFINED DPF_ROOT_DIR)
    message(FATAL_ERROR "DPF_ROOT_DIR not set. Please run CMake from the project root.")
endif()

# Set plugin name
set(PLUGIN_NAME RegrooveFX)

# Source files
set(PLUGIN_SOURCES
    RegrooveFXPlugin.cpp
    RegrooveFXUI.cpp
)

# DPF core sources
file(GLOB DPF_SOURCES
    ${DPF_ROOT_DIR}/distrho/DistrhoPluginMain.cpp
    ${DPF_ROOT_DIR}/distrho/DistrhoUIMain.cpp
)

# ImGui sources (from DPF)
file(GLOB IMGUI_SOURCES
    ${DPF_ROOT_DIR}/dgl/src/pugl.cpp
    ${DPF_ROOT_DIR}/dgl/src/OpenGL.cpp
    ${DPF_ROOT_DIR}/dgl/src/Color.cpp
    ${DPF_ROOT_DIR}/dgl/src/ImageBase.cpp
    ${DPF_ROOT_DIR}/dgl/src/NanoVG.cpp
    ${DPF_ROOT_DIR}/dgl/src/Resources.cpp
    ${DPF_ROOT_DIR}/dgl/src/Widget.cpp
    ${DPF_ROOT_DIR}/dgl/src/Window.cpp
)

# Add ImGui from DPF
file(GLOB IMGUI_CORE_SOURCES
    ${DPF_ROOT_DIR}/dgl/src/sofd/libsofd.c
)

# Include directories
set(PLUGIN_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DPF_ROOT_DIR}/distrho
    ${DPF_ROOT_DIR}/dgl
    ${CMAKE_SOURCE_DIR}  # For regroove_effects.h
)

# Platform-specific settings
if(APPLE)
    set(PLUGIN_SUFFIX ".vst")
    set(DGL_SYSTEM_LIBS "-framework Cocoa -framework OpenGL")
elseif(WIN32)
    set(PLUGIN_SUFFIX ".vst3")
    set(DGL_SYSTEM_LIBS opengl32 gdi32)
else()
    set(PLUGIN_SUFFIX ".so")
    find_package(X11 REQUIRED)
    set(DGL_SYSTEM_LIBS ${X11_LIBRARIES} GL)
    list(APPEND PLUGIN_INCLUDES ${X11_INCLUDE_DIR})
endif()

# VST3 Plugin target
add_library(${PLUGIN_NAME}_vst3 MODULE
    ${PLUGIN_SOURCES}
    ${DPF_SOURCES}
)

target_include_directories(${PLUGIN_NAME}_vst3 PRIVATE
    ${PLUGIN_INCLUDES}
)

target_link_libraries(${PLUGIN_NAME}_vst3 PRIVATE
    regroove_effects
    ${DGL_SYSTEM_LIBS}
)

target_compile_definitions(${PLUGIN_NAME}_vst3 PRIVATE
    DISTRHO_PLUGIN_TARGET_VST3
)

set_target_properties(${PLUGIN_NAME}_vst3 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${PLUGIN_NAME}"
    SUFFIX "${PLUGIN_SUFFIX}"
)

# LV2 Plugin target
add_library(${PLUGIN_NAME}_lv2 MODULE
    ${PLUGIN_SOURCES}
    ${DPF_SOURCES}
)

target_include_directories(${PLUGIN_NAME}_lv2 PRIVATE
    ${PLUGIN_INCLUDES}
)

target_link_libraries(${PLUGIN_NAME}_lv2 PRIVATE
    regroove_effects
    ${DGL_SYSTEM_LIBS}
)

target_compile_definitions(${PLUGIN_NAME}_lv2 PRIVATE
    DISTRHO_PLUGIN_TARGET_LV2
)

set_target_properties(${PLUGIN_NAME}_lv2 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${PLUGIN_NAME}"
    SUFFIX ".so"
)

# Standalone target (optional)
add_executable(${PLUGIN_NAME}_standalone
    ${PLUGIN_SOURCES}
    ${DPF_SOURCES}
)

target_include_directories(${PLUGIN_NAME}_standalone PRIVATE
    ${PLUGIN_INCLUDES}
)

target_link_libraries(${PLUGIN_NAME}_standalone PRIVATE
    regroove_effects
    ${DGL_SYSTEM_LIBS}
)

target_compile_definitions(${PLUGIN_NAME}_standalone PRIVATE
    DISTRHO_PLUGIN_TARGET_JACK
)

# Installation
install(TARGETS ${PLUGIN_NAME}_vst3 ${PLUGIN_NAME}_lv2
    LIBRARY DESTINATION lib/vst3
    RUNTIME DESTINATION bin
)

message(STATUS "RegrooveFX plugin configured")
message(STATUS "  VST3: ${PLUGIN_NAME}_vst3")
message(STATUS "  LV2:  ${PLUGIN_NAME}_lv2")
message(STATUS "  Standalone: ${PLUGIN_NAME}_standalone")
