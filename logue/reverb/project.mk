# Regroove Reverb Effect for logue SDK

PROJECT = regroove_reverb
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = regroove_reverb.cpp ../../effects/fx_reverb.c

# Include effects directory
BUILD_C_FLAGS += -I../../effects

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
