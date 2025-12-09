# MODEL 1 Contour HPF Effect for logue SDK

PROJECT = model1_hpf
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = model1_hpf.cpp ../../effects/fx_model1_hpf.c

# Include effects directory
BUILD_C_FLAGS += -I../../effects

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
