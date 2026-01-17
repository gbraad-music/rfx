#!/usr/bin/make -f

# Set Windows cross-compiler when WINDOWS=true
ifeq ($(WINDOWS),true)
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
endif

include ../../dpf/Makefile.base.mk

NAME = RGSID_Synth
TARGETS = lv2_sep vst3

FILES_DSP = \
	RGSID_SynthPlugin.cpp \
	SIDSysEx.cpp \
	../../synth/synth_sid.c \
	../../synth/synth_sid_cc.c \
	../../synth/synth_lfo.c

FILES_UI = \
	RGSID_SynthUI.cpp \
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
# WebAssembly Build Target (for web use)
# ==============================================================================

# WebAssembly compiler and flags
EMCC = emcc
WASM_CFLAGS = -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 \
              -s EXPORT_NAME='RGSIDSynthModule' \
              -I../.. -I../../synth -I../../common

# WebAssembly source files
WASM_SOURCES = ../../synth/synth_sid.c \
               ../../synth/synth_sid_cc.c \
               ../../synth/synth_lfo.c \
               ../../synth/synth_sid_midibox.c \
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
    "_regroove_synth_handle_midi",\
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
    "_regroove_synth_get_preset_count",\
    "_regroove_synth_get_preset_name",\
    "_regroove_synth_load_preset"\
  ]'

WASM_OUTPUT = rgsidsynth.js

# WebAssembly build target
web: $(WASM_SOURCES)
	@echo "Building RGSID_Synth for WebAssembly..."
	$(EMCC) $(WASM_CFLAGS) $(WASM_EXPORTED_FUNCTIONS) $(WASM_SOURCES) -o $(WASM_OUTPUT)
	@echo "Built: $(WASM_OUTPUT) and rgsidsynth.wasm"
	@echo "Copying to web/synths directory..."
	@mkdir -p ../../web/synths
	@cp rgsidsynth.wasm rgsidsynth.js ../../web/synths/
	@echo "   ✅ Copied to ../../web/synths/"

clean-web:
	rm -f $(WASM_OUTPUT) rgsidsynth.wasm

.PHONY: web clean-web
