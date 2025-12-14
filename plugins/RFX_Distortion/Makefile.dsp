#!/usr/bin/make -f
# DSP-only build (no UI)

include ../../dpf/Makefile.base.mk

NAME = RFX_Distortion
TARGETS = lv2_dsp vst3

FILES_DSP = \
	RFX_DistortionPlugin.cpp \
	../../effects/fx_distortion.c

BUILD_CXX_FLAGS += -I../.. -I../../effects
LINK_FLAGS += -lm

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -I../.. -I../../effects -fno-builtin-pow -fno-builtin-powf
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf
endif

include ../../dpf/Makefile.plugins.mk

lv2_dsp: $(lv2_dsp)
	cp ../../assets/icon-192x192.png ../../bin/$(NAME).lv2/icon.png 2>/dev/null || true
