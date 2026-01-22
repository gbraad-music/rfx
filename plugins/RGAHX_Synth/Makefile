#!/usr/bin/make -f

ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RGAHX_Synth
TARGETS = lv2_sep vst3

FILES_DSP = \
	RGAHX_SynthPlugin.cpp \
	../../synth/ahx_synth_core.c \
	../../synth/ahx_instrument.c \
	../../synth/ahx_preset.c \
	../../synth/ahx_plist.c \
	../../synth/tracker_voice.c \
	../../synth/tracker_modulator.c \
	../../synth/tracker_sequence.c \
	../../synth/synth_midi.c

FILES_UI = \
	RGAHX_SynthUI.cpp \
	../DearImGui.cpp

BUILD_C_FLAGS += -I../.. -I../../synth
BUILD_CXX_FLAGS += -I../DearImGui -I../DearImGuiKnobs -I../DearImGuiToggle
BUILD_CXX_FLAGS += -I.. -I../.. -I../../synth

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
              -s EXPORT_NAME='RGAHXSynthModule' \
              -I../.. -I../../synth

# WebAssembly source files
WASM_SOURCES = ../../synth/ahx_synth_core.c \
               ../../synth/ahx_instrument.c \
               ../../synth/ahx_plist.c \
               ../../synth/ahx_preset.c \
               ../../synth/synth_midi.c \
               ../../players/tracker_voice.c \
               ../../players/tracker_modulator.c \
               ../../players/tracker_sequence.c \
               wasm_bindings.c

# Exported functions for JavaScript (regroove_synth_* interface)
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
    "_regroove_synth_get_plist_length",\
    "_regroove_synth_get_plist_speed",\
    "_regroove_synth_set_plist_speed",\
    "_regroove_synth_get_plist_entry",\
    "_regroove_synth_set_plist_entry",\
    "_regroove_synth_add_plist_entry",\
    "_regroove_synth_remove_plist_entry",\
    "_regroove_synth_clear_plist",\
    "_regroove_synth_export_preset",\
    "_regroove_synth_free_preset_buffer",\
    "_regroove_synth_import_preset"\
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
