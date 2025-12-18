# Regroove Distortion Effect for logue SDK

PROJECT = regroove_distortion
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = regroove_distortion.cpp

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
