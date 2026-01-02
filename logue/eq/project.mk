# Regroove Effect for logue SDK (NTS-3 kaoss pad)

PROJECT = regroove_$(shell basename $(CURDIR))
PROJECT_TYPE = modfx

# Use container environment if available, fallback to standalone
ifndef PLATFORMDIR
  PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/kaoss-3
endif

# Source files
UCSRC = $(PROJECT).cpp

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
