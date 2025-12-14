#!/usr/bin/make -f

include ../../dpf/Makefile.base.mk

NAME = RFX_Compressor

FILES_DSP = \
	RFX_CompressorPlugin.cpp \
	../../effects/fx_compressor.c

BUILD_CXX_FLAGS += -I../.. -I../../effects
LINK_FLAGS += -lm

include ../../dpf/Makefile.plugins.mk
