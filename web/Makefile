# Makefile for WebAssembly build of Regroove Effects
# Requires Emscripten SDK (emsdk)

CC = emcc
CFLAGS = -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
         -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 \
         -s EXPORT_NAME='RegrooveEffectsModule' -I../effects

# Source files
EFFECTS_DIR = ../effects
SOURCES = $(EFFECTS_DIR)/fx_distortion.c \
          $(EFFECTS_DIR)/fx_filter.c \
          $(EFFECTS_DIR)/fx_eq.c \
          $(EFFECTS_DIR)/fx_compressor.c \
          $(EFFECTS_DIR)/fx_delay.c \
          $(EFFECTS_DIR)/fx_reverb.c \
          $(EFFECTS_DIR)/fx_phaser.c \
          $(EFFECTS_DIR)/fx_stereo_widen.c \
          $(EFFECTS_DIR)/fx_model1_trim.c \
          $(EFFECTS_DIR)/fx_model1_hpf.c \
          $(EFFECTS_DIR)/fx_model1_lpf.c \
          $(EFFECTS_DIR)/fx_model1_sculpt.c \
          wasm_bindings.c

OUTPUT = regroove-effects.js

# Exported functions
EXPORTED_FUNCTIONS = \
  -s EXPORTED_FUNCTIONS='[\
    "_malloc", "_free",\
    "_fx_distortion_create", "_fx_distortion_destroy", "_fx_distortion_reset",\
    "_fx_distortion_set_enabled", "_fx_distortion_set_drive", "_fx_distortion_set_mix",\
    "_fx_distortion_get_enabled", "_fx_distortion_get_drive", "_fx_distortion_get_mix",\
    "_fx_distortion_process_f32",\
    "_fx_filter_create", "_fx_filter_destroy", "_fx_filter_reset",\
    "_fx_filter_set_enabled", "_fx_filter_set_cutoff", "_fx_filter_set_resonance",\
    "_fx_filter_get_enabled", "_fx_filter_get_cutoff", "_fx_filter_get_resonance",\
    "_fx_filter_process_f32",\
    "_fx_eq_create", "_fx_eq_destroy", "_fx_eq_reset",\
    "_fx_eq_set_enabled", "_fx_eq_set_low", "_fx_eq_set_mid", "_fx_eq_set_high",\
    "_fx_eq_get_enabled", "_fx_eq_get_low", "_fx_eq_get_mid", "_fx_eq_get_high",\
    "_fx_eq_process_f32",\
    "_fx_compressor_create", "_fx_compressor_destroy", "_fx_compressor_reset",\
    "_fx_compressor_set_enabled", "_fx_compressor_set_threshold", "_fx_compressor_set_ratio",\
    "_fx_compressor_set_attack", "_fx_compressor_set_release", "_fx_compressor_set_makeup",\
    "_fx_compressor_get_enabled", "_fx_compressor_get_threshold", "_fx_compressor_get_ratio",\
    "_fx_compressor_get_attack", "_fx_compressor_get_release", "_fx_compressor_get_makeup",\
    "_fx_compressor_process_f32",\
    "_fx_delay_create", "_fx_delay_destroy", "_fx_delay_reset",\
    "_fx_delay_set_enabled", "_fx_delay_set_time", "_fx_delay_set_feedback", "_fx_delay_set_mix",\
    "_fx_delay_get_enabled", "_fx_delay_get_time", "_fx_delay_get_feedback", "_fx_delay_get_mix",\
    "_fx_delay_process_f32",\
    "_fx_reverb_create", "_fx_reverb_destroy", "_fx_reverb_reset",\
    "_fx_reverb_set_enabled", "_fx_reverb_set_size", "_fx_reverb_set_damping", "_fx_reverb_set_mix",\
    "_fx_reverb_get_enabled", "_fx_reverb_get_size", "_fx_reverb_get_damping", "_fx_reverb_get_mix",\
    "_fx_reverb_process_f32",\
    "_fx_phaser_create", "_fx_phaser_destroy", "_fx_phaser_reset",\
    "_fx_phaser_set_enabled", "_fx_phaser_set_rate", "_fx_phaser_set_depth", "_fx_phaser_set_feedback",\
    "_fx_phaser_get_enabled", "_fx_phaser_get_rate", "_fx_phaser_get_depth", "_fx_phaser_get_feedback",\
    "_fx_phaser_process_f32",\
    "_fx_stereo_widen_create", "_fx_stereo_widen_destroy", "_fx_stereo_widen_reset",\
    "_fx_stereo_widen_set_enabled", "_fx_stereo_widen_set_width", "_fx_stereo_widen_set_mix",\
    "_fx_stereo_widen_get_enabled", "_fx_stereo_widen_get_width", "_fx_stereo_widen_get_mix",\
    "_fx_stereo_widen_process_interleaved",\
    "_fx_model1_trim_create", "_fx_model1_trim_destroy", "_fx_model1_trim_reset",\
    "_fx_model1_trim_set_enabled", "_fx_model1_trim_set_drive",\
    "_fx_model1_trim_get_enabled", "_fx_model1_trim_get_drive", "_fx_model1_trim_get_peak_level",\
    "_fx_model1_trim_process_interleaved",\
    "_fx_model1_hpf_create", "_fx_model1_hpf_destroy", "_fx_model1_hpf_reset",\
    "_fx_model1_hpf_set_enabled", "_fx_model1_hpf_set_cutoff",\
    "_fx_model1_hpf_get_enabled", "_fx_model1_hpf_get_cutoff",\
    "_fx_model1_hpf_process_interleaved",\
    "_fx_model1_lpf_create", "_fx_model1_lpf_destroy", "_fx_model1_lpf_reset",\
    "_fx_model1_lpf_set_enabled", "_fx_model1_lpf_set_cutoff",\
    "_fx_model1_lpf_get_enabled", "_fx_model1_lpf_get_cutoff",\
    "_fx_model1_lpf_process_interleaved",\
    "_fx_model1_sculpt_create", "_fx_model1_sculpt_destroy", "_fx_model1_sculpt_reset",\
    "_fx_model1_sculpt_set_enabled", "_fx_model1_sculpt_set_frequency", "_fx_model1_sculpt_set_gain",\
    "_fx_model1_sculpt_get_enabled", "_fx_model1_sculpt_get_frequency", "_fx_model1_sculpt_get_gain",\
    "_fx_model1_sculpt_process_interleaved"\
  ]'

all: $(OUTPUT)

$(OUTPUT): $(SOURCES)
	$(CC) $(CFLAGS) $(EXPORTED_FUNCTIONS) $(SOURCES) -o $(OUTPUT)
	@echo "🔄 Auto-updating WASM function mappings..."
	@python3 update-wasm-mappings.py || echo "⚠️ Failed to update mappings (continuing anyway)"
	@echo "📦 Copying drop-in files to revision project..."
	@if [ -d ~/Projects/revision ]; then \
		cp regroove-effects.wasm regroove-effects.js audio-worklet-processor.js wasm-mappings.json ~/Projects/revision/ && \
		echo "   ✅ Copied to ~/Projects/revision/"; \
	fi

clean:
	rm -f $(OUTPUT) regroove-effects.wasm

.PHONY: all clean
