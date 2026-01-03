# Generic Synthesis Components
# These are reusable across all synth plugins (RG909, 808, etc.)

CC = gcc
CFLAGS = -fPIC -Wall -O2
LDFLAGS = -lm

# Generic component source files
SOURCES = synth_resonator.c \
          synth_noise.c \
          synth_filter.c \
          synth_filter_ladder.c \
          synth_envelope.c \
          synth_lfo.c \
          synth_oscillator.c \
          synth_voice_manager.c \
          synth_utils.c \
          bass_station.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default target - just compile objects
all: $(OBJECTS)
	@echo "✓ Built generic synth components"

# Compile individual object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS)
	@echo "✓ Cleaned generic synth components"

.PHONY: all clean
