#!/usr/bin/make -f

ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RG909_Drum
TARGETS = lv2_sep vst3

FILES_DSP = \
	RG909_DrumPlugin.cpp \
	../../synth/synth_utils.c \
	../../synth/synth_oscillator.c \
	../../synth/synth_noise.c \
	../../synth/synth_filter.c \
	../../synth/synth_envelope.c \
	../../synth/synth_voice_manager.c

FILES_UI = \
	RG909_DrumUI.cpp \
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

# ==============================================================================
# WebAssembly Build Target
# ==============================================================================

EMCC = emcc
WASM_CFLAGS = -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 \
              -s EXPORT_NAME='RG909Module' \
              -I../.. -I../../synth

# WebAssembly source files (core synth only, no DPF/UI)
WASM_SOURCES = ../../synth/synth_oscillator.c \
               ../../synth/synth_noise.c \
               ../../synth/synth_filter.c \
               ../../synth/synth_envelope.c \
               ../../synth/synth_voice_manager.c \
               ../../synth/synth_utils.c \
               wasm_bindings.c

# Exported functions for JavaScript
WASM_EXPORTED_FUNCTIONS = \
  -s EXPORTED_FUNCTIONS='[\
    "_malloc", "_free",\
    "_rg909_create",\
    "_rg909_destroy",\
    "_rg909_reset",\
    "_rg909_trigger_drum",\
    "_rg909_process_f32",\
    "_rg909_set_parameter",\
    "_rg909_get_parameter"\
  ]'

WASM_OUTPUT = rg909-drum.js

web: $(WASM_SOURCES)
	@echo "Building RG909 Drum for WebAssembly..."
	$(EMCC) $(WASM_CFLAGS) $(WASM_EXPORTED_FUNCTIONS) $(WASM_SOURCES) -o $(WASM_OUTPUT)
	@echo "Built: $(WASM_OUTPUT) and rg909-drum.wasm"
	@echo "Copying to revision project synths/ directory..."
	@if [ -d ~/Projects/revision ]; then \
		mkdir -p ~/Projects/revision/synths && \
		cp rg909-drum.wasm rg909-drum.js ~/Projects/revision/synths/ && \
		echo "   ✅ Copied to ~/Projects/revision/synths/"; \
	fi

clean-web:
	rm -f $(WASM_OUTPUT) rg909-drum.wasm

.PHONY: web clean-web
