# MODEL 1 Sculpt Effect for logue SDK

PROJECT = model1_sculpt
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = model1_sculpt.cpp ../../effects/fx_model1_sculpt.c

# Include effects directory
BUILD_C_FLAGS += -I../../effects

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
