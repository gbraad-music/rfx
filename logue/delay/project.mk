# Regroove Delay Effect for logue SDK

PROJECT = regroove_delay
PROJECT_TYPE = delfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = regroove_delay.cpp

# Include the logue build system
include $(PLATFORMDIR)/delfx.mk
