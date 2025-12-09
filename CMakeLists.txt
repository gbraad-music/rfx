cmake_minimum_required(VERSION 3.15)
project(RegrooveFX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

# Options
option(BUILD_PLUGIN "Build the DPF plugin" ON)
option(BUILD_MODULAR "Build individual effect modules" ON)
option(BUILD_LEGACY_WRAPPER "Build legacy regroove_effects wrapper" ON)

# Individual effect modules
if(BUILD_MODULAR)
    add_library(fx_distortion STATIC
        effects/fx_distortion.c
    )
    target_include_directories(fx_distortion PUBLIC effects)

    add_library(fx_filter STATIC
        effects/fx_filter.c
    )
    target_include_directories(fx_filter PUBLIC effects)
    target_link_libraries(fx_filter PUBLIC m)  # For powf()

    add_library(fx_eq STATIC
        effects/fx_eq.c
    )
    target_include_directories(fx_eq PUBLIC effects)
    target_link_libraries(fx_eq PUBLIC m)  # For powf(), expf()

    add_library(fx_compressor STATIC
        effects/fx_compressor.c
    )
    target_include_directories(fx_compressor PUBLIC effects)
    target_link_libraries(fx_compressor PUBLIC m)  # For sqrtf(), expf(), powf()

    add_library(fx_delay STATIC
        effects/fx_delay.c
    )
    target_include_directories(fx_delay PUBLIC effects)

    # MODEL 1 effects
    add_library(fx_model1_lpf STATIC
        effects/fx_model1_lpf.c
    )
    target_include_directories(fx_model1_lpf PUBLIC effects)
    target_link_libraries(fx_model1_lpf PUBLIC m)  # For powf(), cosf(), sinf()

    add_library(fx_model1_hpf STATIC
        effects/fx_model1_hpf.c
    )
    target_include_directories(fx_model1_hpf PUBLIC effects)
    target_link_libraries(fx_model1_hpf PUBLIC m)  # For powf(), cosf(), sinf()

    add_library(fx_model1_sculpt STATIC
        effects/fx_model1_sculpt.c
    )
    target_include_directories(fx_model1_sculpt PUBLIC effects)
    target_link_libraries(fx_model1_sculpt PUBLIC m)  # For powf(), cosf(), sinf()

    # Combined library (all individual modules)
    add_library(regroove_effects_modular INTERFACE)
    target_link_libraries(regroove_effects_modular INTERFACE
        fx_distortion
        fx_filter
        fx_eq
        fx_compressor
        fx_delay
        fx_model1_lpf
        fx_model1_hpf
        fx_model1_sculpt
    )
endif()

# Legacy wrapper (optional, for backward compatibility)
if(BUILD_LEGACY_WRAPPER)
    add_library(regroove_effects STATIC
        regroove_effects.c
    )

    target_include_directories(regroove_effects PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        effects
    )

    # Link to individual modules if available
    if(BUILD_MODULAR)
        target_link_libraries(regroove_effects PRIVATE regroove_effects_modular)
    endif()
endif()

# Build the plugin if enabled
if(BUILD_PLUGIN)
    # Check if DPF exists as a submodule
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dpf/Makefile.base.mk")
        message(STATUS "DPF not found, fetching from git...")

        # Download DPF
        include(FetchContent)
        FetchContent_Declare(
            DPF
            GIT_REPOSITORY https://github.com/DISTRHO/DPF.git
            GIT_TAG        v1.7
        )
        FetchContent_MakeAvailable(DPF)
        set(DPF_ROOT_DIR ${dpf_SOURCE_DIR})
    else()
        set(DPF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dpf)
    endif()

    # Add the plugin subdirectory
    add_subdirectory(plugins/RegrooveFX)
endif()
