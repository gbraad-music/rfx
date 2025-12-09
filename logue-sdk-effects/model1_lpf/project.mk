# MODEL 1 Contour LPF Effect for logue SDK

PROJECT = model1_lpf
PROJECT_TYPE = modfx

# Include logue SDK
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital

# Source files
UCSRC = model1_lpf.cpp ../../effects/fx_model1_lpf.c

# Include effects directory
BUILD_C_FLAGS += -I../../effects

# Include the logue build system
include $(PLATFORMDIR)/modfx.mk
