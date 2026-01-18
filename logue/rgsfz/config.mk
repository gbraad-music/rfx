##############################################################################
# Project Configuration
#

PROJECT := rgsfz
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# Path to SFZ player components
SYNTH_PATH ?= /rfx/synth

## C sources
CSRC = header.c
CSRC += $(SYNTH_PATH)/sfz_player.c
CSRC += $(SYNTH_PATH)/sfz_parser.c
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
