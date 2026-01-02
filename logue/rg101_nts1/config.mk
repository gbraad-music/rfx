##############################################################################
# Project Configuration
#

PROJECT := rg101
PROJECT_TYPE := osc

##############################################################################
# Sources
#

# C sources
UCSRC = header.c

# Path to synth components
UCSRC += /rfx/synth/synth_oscillator.c
UCSRC += /rfx/synth/synth_filter_ladder.c
UCSRC += /rfx/synth/synth_envelope.c
UCSRC += /rfx/synth/synth_lfo.c
UCSRC += /rfx/synth/synth_noise.c

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
