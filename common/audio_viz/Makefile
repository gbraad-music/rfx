# Audio Visualization WASM Build System
# Compiles shared C code to WebAssembly for browser use

EMCC = /opt/emsdk/upstream/emscripten/emcc

# Source files
SRC = audio_viz_wasm.c
TARGET = audio-viz

# Output directory
OUT_DIR = ../../web/external

# Compiler flags
CFLAGS = -O3 \
         -I. \
         -Wall \
         -Wno-unused-function

# Linker flags - CRITICAL: ES6 module format + no name mangling
LDFLAGS = -s WASM=1 \
          -s EXPORT_ES6=1 \
          -s MODULARIZE=1 \
          -s EXPORT_NAME="AudioVizModule" \
          -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall","getValue","setValue"]' \
          -s ALLOW_MEMORY_GROWTH=1 \
          -s ENVIRONMENT=web,worker \
          -s EXPORT_ALL=1

# Build targets
all: $(OUT_DIR)/$(TARGET).js

$(OUT_DIR)/$(TARGET).js: $(SRC) vu_meter.h waveform.h
	@echo "Building WASM module with ES6 exports..."
	$(EMCC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)
	@echo "Built: $(OUT_DIR)/$(TARGET).js ($(shell stat -c%s $(OUT_DIR)/$(TARGET).js) bytes)"
	@echo "Built: $(OUT_DIR)/$(TARGET).wasm ($(shell stat -c%s $(OUT_DIR)/$(TARGET).wasm) bytes)"
	@echo "ES6 module format - use: import AudioVizModule from './audio-viz.js'"

clean:
	rm -f $(OUT_DIR)/$(TARGET).js $(OUT_DIR)/$(TARGET).wasm

.PHONY: all clean
