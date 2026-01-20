# Regroove Amiga Filter Effect for NTS-3 kaoss pad

PROJECT := regroove_amigafilter
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_amiga_filter.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
