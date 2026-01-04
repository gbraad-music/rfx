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

# Path to shared sample data
UCSRC += /rfx/data/rg1piano/sample_data.c

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
UINCDIR += /rfx/data/rg1piano

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
