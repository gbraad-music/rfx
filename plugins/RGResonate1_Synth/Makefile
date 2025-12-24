#!/usr/bin/make -f

# Set Windows cross-compiler when WINDOWS=true
ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RGResonate1_Synth
TARGETS = lv2_sep vst3

FILES_DSP = \
	RGResonate1_SynthPlugin.cpp \
	../../synth/synth_resonate1.c \
	../../synth/synth_voice_manager.c \
	../../synth/synth_oscillator.c \
	../../synth/synth_envelope.c \
	../../synth/synth_filter_ladder.c \
	../../synth/synth_utils.c

FILES_UI = \
	RGResonate1_SynthUI.cpp \
	../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../synth
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../synth

# UI Type - use default OpenGL2 for better Windows compatibility
# ifeq ($(WINDOWS),true)
# UI_TYPE = opengl3
# endif

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
LINK_FLAGS += -static-libgcc -static-libstdc++ -lopengl32 -lgdi32
else
LINK_FLAGS += -lGL
endif

LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk

# Fix Windows .def file handling for modern MinGW
ifeq ($(WINDOWS),true)
SYMBOLS_VST3 = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
endif

lv2_sep: $(lv2_dsp) $(lv2_ui)
	cp ../../assets/icon-192x192.png ../../bin/$(NAME).lv2/icon.png 2>/dev/null || true

# ==============================================================================
# WebAssembly Build Target (for ../revision/ and web use)
# ==============================================================================

# WebAssembly compiler and flags
EMCC = emcc
WASM_CFLAGS = -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 \
              -s EXPORT_NAME='RGResonate1Module' \
              -I../.. -I../../synth

# WebAssembly source files
WASM_SOURCES = ../../synth/synth_resonate1.c \
               ../../synth/synth_voice_manager.c \
               ../../synth/synth_oscillator.c \
               ../../synth/synth_envelope.c \
               ../../synth/synth_filter_ladder.c \
               ../../synth/synth_utils.c \
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

WASM_OUTPUT = rgresonate1-synth.js

# WebAssembly build target
web: $(WASM_SOURCES)
	@echo "Building RGResonate1 for WebAssembly..."
	$(EMCC) $(WASM_CFLAGS) $(WASM_EXPORTED_FUNCTIONS) $(WASM_SOURCES) -o $(WASM_OUTPUT)
	@echo "Built: $(WASM_OUTPUT) and rgresonate1-synth.wasm"
	@echo "Copying to revision project synths/ directory..."
	@if [ -d ~/Projects/revision ]; then \
		mkdir -p ~/Projects/revision/synths && \
		cp rgresonate1-synth.wasm rgresonate1-synth.js ~/Projects/revision/synths/ && \
		echo "   ✅ Copied to ~/Projects/revision/synths/"; \
	fi

clean-web:
	rm -f $(WASM_OUTPUT) rgresonate1-synth.wasm

.PHONY: web clean-web
