# Regroove EQ Effect for NTS-3 kaoss pad

PROJECT := regroove_eq
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_eq.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
