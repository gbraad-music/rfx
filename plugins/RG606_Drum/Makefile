#!/usr/bin/make -f

ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RG606_Drum
TARGETS = lv2_sep vst3

FILES_DSP = \
	RG606_DrumPlugin.cpp \
	../../synth/synth_utils.c \
	../../synth/synth_oscillator.c \
	../../synth/synth_noise.c \
	../../synth/synth_filter.c \
	../../synth/synth_envelope.c \
	../../synth/synth_voice_manager.c

FILES_UI = \
	RG606_DrumUI.cpp \
	../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../synth
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../synth

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
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
