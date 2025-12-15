#!/usr/bin/make -f
# Makefile for RegrooveM1 Plugin - MODEL 1 Channel Strip
# Builds: Linux LV2 (for Mixxx), Windows VST3

# Set Windows cross-compiler when WINDOWS=true
ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
export CC CXX
endif

# Include DPF Makefile framework
include ../../dpf/Makefile.base.mk

# Plugin name
NAME = RegrooveM1

# Files to build - DSP
FILES_DSP = \
	RegrooveM1Plugin.cpp \
	../../effects/fx_model1_lpf.c \
	../../effects/fx_model1_hpf.c \
	../../effects/fx_model1_sculpt.c

# UI files - using DPF-Widgets DearImGui with ImGuiKnobs
FILES_UI = \
	RegrooveM1UI.cpp \
	../DearImGui.cpp

# Build targets
ifeq ($(WINDOWS),true)
    # Windows: VST3 only
    TARGETS = vst3
else
    # Linux: LV2 (for Mixxx) and VST3
    TARGETS = lv2_sep vst3
endif

# Include directories - prioritize DearImGui to avoid header conflicts
BUILD_C_FLAGS += -I../.. -I../../effects
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../effects

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
