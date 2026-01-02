# Regroove Stereo Widen Effect for NTS-3 kaoss pad

PROJECT := regroove_stereo_widen
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_stereo_widen.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
