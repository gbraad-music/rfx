##############################################################################
# Project Configuration
#

PROJECT := rg909_drum
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# Path to synth components - will be /rfx/synth in container, ../../../synth locally
SYNTH_PATH ?= ../../../synth

CSRC += $(SYNTH_PATH)/rg909_drum_synth.c
CSRC += $(SYNTH_PATH)/synth_resonator.c
CSRC += $(SYNTH_PATH)/synth_noise.c
CSRC += $(SYNTH_PATH)/synth_filter.c
CSRC += $(SYNTH_PATH)/synth_envelope.c
CSRC += $(SYNTH_PATH)/synth_voice_manager.c

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
