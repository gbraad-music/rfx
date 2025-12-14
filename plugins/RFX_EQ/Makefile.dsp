#!/usr/bin/make -f
# DSP-only build (no UI)

include ../../dpf/Makefile.base.mk

NAME = RFX_EQ
TARGETS = lv2_dsp vst3

FILES_DSP = \
	RFX_EQPlugin.cpp \
	../../effects/fx_eq.c

BUILD_CXX_FLAGS += -I../.. -I../../effects  
LINK_FLAGS += -lm

ifeq ($(WINDOWS),true)
BUILD_C_FLAGS += -I../.. -I../../effects -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
BUILD_CXX_FLAGS += -fno-builtin-pow -fno-builtin-powf -fno-builtin-exp2f
LINK_FLAGS += -lm
endif

include ../../dpf/Makefile.plugins.mk

lv2_dsp: $(lv2_dsp)
	cp ../../assets/icon-192x192.png ../../bin/$(NAME).lv2/icon.png 2>/dev/null || true
