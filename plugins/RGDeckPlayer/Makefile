#!/usr/bin/make -f

ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RGDeckPlayer
TARGETS = lv2_sep vst3

FILES_DSP = \
	RGDeckPlayerPlugin.cpp \
	../../players/deck_player.c \
	../../players/mod_player.c \
	../../players/mmd_player.c \
	../../players/ahx_player.c \
	../../players/sid_player.c \
	../../players/pattern_sequencer.c \
	../../players/tracker_sequence.c \
	../../players/tracker_mixer.c \
	../../players/tracker_voice.c \
	../../players/tracker_modulator.c \
	../../synth/ahx_synth_core.c \
	../../synth/ahx_waves.c \
	../../synth/ahx_instrument.c \
	../../synth/ahx_preset.c \
	../../synth/ahx_plist.c \
	../../synth/synth_sample_player.c \
	../../synth/synth_sid.c \
	../../common/cpu_6502.c

FILES_UI = \
	RGDeckPlayerUI.cpp \
	../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../players -I../../synth -I../../common
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../players -I../../synth -I../../common

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f -fno-builtin-expf
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f -fno-builtin-expf
LINK_FLAGS += -static-libgcc -static-libstdc++ -lopengl32 -lgdi32
else
LINK_FLAGS += -lGL
endif

LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk

ifeq ($(WINDOWS),true)
SYMBOLS_VST3 = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
endif

lv2_sep: $(lv2_dsp) $(lv2_ui)
	cp ../../assets/icon-192x192.png ../../bin/$(NAME).lv2/icon.png 2>/dev/null || true

.PHONY: all
