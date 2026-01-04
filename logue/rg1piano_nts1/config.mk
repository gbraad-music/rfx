##############################################################################
# Project Configuration
#

PROJECT := rg1piano
PROJECT_TYPE := osc

##############################################################################
# Sources
#

# C sources
UCSRC = header.c
UCSRC += sample_data.c

# Path to synth components
UCSRC += /rfx/synth/synth_sample_player.c

# C++ sources
UCXXSRC = unit.cc

# List ASM source files here
ASMSRC =

ASMXSRC =

##############################################################################
# Include Paths
#

UINCDIR  = /rfx/synth

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
