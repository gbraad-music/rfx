# Python Tools Build System
# Builds librg909.so for Python ctypes wrapper (testing/tweaking only)
# For actual VST3/LV2 plugins, use plugins/RG909_Drum/Makefile

CC = gcc
CFLAGS = -shared -fPIC -Wall -O2 -I.. -I../synth
LDFLAGS = -lm

SYNTH_DIR = ../synth

# RG909-specific sources
RG909_SOURCES = $(SYNTH_DIR)/rg909_drum_synth.c \
                $(SYNTH_DIR)/synth_resonator.c \
                $(SYNTH_DIR)/synth_noise.c \
                $(SYNTH_DIR)/synth_filter.c \
                $(SYNTH_DIR)/synth_envelope.c \
                $(SYNTH_DIR)/synth_voice_manager.c \
                $(SYNTH_DIR)/synth_utils.c

# Target library for Python wrapper
TARGET = librg909.so

# Test program
TEST = test_c_kick

all: $(TARGET) $(TEST)
	@echo "✓ Built Python wrapper library and test program"

# Build shared library for Python
$(TARGET): $(RG909_SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(RG909_SOURCES) $(LDFLAGS)
	@echo "✓ Built $(TARGET)"

# Build test program
$(TEST): test_c_kick.c $(TARGET)
	$(CC) -o $(TEST) test_c_kick.c -I.. -I../synth ./$(TARGET) -lm
	@echo "✓ Built $(TEST)"

# Run test
test: $(TEST)
	./$(TEST) 0.96 0.13 100

# Clean
clean:
	rm -f $(TARGET) $(TEST) test_c_kick.wav
	@echo "✓ Cleaned Python wrapper artifacts"

.PHONY: all test clean
