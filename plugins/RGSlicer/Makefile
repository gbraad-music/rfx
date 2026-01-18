#!/usr/bin/make -f

# Set Windows cross-compiler when WINDOWS=true
ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RGSlicer
TARGETS = lv2_sep vst3

FILES_DSP = \
	RGSlicerPlugin.cpp \
	../../synth/rgslicer.c \
	../../synth/rgslicer_detect.c \
	../../synth/rgslicer_sfz.c \
	../../synth/sample_fx.c \
	../../synth/synth_sample_player.c \
	../../synth/synth_envelope.c \
	../../synth/sfz_parser.c \
	../../synth/wav_cue.c

# UI implementation
FILES_UI = RGSlicerUI.cpp ../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../synth -I../../effects
BUILD_CXX_FLAGS += -I.. -I../.. -I../../synth -I../DearImGui -I../DearImGuiKnobs

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
LINK_FLAGS += -static-libgcc -static-libstdc++
endif

LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk

# Fix Windows .def file handling for modern MinGW
ifeq ($(WINDOWS),true)
SYMBOLS_VST3 = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
endif

lv2_sep: $(lv2_dsp) $(lv2_ui)
	@echo "RGSlicer LV2 built"

vst3: $(vst3)
	@echo "RGSlicer VST3 built"

.PHONY: install
install: lv2_sep vst3
	mkdir -p ../../bin
	cp -r $(lv2_dsp) ../../bin/ || true
	cp $(vst3) ../../bin/ || true
	@echo "RGSlicer installed to ../../bin/"

# ==============================================================================
# WebAssembly Build Target (for web use)
# ==============================================================================

# WebAssembly compiler and flags
EMCC = emcc
WASM_CFLAGS = -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 \
              -s EXPORT_NAME='RGSlicerModule' \
              -I../.. -I../../synth

# WebAssembly source files
WASM_SOURCES = ../../synth/rgslicer.c \
               ../../synth/rgslicer_detect.c \
               ../../synth/rgslicer_sfz.c \
               ../../synth/sample_fx.c \
               ../../synth/synth_sample_player.c \
               ../../synth/synth_envelope.c \
               ../../synth/sfz_parser.c \
               ../../synth/wav_cue.c \
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
    "_synth_get_buffer_size_bytes",\
    "_rgslicer_load_wav_from_memory",\
    "_rgslicer_get_slice_count",\
    "_rgslicer_get_slice_offset_at",\
    "_rgslicer_get_slice_length_at",\
    "_rgslicer_set_slices_from_cues",\
    "_rgslicer_set_slices_with_notes",\
    "_rgslicer_add_slice"\
  ]'

WASM_OUTPUT = rgslicer.js

# WebAssembly build target
web: $(WASM_SOURCES)
	@echo "Building RGSlicer for WebAssembly..."
	$(EMCC) $(WASM_CFLAGS) $(WASM_EXPORTED_FUNCTIONS) $(WASM_SOURCES) -o $(WASM_OUTPUT)
	@echo "Built: $(WASM_OUTPUT) and rgslicer.wasm"
	@echo "Copying to web/synths directory..."
	@mkdir -p ../../web/synths
	@cp rgslicer.wasm rgslicer.js ../../web/synths/
	@echo "   Copied to ../../web/synths/"

clean-web:
	rm -f $(WASM_OUTPUT) rgslicer.wasm

.PHONY: web clean-web
