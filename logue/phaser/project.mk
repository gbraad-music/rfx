# Regroove Phaser Effect for logue SDK

PROJECT = regroove_phaser
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = regroove_phaser.cpp ../../effects/fx_phaser.c

# Include effects directory
BUILD_C_FLAGS += -I../../effects

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
