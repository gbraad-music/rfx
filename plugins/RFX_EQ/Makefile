#!/usr/bin/make -f

include ../../dpf/Makefile.base.mk

NAME = RFX_EQ

FILES_DSP = \
	RFX_EQPlugin.cpp \
	../../effects/fx_eq.c

BUILD_CXX_FLAGS += -I../.. -I../../effects
LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk
