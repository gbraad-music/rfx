#!/usr/bin/make -f

ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RG1Piano
TARGETS = lv2_sep vst3

FILES_DSP = \
	RG1PianoPlugin.cpp \
	../../synth/synth_sample_player.c \
	../../data/rg1piano/sample_data.c

FILES_UI = \
	RG1PianoUI.cpp \
	../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../synth -I../../data/rg1piano
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../synth -I../../data/rg1piano

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
