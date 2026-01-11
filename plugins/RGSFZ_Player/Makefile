#!/usr/bin/make -f

ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RGSFZ_Player
TARGETS = vst2

FILES_DSP = \
	RGSFZ_PlayerPlugin.cpp \
	../../synth/sfz_parser.c \
	../../synth/synth_sample_player.c

FILES_UI = \
	RGSFZ_PlayerUI.cpp \
	../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../synth
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../synth

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f -mcmodel=large
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f -mcmodel=large
LINK_FLAGS += -static-libgcc -static-libstdc++ -lopengl32 -lgdi32 -mcmodel=large
else
LINK_FLAGS += -lGL
endif

LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk
