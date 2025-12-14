#!/usr/bin/make -f

include ../../dpf/Makefile.base.mk

NAME = RFX_Distortion

FILES_DSP = \
	RFX_DistortionPlugin.cpp \
	../../effects/fx_distortion.c

BUILD_CXX_FLAGS += -I../.. -I../../effects
LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk
