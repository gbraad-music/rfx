# Regroove Model1 Trim Effect for logue SDK

PROJECT = regroove_model1_trim
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = regroove_model1_trim.cpp ../../effects/fx_model1_trim.c

# Include effects directory
BUILD_C_FLAGS += -I../../effects

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
