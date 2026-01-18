##############################################################################
# Project Configuration
#

PROJECT := rgslicer
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# Path to RGSlicer components
SYNTH_PATH ?= /rfx/synth
EFFECTS_PATH ?= /rfx/effects

## C sources
CSRC = header.c
CSRC += $(SYNTH_PATH)/rgslicer.c
CSRC += $(SYNTH_PATH)/rgslicer_detect.c
CSRC += $(SYNTH_PATH)/rgslicer_sfz.c
CSRC += $(SYNTH_PATH)/sample_fx.c
CSRC += $(SYNTH_PATH)/synth_sample_player.c
CSRC += $(SYNTH_PATH)/synth_envelope.c
CSRC += $(SYNTH_PATH)/sfz_parser.c

# C++ sources
CXXSRC = unit.cc

# List ASM source files here
ASMSRC =

ASMXSRC =

##############################################################################
# Include Paths
#

UINCDIR  = $(SYNTH_PATH)
UINCDIR += $(EFFECTS_PATH)

##############################################################################
# Library Paths
#

ULIBDIR =

##############################################################################
# Libraries
#

ULIBS  = -lm
ULIBS += -lc

##############################################################################
# Macros
#

UDEFS =
