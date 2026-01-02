# Regroove Compressor Effect for NTS-3 kaoss pad

PROJECT := regroove_compressor
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_compressor.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
