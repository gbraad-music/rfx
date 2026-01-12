# If RACK_DIR is not defined when calling the Makefile, default to standard location
RACK_DIR ?= $(HOME)/Applications/VCVRack/Rack-SDK

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I../../synth
FLAGS += -DMMD_SYNTH_SUPPORT
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Tell make where to find source files
VPATH += ../../synth

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add tracker player C files
SOURCES += ../../synth/mod_player.c
SOURCES += ../../synth/mmd_player.c
SOURCES += ../../synth/ahx_player.c

# Add shared tracker components
SOURCES += ../../synth/tracker_mixer.c
SOURCES += ../../synth/tracker_voice.c
SOURCES += ../../synth/tracker_modulator.c
SOURCES += ../../synth/tracker_sequence.c

# Add OctaMED synth support components
SOURCES += ../../synth/synth_oscillator.c
SOURCES += ../../synth/synth_envelope.c
SOURCES += ../../synth/synth_lfo.c

# Add dependencies
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
