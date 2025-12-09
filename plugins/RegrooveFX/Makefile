#!/usr/bin/make -f
# Makefile for RegrooveFX Plugin
# This uses DPF's native Makefile system (simpler than CMake for DPF)

# Include DPF Makefile framework
include ../../dpf/Makefile.base.mk

# Plugin name
NAME = RegrooveFX

# Files to build
FILES_DSP = \
	RegrooveFXPlugin.cpp \
	../../regroove_effects.c

FILES_UI = \
	RegrooveFXUI.cpp

# Build targets
TARGETS = lv2_sep vst2 vst3 clap jack

# Include directories
BUILD_CXX_FLAGS += -I../..

# Enable DGL/ImGui
DGL_FLAGS = -DDGL_USE_IMGUI
BUILD_CXX_FLAGS += $(DGL_FLAGS)

# Link math library
LINK_FLAGS += -lm

# Include DPF plugin makefile
include ../../dpf/Makefile.plugins.mk

# Custom target: fetch DPF if not present
dpf-check:
	@if [ ! -d "../../dpf" ]; then \
		echo "DPF not found. Cloning..."; \
		git clone --depth 1 --branch v1.7 https://github.com/DISTRHO/DPF.git ../../dpf; \
	fi

.PHONY: dpf-check

# Override default target to check for DPF first
all: dpf-check
	@$(MAKE) -f Makefile $(TARGETS)
