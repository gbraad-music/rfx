# Regroove Filter Effect for logue SDK

PROJECT = regroove_filter
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = regroove_filter.cpp

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
