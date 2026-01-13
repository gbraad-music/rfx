#!/usr/bin/make -f

ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RGAHX_Synth
TARGETS = lv2_dsp vst3

FILES_DSP = \
	RGAHX_SynthPlugin.cpp \
	../../synth/ahx_instrument.c \
	../../synth/tracker_voice.c \
	../../synth/tracker_modulator.c \
	../../synth/tracker_sequence.c \
	../../synth/synth_midi.c

BUILD_C_FLAGS += -I../.. -I../../synth
BUILD_CXX_FLAGS += -I.. -I../.. -I../../synth

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f -fno-builtin-expf
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f -fno-builtin-expf
LINK_FLAGS += -static-libgcc -static-libstdc++
endif

LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk

ifeq ($(WINDOWS),true)
SYMBOLS_VST3 = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
endif

lv2_dsp: $(lv2_dsp)
	cp ../../assets/icon-192x192.png ../../bin/$(NAME).lv2/icon.png 2>/dev/null || true

# ==============================================================================
# WebAssembly Build Target (for web use)
# ==============================================================================

# WebAssembly compiler and flags
EMCC = emcc
WASM_CFLAGS = -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 \
              -s EXPORT_NAME='RGAHXSynthModule' \
              -I../.. -I../../synth

# WebAssembly source files
WASM_SOURCES = ../../synth/ahx_instrument.c \
               ../../synth/tracker_voice.c \
               ../../synth/tracker_modulator.c \
               ../../synth/tracker_sequence.c \
               ../../synth/synth_midi.c \
               wasm_bindings.c

# Exported functions for JavaScript
WASM_EXPORTED_FUNCTIONS = \
  -s EXPORTED_FUNCTIONS='[\
    "_malloc", "_free",\
    "_ahx_instrument_init",\
    "_ahx_instrument_set_params",\
    "_ahx_instrument_get_params",\
    "_ahx_instrument_note_on",\
    "_ahx_instrument_note_off",\
    "_ahx_instrument_process",\
    "_ahx_instrument_is_active",\
    "_ahx_instrument_reset",\
    "_ahx_instrument_default_params"\
  ]'

WASM_OUTPUT = rgahxsynth.js

# WebAssembly build target
web: $(WASM_SOURCES)
	@echo "Building RGAHX_Synth for WebAssembly..."
	$(EMCC) $(WASM_CFLAGS) $(WASM_EXPORTED_FUNCTIONS) $(WASM_SOURCES) -o $(WASM_OUTPUT)
	@echo "Built: $(WASM_OUTPUT) and rgahxsynth.wasm"
	@echo "Copying to web/synths directory..."
	@mkdir -p ../../web/synths
	@cp rgahxsynth.wasm rgahxsynth.js ../../web/synths/
	@echo "   ✅ Copied to ../../web/synths/"

clean-web:
	rm -f $(WASM_OUTPUT) rgahxsynth.wasm

.PHONY: web clean-web
