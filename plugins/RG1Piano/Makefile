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
	../../synth/synth_modal_piano.c \
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

# ==============================================================================
# WebAssembly Build Target (for web use)
# ==============================================================================

# WebAssembly compiler and flags
EMCC = emcc
WASM_CFLAGS = -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 \
              -s EXPORT_NAME='RG1PianoModule' \
              -I../.. -I../../synth -I../../data/rg1piano

# WebAssembly source files
WASM_SOURCES = ../../synth/synth_sample_player.c \
               ../../synth/synth_modal_piano.c \
               ../../data/rg1piano/sample_data.c \
               wasm_bindings.c

# Exported functions for JavaScript
WASM_EXPORTED_FUNCTIONS = \
  -s EXPORTED_FUNCTIONS='[\
    "_malloc", "_free",\
    "_regroove_synth_create",\
    "_regroove_synth_destroy",\
    "_regroove_synth_reset",\
    "_regroove_synth_note_on",\
    "_regroove_synth_note_off",\
    "_regroove_synth_control_change",\
    "_regroove_synth_pitch_bend",\
    "_regroove_synth_all_notes_off",\
    "_regroove_synth_process_f32",\
    "_regroove_synth_get_parameter_count",\
    "_regroove_synth_get_parameter",\
    "_regroove_synth_set_parameter",\
    "_regroove_synth_get_parameter_name",\
    "_regroove_synth_get_parameter_label",\
    "_regroove_synth_get_parameter_default",\
    "_regroove_synth_get_parameter_min",\
    "_regroove_synth_get_parameter_max",\
    "_regroove_synth_get_parameter_group",\
    "_regroove_synth_get_group_name",\
    "_regroove_synth_parameter_is_integer",\
    "_regroove_synth_get_engine",\
    "_regroove_synth_get_engine_name",\
    "_synth_create_audio_buffer",\
    "_synth_destroy_audio_buffer",\
    "_synth_get_buffer_size_bytes"\
  ]'

WASM_OUTPUT = rg1piano.js

# WebAssembly build target
web: $(WASM_SOURCES)
	@echo "Building RG1Piano for WebAssembly..."
	$(EMCC) $(WASM_CFLAGS) $(WASM_EXPORTED_FUNCTIONS) $(WASM_SOURCES) -o $(WASM_OUTPUT)
	@echo "Built: $(WASM_OUTPUT) and rg1piano.wasm"
	@echo "Copying to web directory..."
	@mkdir -p ../../web
	@cp rg1piano.wasm rg1piano.js ../../web/
	@echo "   ✅ Copied to ../../web/"

clean-web:
	rm -f $(WASM_OUTPUT) rg1piano.wasm

.PHONY: web clean-web
