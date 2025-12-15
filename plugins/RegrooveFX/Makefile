#!/usr/bin/make -f
# Makefile for RegrooveFX Plugin - WITH UI
# Builds: Linux LV2 (with UI), Windows VST3 (with UI)

# Set Windows cross-compiler when WINDOWS=true
ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
export CC CXX
endif

# Include DPF Makefile framework
include ../../dpf/Makefile.base.mk

# Plugin name
NAME = RegrooveFX

# Files to build - DSP
FILES_DSP = \
	RegrooveFXPlugin.cpp \
	../../regroove_effects.c \
	../../effects/fx_distortion.c \
	../../effects/fx_filter.c \
	../../effects/fx_eq.c \
	../../effects/fx_compressor.c \
	../../effects/fx_delay.c

# UI files - using DPF-Widgets DearImGui
# Note: DearImGui.cpp includes all ImGui, ImGuiKnobs, and ImGuiToggle sources
FILES_UI = \
	RegrooveFXUI.cpp \
	../DearImGui.cpp

# UI Type - use default OpenGL2 for better Windows compatibility
# ifeq ($(WINDOWS),true)
# UI_TYPE = opengl3
# endif

# Build targets - with UI
ifeq ($(WINDOWS),true)
    # Windows: VST3 with UI
    TARGETS = vst3
else
    # Linux: LV2 with UI (lv2_sep = separate DSP/UI) and VST3
    TARGETS = lv2_sep vst3
endif

# Include directories - prioritize DearImGui to avoid header conflicts
BUILD_C_FLAGS += -I../.. -I../../effects
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../effects

# Define RFX_BUILD_UI=1 for UI build
BUILD_C_FLAGS += -DRFX_BUILD_UI=1
BUILD_CXX_FLAGS += -DRFX_BUILD_UI=1

# Windows-specific flags
ifeq ($(WINDOWS),true)
    BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
    BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
    LINK_FLAGS += -static-libgcc -static-libstdc++
    # OpenGL for ImGui
    LINK_FLAGS += -lopengl32 -lgdi32
else
    # Linux OpenGL for ImGui
    LINK_FLAGS += -lGL
endif

# Link math library
LINK_FLAGS += -lm

# Include DPF plugin makefile
include ../../dpf/Makefile.plugins.mk

# Fix Windows .def file handling for modern MinGW - use version script instead
ifeq ($(WINDOWS),true)
SYMBOLS_VST3 = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
endif

# Default target
all: $(TARGETS)

# Post-build: copy icon for LV2
.PHONY: install-icon
install-icon:
	@if [ -f ../../assets/icon-192x192.png ] && [ -d $(TARGET_DIR)/$(NAME).lv2 ]; then \
		cp ../../assets/icon-192x192.png $(TARGET_DIR)/$(NAME).lv2/icon.png; \
	fi

.PHONY: all lv2_sep vst3
