# If RACK_DIR is not defined when calling the Makefile, default to standard location
RACK_DIR ?= $(HOME)/Applications/VCVRack/Rack-SDK

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I../../effects
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Tell make where to find source files
VPATH += ../../effects

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add fx_crossfader.c (will be compiled by build script)
SOURCES += fx_crossfader.c

# Add dependencies
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
