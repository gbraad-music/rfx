# Regroove Stereo Widening Effect for logue SDK

PROJECT = regroove_stereo_widen
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = regroove_stereo_widen.cpp ../../effects/fx_stereo_widen.c

# Include effects directory
BUILD_C_FLAGS += -I../../effects

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
