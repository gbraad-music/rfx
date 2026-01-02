# Regroove Reverb Effect for NTS-3 kaoss pad

PROJECT := regroove_reverb
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_reverb.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
