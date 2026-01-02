# Regroove Distortion Effect for NTS-3 kaoss pad

PROJECT := regroove_distortion
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_distortion.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
