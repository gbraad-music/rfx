# If RACK_DIR is not defined when calling the Makefile, default to standard location
RACK_DIR ?= $(HOME)/Applications/VCVRack/Rack-SDK

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I../../effects
CFLAGS +=
CXXFLAGS +=

# Use local aubio from dep directory for spectral analysis
# Compiles aubio source directly into the plugin for cross-platform compatibility
# This enables RGB spectral waveform coloring (low/mid/high frequency bands)
AUBIO_SRC_DIR = dep/aubio-src
AUBIO_HEADERS = $(wildcard $(AUBIO_SRC_DIR)/aubio.h)
ifneq ($(AUBIO_HEADERS),)
FLAGS += -DUSE_AUBIO
FLAGS += -I$(AUBIO_SRC_DIR)
FLAGS += -DHAVE_CONFIG_H
FLAGS += -DHAVE_AUBIO_DOUBLE=0
# Add all aubio C source files to build
SOURCES += $(wildcard $(AUBIO_SRC_DIR)/*.c)
SOURCES += $(wildcard $(AUBIO_SRC_DIR)/spectral/*.c)
SOURCES += $(wildcard $(AUBIO_SRC_DIR)/tempo/*.c)
SOURCES += $(wildcard $(AUBIO_SRC_DIR)/onset/*.c)
SOURCES += $(wildcard $(AUBIO_SRC_DIR)/temporal/*.c)
SOURCES += $(wildcard $(AUBIO_SRC_DIR)/utils/*.c)
$(info Building WITH aubio spectral analysis from dep/aubio-src)
else
$(info Building WITHOUT aubio - waveforms will be amplitude-only)
endif

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Tell make where to find source files
VPATH += ../../effects

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add effect C files (will be compiled by build script)
SOURCES += fx_fader.c
SOURCES += fx_crossfader.c

# Add audio analysis and caching C files
SOURCES += src/audio_analysis.c
SOURCES += src/audio_cache.c

# Add dependencies
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
