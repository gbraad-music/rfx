##############################################################################
# Project Configuration
#

PROJECT := rgslicer_mk2
PROJECT_TYPE := osc

##############################################################################
# Sources
#

# Path to RGSlicer engine
SYNTH_PATH ?= /rfx/synth

# C sources
CSRC = header.c

CSRC += $(SYNTH_PATH)/rgslicer.c
CSRC += $(SYNTH_PATH)/synth_sample_player.c
CSRC += $(SYNTH_PATH)/synth_envelope.c

# C++ sources
CXXSRC = unit.cc

# List ASM source files here
ASMSRC =

ASMXSRC =

##############################################################################
# Include Paths
#

UINCDIR  = $(SYNTH_PATH)

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
