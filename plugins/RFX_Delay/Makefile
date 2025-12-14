#!/usr/bin/make -f

include ../../dpf/Makefile.base.mk

NAME = RFX_Delay

FILES_DSP = \
	RFX_DelayPlugin.cpp \
	../../effects/fx_delay.c

BUILD_CXX_FLAGS += -I../.. -I../../effects
LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk
