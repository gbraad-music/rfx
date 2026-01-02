##############################################################################
# Project Configuration
#

PROJECT := rg303
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# Path to synth components - will be /rfx/synth in container, ../../../synth locally
SYNTH_PATH ?= ../../../synth

CSRC += $(SYNTH_PATH)/synth_oscillator.c
CSRC += $(SYNTH_PATH)/synth_filter.c
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

