# Regroove Filter Effect for NTS-3 kaoss pad

PROJECT := regroove_filter
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_filter.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
