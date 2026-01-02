##############################################################################
# Configuration for Makefile
#

PROJECT := distortion
PROJECT_TYPE := modfx

##############################################################################
# Sources
#

# C sources
UCSRC = header.c
UCSRC += /rfx/effects/fx_distortion.c

# C++ sources
UCXXSRC = unit.cc

# List ASM source files here
UASMSRC =

UASMXSRC =

##############################################################################
# Include Paths
#

UINCDIR = /rfx/effects

##############################################################################
# Library Paths
#

ULIBDIR =

##############################################################################
# Libraries
#

ULIBS  = -lm

##############################################################################
# Macros
#

UDEFS =
