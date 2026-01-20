#!/usr/bin/make -f

# Set Windows cross-compiler when WINDOWS=true
ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
export CC CXX
endif

include ../../dpf/Makefile.base.mk

NAME = RFX_AmigaFilter
TARGETS = lv2_sep vst3

FILES_DSP = \
	RFX_AmigaPlugin.cpp \
	../../effects/fx_amiga_filter.c

FILES_UI = \
	RFX_AmigaUI.cpp \
	../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../effects
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../effects

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
LINK_FLAGS += -static-libgcc -static-libstdc++ -lopengl32 -lgdi32
else
LINK_FLAGS += -lGL
endif

LINK_FLAGS += -lm

# Fix Windows .def file handling for modern MinGW
ifeq ($(WINDOWS),true)
SYMBOLS_VST3 = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
endif

include ../../dpf/Makefile.plugins.mk

lv2_sep: $(lv2_dsp) $(lv2_ui)
	cp ../../assets/icon-192x192.png ../../bin/$(NAME).lv2/icon.png 2>/dev/null || true

vst3: $(vst3)
	@echo "RFX Amiga VST3 built"

.PHONY: install
install: vst3
	mkdir -p ../../bin
	cp $(vst3) ../../bin/ || true
	@echo "RFX Amiga installed to ../../bin/"
