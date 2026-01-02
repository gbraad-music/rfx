# Regroove RM1 - Model1 Channel Strip for NTS-3 kaoss pad

PROJECT := regroove_rm1
PROJECT_TYPE := genericfx

# Sources
UCSRC = header.c /rfx/effects/fx_model1_trim.c /rfx/effects/fx_model1_hpf.c /rfx/effects/fx_model1_lpf.c /rfx/effects/fx_model1_sculpt.c
UCXXSRC = unit.cc

# Libraries
UINCDIR = /rfx/effects
ULIBDIR =
ULIBS = -lm

# Definitions
UDEFS =
