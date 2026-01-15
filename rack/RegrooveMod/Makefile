# If RACK_DIR is not defined when calling the Makefile, default to standard location
RACK_DIR ?= $(HOME)/Applications/VCVRack/Rack-SDK

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I../../players
FLAGS += -I../../synth
FLAGS += -DMMD_SYNTH_SUPPORT
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Tell make where to find source files
VPATH += ../../players
VPATH += ../../synth

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add tracker player C files (moved to players/)
SOURCES += ../../players/mod_player.c
SOURCES += ../../players/mmd_player.c
SOURCES += ../../players/ahx_player.c
SOURCES += ../../players/pattern_sequencer.c

# Add shared tracker components (moved to players/)
SOURCES += ../../players/tracker_mixer.c
SOURCES += ../../players/tracker_voice.c
SOURCES += ../../players/tracker_modulator.c
SOURCES += ../../players/tracker_sequence.c

# Add synth support components (remain in synth/)
SOURCES += ../../synth/synth_oscillator.c
SOURCES += ../../synth/synth_envelope.c
SOURCES += ../../synth/synth_lfo.c
SOURCES += ../../synth/synth_sample_player.c
SOURCES += ../../synth/ahx_synth_core.c
SOURCES += ../../synth/ahx_instrument.c
SOURCES += ../../synth/ahx_preset.c

# Add dependencies
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
