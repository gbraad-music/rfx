# Regroove RG909 Kaoss Effect for NTS-3

PROJECT := rg909_kaoss
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c rg909_drum.c
UCSRC += /rfx/synth/rg909_bd.c
UCSRC += /rfx/synth/synth_resonator.c
UCSRC += /rfx/synth/synth_noise.c
UCSRC += /rfx/synth/synth_filter.c

UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/synth
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS = -DEMBEDDED_STATIC
