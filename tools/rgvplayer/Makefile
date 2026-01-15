# Makefile for RGVPlayer - RegrooveController Test Tool

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../players -I../../synth
LDFLAGS = -lm -lSDL2

# Source files
PLAYER_SRCS = \
	../../players/deck_player.c \
	../../players/mod_player.c \
	../../players/mmd_player.c \
	../../players/ahx_player.c \
	../../players/pattern_sequencer.c \
	../../players/regroove_controller.c \
	../../players/tracker_voice.c \
	../../players/tracker_mixer.c \
	../../players/tracker_modulator.c \
	../../players/tracker_sequence.c

SYNTH_SRCS = \
	../../synth/synth_envelope.c \
	../../synth/synth_sample_player.c \
	../../synth/ahx_synth_core.c \
	../../synth/ahx_instrument.c \
	../../synth/ahx_preset.c

APP_SRCS = \
	main.c

ALL_SRCS = $(PLAYER_SRCS) $(SYNTH_SRCS) $(APP_SRCS)

# Object files
OBJS = $(ALL_SRCS:.c=.o)

# Output binary
TARGET = rgvplayer

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "✓ Built: $(TARGET)"
	@echo "  Usage: ./$(TARGET) <file.mod|file.mmd|file.ahx>"
	@echo ""

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned build artifacts"

rebuild: clean all

# Install to bin directory (optional)
install: $(TARGET)
	mkdir -p ../../bin
	cp $(TARGET) ../../bin/
	@echo "Installed to ../../bin/$(TARGET)"

.PHONY: all clean rebuild install
