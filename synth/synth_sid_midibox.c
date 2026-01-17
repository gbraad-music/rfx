/*
 * MIDIbox SID V2 Compatibility Layer Implementation
 */

#include "synth_sid_midibox.h"
#include "synth_sid_cc.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Factory Presets (128 presets matching MIDIbox SID V2)
// ============================================================================

static const MIDIboxPreset factoryPresets[] = {
    // ===== INIT & BASICS (0-7) =====
    { "Init", 4, 0.5f, 0.0f, 0.5f, 0.7f, 0.3f, 1, 0.5f, 0.0f, 0, 0, 0 },
    { "Saw Lead", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },
    { "Pulse Lead", 4, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },
    { "Triangle Lead", 1, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },
    { "Noise Lead", 8, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },
    { "Square 25%", 4, 0.25f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },
    { "Square 12.5%", 4, 0.12f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },
    { "Square 75%", 4, 0.75f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },

    // ===== BASS SOUNDS (8-23) =====
    { "Pulse Bass", 4, 0.25f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.4f, 0.5f, 1, 0, 0 },
    { "Saw Bass", 2, 0.5f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.35f, 0.6f, 1, 0, 0 },
    { "Triangle Bass", 1, 0.5f, 0.0f, 0.4f, 0.3f, 0.15f, 1, 0.3f, 0.4f, 1, 0, 0 },
    { "Sync Bass", 2, 0.5f, 0.0f, 0.3f, 0.5f, 0.1f, 1, 0.3f, 0.7f, 1, 0, 0 },
    { "Resonant Bass", 4, 0.5f, 0.0f, 0.5f, 0.2f, 0.05f, 1, 0.25f, 0.9f, 1, 0, 0 },
    { "Acid Bass", 2, 0.5f, 0.0f, 0.5f, 0.3f, 0.1f, 1, 0.2f, 0.85f, 1, 0, 0 },
    { "Deep Bass", 1, 0.5f, 0.0f, 0.6f, 0.0f, 0.1f, 1, 0.15f, 0.3f, 1, 0, 0 },
    { "Sync Wobble", 2, 0.5f, 0.0f, 0.4f, 0.4f, 0.2f, 1, 0.3f, 0.8f, 1, 0, 0 },
    { "Seq Bass 1", 4, 0.3f, 0.0f, 0.4f, 0.3f, 0.05f, 1, 0.35f, 0.7f, 1, 0, 0 },
    { "Seq Bass 2", 2, 0.5f, 0.0f, 0.35f, 0.4f, 0.1f, 1, 0.3f, 0.75f, 1, 0, 0 },
    { "Funky Bass", 4, 0.4f, 0.0f, 0.2f, 0.5f, 0.15f, 1, 0.4f, 0.6f, 1, 0, 0 },
    { "Noise Bass", 8, 0.5f, 0.0f, 0.3f, 0.4f, 0.1f, 3, 0.4f, 0.5f, 1, 0, 0 },
    { "Reso Pluck", 4, 0.5f, 0.0f, 0.6f, 0.0f, 0.2f, 1, 0.3f, 0.85f, 1, 0, 0 },
    { "Fat Bass", 4, 0.6f, 0.0f, 0.5f, 0.2f, 0.1f, 1, 0.3f, 0.6f, 1, 0, 0 },
    { "Sub Bass", 1, 0.5f, 0.0f, 0.7f, 0.0f, 0.1f, 1, 0.1f, 0.2f, 1, 0, 0 },
    { "Zap Bass", 2, 0.5f, 0.0f, 0.2f, 0.5f, 0.1f, 1, 0.5f, 0.8f, 1, 0, 0 },

    // ===== LEAD SOUNDS (24-39) =====
    { "Brass Lead", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1, 0, 0 },
    { "Sync Lead", 2, 0.5f, 0.0f, 0.2f, 0.8f, 0.1f, 1, 0.7f, 0.2f, 1, 0, 0 },
    { "Pulse Lead", 4, 0.4f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.65f, 0.4f, 1, 0, 0 },
    { "Fuzzy Lead", 2, 0.5f, 0.0f, 0.1f, 0.8f, 0.1f, 1, 0.8f, 0.3f, 1, 0, 0 },
    { "Soft Lead", 1, 0.5f, 0.0f, 0.4f, 0.6f, 0.3f, 1, 0.5f, 0.2f, 1, 0, 0 },
    { "Ring Lead", 1, 0.5f, 0.0f, 0.3f, 0.7f, 0.2f, 0, 0.6f, 0.0f, 0, 0, 0 },
    { "Hard Lead", 4, 0.3f, 0.0f, 0.1f, 0.8f, 0.1f, 1, 0.75f, 0.5f, 1, 0, 0 },
    { "Screamer", 2, 0.5f, 0.0f, 0.0f, 0.9f, 0.05f, 1, 0.9f, 0.1f, 1, 0, 0 },
    { "Thin Lead", 4, 0.15f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.7f, 0.4f, 1, 0, 0 },
    { "Wide Lead", 4, 0.7f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.6f, 0.4f, 1, 0, 0 },
    { "Stabby Lead", 2, 0.5f, 0.0f, 0.1f, 0.7f, 0.05f, 1, 0.65f, 0.5f, 1, 0, 0 },
    { "Mono Lead", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.55f, 0.4f, 1, 0, 0 },
    { "Reso Lead", 4, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.4f, 0.85f, 1, 0, 0 },
    { "Pure Lead", 1, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Dirty Lead", 8, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 3, 0.6f, 0.4f, 1, 0, 0 },
    { "Epic Lead", 2, 0.5f, 0.0f, 0.4f, 0.7f, 0.3f, 1, 0.6f, 0.3f, 1, 0, 0 },

    // ===== CLASSIC C64 SOUNDS (40-55) =====
    { "SEQ Vintage C", 2, 0.5f, 0.0f, 0.4f, 0.5f, 0.1f, 1, 0.35f, 0.7f, 1, 0, 0 },
    { "Last Ninja", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.45f, 0.6f, 1, 0, 0 },
    { "Commando", 4, 0.4f, 0.0f, 0.2f, 0.7f, 0.1f, 1, 0.5f, 0.7f, 1, 0, 0 },
    { "Monty Run", 2, 0.5f, 0.0f, 0.35f, 0.5f, 0.15f, 1, 0.4f, 0.65f, 1, 0, 0 },
    { "Driller", 4, 0.3f, 0.0f, 0.3f, 0.6f, 0.15f, 1, 0.45f, 0.7f, 1, 0, 0 },
    { "Delta", 2, 0.5f, 0.0f, 0.4f, 0.5f, 0.2f, 1, 0.5f, 0.5f, 1, 0, 0 },
    { "Galway Lead", 2, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.6f, 0.4f, 1, 0, 0 },
    { "Hubbard Bass", 4, 0.3f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.35f, 0.75f, 1, 0, 0 },
    { "Tel Bass", 4, 0.25f, 0.0f, 0.5f, 0.2f, 0.05f, 1, 0.3f, 0.8f, 1, 0, 0 },
    { "Game Over", 8, 0.5f, 0.0f, 0.2f, 0.6f, 0.1f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Arkanoid", 1, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Turrican", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.55f, 0.5f, 1, 0, 0 },
    { "International", 4, 0.4f, 0.0f, 0.25f, 0.6f, 0.15f, 1, 0.5f, 0.6f, 1, 1, 0 },
    { "Ocean Loader", 2, 0.5f, 0.0f, 0.4f, 0.5f, 0.2f, 1, 0.45f, 0.55f, 1, 0, 0 },
    { "Thrust", 4, 0.35f, 0.0f, 0.3f, 0.5f, 0.15f, 1, 0.5f, 0.65f, 1, 0, 0 },
    { "Wizball", 1, 0.5f, 0.0f, 0.4f, 0.5f, 0.25f, 1, 0.4f, 0.4f, 1, 0, 0 },

    // ===== PADS & STRINGS (56-63) =====
    { "Soft Pad", 1, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 1, 0.5f, 0.2f, 1, 0, 0 },
    { "Saw Pad", 2, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Pulse Pad", 4, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Sync Pad", 2, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Strings", 2, 0.5f, 0.6f, 0.9f, 0.9f, 0.6f, 1, 0.6f, 0.2f, 1, 0, 0 },
    { "Brass Sect", 2, 0.5f, 0.3f, 0.6f, 0.7f, 0.4f, 1, 0.55f, 0.3f, 1, 0, 0 },
    { "Slow Pad", 1, 0.5f, 0.8f, 0.9f, 0.9f, 0.7f, 1, 0.5f, 0.2f, 1, 0, 0 },
    { "Atmosphere", 8, 0.5f, 0.6f, 0.9f, 0.9f, 0.6f, 2, 0.5f, 0.3f, 1, 0, 0 },

    // ===== PLUCKS & BELLS (64-71) =====
    { "Pluck", 1, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 1, 0.5f, 0.2f, 1, 0, 0 },
    { "Harp", 1, 0.5f, 0.0f, 0.6f, 0.0f, 0.4f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Marimba", 1, 0.5f, 0.0f, 0.4f, 0.0f, 0.2f, 1, 0.4f, 0.3f, 1, 0, 0 },
    { "Ring Bell", 1, 0.5f, 0.0f, 0.6f, 0.0f, 0.5f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Sync Bell", 2, 0.5f, 0.0f, 0.6f, 0.0f, 0.5f, 0, 0.6f, 0.0f, 0, 0, 0 },
    { "Clav", 4, 0.3f, 0.0f, 0.3f, 0.0f, 0.15f, 1, 0.6f, 0.4f, 1, 0, 0 },
    { "Koto", 1, 0.5f, 0.0f, 0.5f, 0.0f, 0.35f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Kalimba", 1, 0.5f, 0.0f, 0.4f, 0.0f, 0.25f, 0, 0.5f, 0.0f, 0, 0, 0 },

    // ===== FX & PERCUSSION (72-79) =====
    { "Laser", 2, 0.5f, 0.0f, 0.3f, 0.0f, 0.1f, 1, 0.8f, 0.5f, 1, 0, 0 },
    { "Zap", 8, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 3, 0.7f, 0.4f, 1, 0, 0 },
    { "Sweep Up", 2, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 1, 0.3f, 0.7f, 1, 0, 0 },
    { "Sweep Down", 2, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 1, 0.7f, 0.7f, 1, 0, 0 },
    { "Noise Hit", 8, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 3, 0.5f, 0.3f, 1, 0, 0 },
    { "Noise Snare", 8, 0.5f, 0.0f, 0.15f, 0.0f, 0.1f, 3, 0.6f, 0.3f, 1, 0, 0 },
    { "Tom", 1, 0.5f, 0.0f, 0.3f, 0.0f, 0.15f, 1, 0.3f, 0.4f, 1, 0, 0 },
    { "Kick", 1, 0.5f, 0.0f, 0.2f, 0.0f, 0.05f, 1, 0.2f, 0.3f, 1, 0, 0 },

    // ===== SPECIAL (80-87) =====
    { "Digi Bass", 8, 0.5f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.3f, 0.5f, 1, 0, 0 },
    { "Voice", 8, 0.5f, 0.3f, 0.6f, 0.7f, 0.4f, 2, 0.5f, 0.3f, 1, 0, 0 },
    { "Choir", 1, 0.5f, 0.5f, 0.8f, 0.8f, 0.6f, 2, 0.6f, 0.2f, 1, 0, 0 },
    { "Organ", 4, 0.5f, 0.1f, 0.5f, 0.7f, 0.3f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Accordion", 4, 0.6f, 0.2f, 0.6f, 0.7f, 0.4f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Harmonica", 2, 0.5f, 0.1f, 0.5f, 0.7f, 0.3f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Flute", 1, 0.5f, 0.3f, 0.6f, 0.7f, 0.4f, 1, 0.6f, 0.2f, 1, 0, 0 },
    { "Sitar", 1, 0.5f, 0.0f, 0.5f, 0.0f, 0.4f, 0, 0.5f, 0.0f, 0, 0, 0 },

    // ===== MORE BASSES (88-95) =====
    { "Tech Bass", 4, 0.35f, 0.0f, 0.3f, 0.4f, 0.1f, 1, 0.3f, 0.75f, 1, 0, 0 },
    { "Wobble Bass", 2, 0.5f, 0.0f, 0.5f, 0.3f, 0.2f, 1, 0.25f, 0.85f, 1, 0, 0 },
    { "Trance Bass", 2, 0.5f, 0.0f, 0.4f, 0.4f, 0.15f, 1, 0.3f, 0.8f, 1, 0, 0 },
    { "Electro Bass", 4, 0.3f, 0.0f, 0.3f, 0.5f, 0.1f, 1, 0.35f, 0.7f, 1, 0, 0 },
    { "Minimal Bass", 1, 0.5f, 0.0f, 0.5f, 0.2f, 0.1f, 1, 0.25f, 0.5f, 1, 0, 0 },
    { "Hard Bass", 2, 0.5f, 0.0f, 0.2f, 0.6f, 0.05f, 1, 0.4f, 0.8f, 1, 0, 0 },
    { "Soft Bass", 1, 0.5f, 0.0f, 0.5f, 0.4f, 0.2f, 1, 0.3f, 0.3f, 1, 0, 0 },
    { "Vintage Bass", 4, 0.4f, 0.0f, 0.4f, 0.4f, 0.15f, 1, 0.35f, 0.6f, 1, 0, 0 },

    // ===== MORE LEADS (96-103) =====
    { "Space Lead", 2, 0.5f, 0.0f, 0.3f, 0.7f, 0.2f, 0, 0.6f, 0.0f, 0, 0, 0 },
    { "Retro Lead", 4, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.55f, 0.5f, 1, 0, 0 },
    { "Chip Lead", 4, 0.25f, 0.0f, 0.1f, 0.8f, 0.05f, 1, 0.6f, 0.4f, 1, 0, 0 },
    { "8-bit Lead", 4, 0.5f, 0.0f, 0.1f, 0.8f, 0.05f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Arpeggio", 4, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 1, 0.5f, 0.3f, 1, 0, 0 },
    { "Stab", 2, 0.5f, 0.0f, 0.1f, 0.7f, 0.05f, 1, 0.6f, 0.5f, 1, 0, 0 },
    { "PWM Lead", 4, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.5f, 0.4f, 1, 0, 0 },
    { "Dirty Sync", 2, 0.5f, 0.0f, 0.1f, 0.8f, 0.05f, 0, 0.7f, 0.0f, 0, 0, 0 },

    // ===== EXPERIMENTAL (104-111) =====
    { "Random 1", 6, 0.5f, 0.2f, 0.5f, 0.5f, 0.3f, 2, 0.5f, 0.4f, 1, 0, 0 },
    { "Random 2", 7, 0.6f, 0.3f, 0.6f, 0.4f, 0.2f, 1, 0.6f, 0.5f, 1, 0, 0 },
    { "Random 3", 5, 0.4f, 0.1f, 0.4f, 0.6f, 0.25f, 3, 0.5f, 0.3f, 1, 0, 0 },
    { "Glitch 1", 8, 0.5f, 0.0f, 0.1f, 0.0f, 0.05f, 3, 0.7f, 0.5f, 1, 0, 0 },
    { "Glitch 2", 8, 0.5f, 0.0f, 0.15f, 0.0f, 0.1f, 2, 0.6f, 0.6f, 1, 0, 0 },
    { "Lo-Fi", 8, 0.5f, 0.2f, 0.5f, 0.5f, 0.3f, 1, 0.5f, 0.4f, 1, 0, 0 },
    { "Crushed", 8, 0.5f, 0.0f, 0.2f, 0.5f, 0.1f, 3, 0.6f, 0.5f, 1, 0, 0 },
    { "Broken", 6, 0.3f, 0.0f, 0.3f, 0.3f, 0.15f, 2, 0.5f, 0.6f, 1, 0, 0 },

    // ===== DRONE & AMBIENT (112-119) =====
    { "Drone 1", 2, 0.5f, 0.8f, 0.9f, 0.9f, 0.8f, 1, 0.4f, 0.2f, 1, 0, 0 },
    { "Drone 2", 1, 0.5f, 0.8f, 0.9f, 0.9f, 0.8f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Dark Pad", 2, 0.5f, 0.7f, 0.9f, 0.9f, 0.7f, 1, 0.3f, 0.3f, 1, 0, 0 },
    { "Space Pad", 8, 0.5f, 0.6f, 0.9f, 0.9f, 0.6f, 2, 0.5f, 0.2f, 1, 0, 0 },
    { "Wind", 8, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 2, 0.6f, 0.3f, 1, 0, 0 },
    { "Ocean", 8, 0.5f, 0.6f, 0.9f, 0.9f, 0.7f, 1, 0.4f, 0.4f, 1, 0, 0 },
    { "Rain", 8, 0.5f, 0.3f, 0.7f, 0.7f, 0.4f, 3, 0.5f, 0.3f, 1, 0, 0 },
    { "Thunder", 8, 0.5f, 0.0f, 0.3f, 0.0f, 0.2f, 1, 0.3f, 0.5f, 1, 0, 0 },

    // ===== UTILITY & SPECIAL (120-127) =====
    { "Test Tone", 1, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Click", 1, 0.5f, 0.0f, 0.0f, 0.0f, 0.01f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Pop", 4, 0.5f, 0.0f, 0.0f, 0.0f, 0.02f, 1, 0.5f, 0.0f, 1, 0, 0 },
    { "Beep", 4, 0.5f, 0.0f, 0.1f, 0.0f, 0.05f, 0, 0.5f, 0.0f, 0, 0, 0 },
    { "Chirp", 1, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 1, 0.7f, 0.3f, 1, 0, 0 },
    { "Blip", 4, 0.25f, 0.0f, 0.1f, 0.0f, 0.05f, 1, 0.6f, 0.2f, 1, 0, 0 },
    { "Silence", 0, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0, 0, 0 },
    { "Full Volume", 2, 0.5f, 0.0f, 0.5f, 0.7f, 0.3f, 0, 1.0f, 0.0f, 0, 0, 0 }
};

#define NUM_FACTORY_PRESETS 128

// ============================================================================
// Core Functions
// ============================================================================

MIDIboxSIDInstance* midibox_sid_create(float sample_rate) {
    MIDIboxSIDInstance* mb = (MIDIboxSIDInstance*)malloc(sizeof(MIDIboxSIDInstance));
    if (!mb) return NULL;

    mb->sample_rate = sample_rate;
    mb->sid = synth_sid_create((int)sample_rate);

    if (!mb->sid) {
        free(mb);
        return NULL;
    }

    memset(mb->parameters, 0, sizeof(mb->parameters));
    mb->engine_mode = MIDIBOX_ENGINE_LEAD;

    return mb;
}

void midibox_sid_destroy(MIDIboxSIDInstance* mb) {
    if (mb) {
        if (mb->sid) {
            synth_sid_destroy(mb->sid);
        }
        free(mb);
    }
}

void midibox_sid_reset(MIDIboxSIDInstance* mb) {
    if (!mb || !mb->sid) return;
    synth_sid_reset(mb->sid);
}

// ============================================================================
// Engine Mode
// ============================================================================

void midibox_sid_set_engine_mode(MIDIboxSIDInstance* mb, MIDIboxEngineMode mode) {
    if (mb) {
        mb->engine_mode = mode;
    }
}

MIDIboxEngineMode midibox_sid_get_engine_mode(MIDIboxSIDInstance* mb) {
    return mb ? mb->engine_mode : MIDIBOX_ENGINE_LEAD;
}

// ============================================================================
// MIDI Message Handling
// ============================================================================

void midibox_sid_handle_midi(MIDIboxSIDInstance* mb, uint8_t status, uint8_t data1, uint8_t data2) {
    if (!mb || !mb->sid) return;

    const uint8_t message = status & 0xF0;
    const uint8_t channel = status & 0x0F;

    // Lead Engine: Only respond to MIDI channel 1 (channel index 0)
    // Multi Engine: Respond to channels 1, 2, 3 (indices 0, 1, 2)
    if (mb->engine_mode == MIDIBOX_ENGINE_LEAD && channel != 0) {
        return;
    }

    switch (message) {
        case 0x90: // Note On
            if (data2 > 0) {
                midibox_sid_note_on(mb, channel, data1, data2);
            } else {
                midibox_sid_note_off(mb, channel, data1);
            }
            break;

        case 0x80: // Note Off
            midibox_sid_note_off(mb, channel, data1);
            break;

        case 0xB0: // Control Change
            synth_sid_handle_cc(mb->sid, data1, data2);
            break;

        case 0xE0: // Pitch Bend
            {
                uint16_t bend = (data2 << 7) | data1;
                midibox_sid_pitch_bend(mb, channel, bend);
            }
            break;
    }
}

void midibox_sid_note_on(MIDIboxSIDInstance* mb, uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!mb || !mb->sid) return;

    if (mb->engine_mode == MIDIBOX_ENGINE_LEAD) {
        // Lead Engine: trigger ALL 3 voices (unison)
        for (int v = 0; v < 3; v++) {
            synth_sid_note_on(mb->sid, v, note, velocity);
        }
    } else {
        // Multi Engine: route MIDI channels to voices
        if (channel < 3) {
            synth_sid_note_on(mb->sid, channel, note, velocity);
        }
    }
}

void midibox_sid_note_off(MIDIboxSIDInstance* mb, uint8_t channel, uint8_t note) {
    if (!mb || !mb->sid) return;

    if (mb->engine_mode == MIDIBOX_ENGINE_LEAD) {
        // Lead Engine: release ALL 3 voices
        for (int v = 0; v < 3; v++) {
            synth_sid_note_off(mb->sid, v);
        }
    } else {
        // Multi Engine: route to specific voice
        if (channel < 3) {
            synth_sid_note_off(mb->sid, channel);
        }
    }
}

void midibox_sid_control_change(MIDIboxSIDInstance* mb, uint8_t channel, uint8_t controller, uint8_t value) {
    if (!mb || !mb->sid) return;
    synth_sid_handle_cc(mb->sid, controller, value);
}

void midibox_sid_pitch_bend(MIDIboxSIDInstance* mb, uint8_t channel, uint16_t value) {
    if (!mb || !mb->sid) return;

    if (mb->engine_mode == MIDIBOX_ENGINE_LEAD) {
        // Lead Engine: apply to all voices
        for (int v = 0; v < 3; v++) {
            synth_sid_handle_pitch_bend_midi(mb->sid, v, value);
        }
    } else {
        // Multi Engine: apply to specific voice
        if (channel < 3) {
            synth_sid_handle_pitch_bend_midi(mb->sid, channel, value);
        }
    }
}

void midibox_sid_all_notes_off(MIDIboxSIDInstance* mb) {
    if (!mb || !mb->sid) return;
    synth_sid_all_notes_off(mb->sid);
}

// ============================================================================
// Audio Processing
// ============================================================================

void midibox_sid_process_f32(MIDIboxSIDInstance* mb, float* buffer, int frames, float sample_rate) {
    if (!mb || !mb->sid || !buffer) return;
    synth_sid_process_f32(mb->sid, buffer, frames, (int)sample_rate);
}

// ============================================================================
// Parameter Management
// ============================================================================

int midibox_sid_get_parameter_count(void) {
    return 40;
}

float midibox_sid_get_parameter(MIDIboxSIDInstance* mb, int index) {
    if (!mb || index < 0 || index >= 40) return 0.0f;
    return mb->parameters[index];
}

void midibox_sid_set_parameter(MIDIboxSIDInstance* mb, int index, float value) {
    if (!mb || !mb->sid) return;

    if (index >= 0 && index < 40) {
        mb->parameters[index] = value;
    }

    int voice = index / 8;
    int param = index % 8;

    if (voice < 3) {
        // Voice parameters
        switch (param) {
            case 0: synth_sid_set_waveform(mb->sid, voice, (uint8_t)value); break;
            case 1: synth_sid_set_pulse_width(mb->sid, voice, value); break;
            case 2: synth_sid_set_attack(mb->sid, voice, value); break;
            case 3: synth_sid_set_decay(mb->sid, voice, value); break;
            case 4: synth_sid_set_sustain(mb->sid, voice, value); break;
            case 5: synth_sid_set_release(mb->sid, voice, value); break;
            case 6: synth_sid_set_ring_mod(mb->sid, voice, value > 0.5f); break;
            case 7: synth_sid_set_sync(mb->sid, voice, value > 0.5f); break;
        }
    } else {
        // Filter and global parameters
        switch (index) {
            case 24: synth_sid_set_filter_mode(mb->sid, (SIDFilterMode)(int)value); break;
            case 25: synth_sid_set_filter_cutoff(mb->sid, value); break;
            case 26: synth_sid_set_filter_resonance(mb->sid, value); break;
            case 27: synth_sid_set_filter_voice(mb->sid, 0, value > 0.5f); break;
            case 28: synth_sid_set_filter_voice(mb->sid, 1, value > 0.5f); break;
            case 29: synth_sid_set_filter_voice(mb->sid, 2, value > 0.5f); break;
            case 30: synth_sid_set_volume(mb->sid, value); break;
            case 31: synth_sid_set_lfo_frequency(mb->sid, 0, 0.1f * powf(100.0f, value)); break;
            case 32: synth_sid_set_lfo_waveform(mb->sid, 0, (int)value); break;
            case 33: synth_sid_set_lfo1_to_pitch(mb->sid, value); break;
            case 34: synth_sid_set_lfo_frequency(mb->sid, 1, 0.05f * powf(100.0f, value)); break;
            case 35: synth_sid_set_lfo_waveform(mb->sid, 1, (int)value); break;
            case 36: synth_sid_set_lfo2_to_filter(mb->sid, value); break;
            case 37: synth_sid_set_lfo2_to_pw(mb->sid, value); break;
            case 38: synth_sid_set_mod_wheel(mb->sid, value); break;
            case 39: midibox_sid_set_engine_mode(mb, (MIDIboxEngineMode)((int)(value > 0.5f))); break;
        }
    }
}

const char* midibox_sid_get_parameter_name(int index) {
    static const char* names[] = {
        "V1 Waveform", "V1 Pulse Width", "V1 Attack", "V1 Decay",
        "V1 Sustain", "V1 Release", "V1 Ring Mod", "V1 Sync",
        "V2 Waveform", "V2 Pulse Width", "V2 Attack", "V2 Decay",
        "V2 Sustain", "V2 Release", "V2 Ring Mod", "V2 Sync",
        "V3 Waveform", "V3 Pulse Width", "V3 Attack", "V3 Decay",
        "V3 Sustain", "V3 Release", "V3 Ring Mod", "V3 Sync",
        "Filter Mode", "Filter Cutoff", "Filter Resonance",
        "Filter V1", "Filter V2", "Filter V3", "Volume",
        "LFO1 Rate", "LFO1 Waveform", "LFO1 → Pitch",
        "LFO2 Rate", "LFO2 Waveform", "LFO2 → Filter", "LFO2 → PW",
        "Mod Wheel",
        "Engine Mode"
    };

    if (index < 0 || index >= 40) return "";
    return names[index];
}

const char* midibox_sid_get_parameter_label(int index) {
    return "";
}

float midibox_sid_get_parameter_default(int index) {
    if (index % 8 == 0) return 4.0f;  // Waveform: Pulse
    if (index % 8 == 1) return 0.5f;  // Pulse Width: 50%
    if (index % 8 == 4) return 0.7f;  // Sustain: 70%
    if (index == 30) return 0.7f;     // Volume: 70%
    if (index == 39) return 0.0f;     // Engine Mode: Lead
    return 0.0f;
}

float midibox_sid_get_parameter_min(int index) {
    return 0.0f;
}

float midibox_sid_get_parameter_max(int index) {
    if (index % 8 == 0) return 15.0f;  // Waveform
    if (index == 24) return 3.0f;      // Filter mode
    if (index == 39) return 1.0f;      // Engine Mode
    return 1.0f;
}

int midibox_sid_get_parameter_group(int index) {
    if (index < 8) return 0;
    if (index < 16) return 1;
    if (index < 24) return 2;
    return 3;
}

const char* midibox_sid_get_group_name(int group) {
    static const char* groups[] = { "Voice 1", "Voice 2", "Voice 3", "Filter/Global" };
    if (group < 0 || group >= 4) return "";
    return groups[group];
}

int midibox_sid_parameter_is_integer(int index) {
    if (index % 8 == 0 || index == 24) return 1;
    if (index % 8 == 6 || index % 8 == 7) return 1;
    return 0;
}

// ============================================================================
// Preset System
// ============================================================================

int midibox_sid_get_preset_count(void) {
    return NUM_FACTORY_PRESETS;
}

const char* midibox_sid_get_preset_name(int index) {
    if (index < 0 || index >= NUM_FACTORY_PRESETS) return "";
    return factoryPresets[index].name;
}

void midibox_sid_load_preset(MIDIboxSIDInstance* mb, int index, int voice) {
    if (!mb || !mb->sid) return;
    if (index < 0 || index >= NUM_FACTORY_PRESETS) return;
    if (voice < 0 || voice > 2) voice = 0;

    const MIDIboxPreset* preset = &factoryPresets[index];

    // Lead Engine Mode: When loading to Voice 1, apply to ALL 3 voices (unison)
    if (voice == 0) {
        for (int v = 0; v < 3; v++) {
            int base = v * 8;

            midibox_sid_set_parameter(mb, base + 0, (float)preset->waveform);
            midibox_sid_set_parameter(mb, base + 1, preset->pulseWidth);
            midibox_sid_set_parameter(mb, base + 2, preset->attack);
            midibox_sid_set_parameter(mb, base + 3, preset->decay);
            midibox_sid_set_parameter(mb, base + 4, preset->sustain);
            midibox_sid_set_parameter(mb, base + 5, preset->release);
            midibox_sid_set_parameter(mb, base + 6, 0.0f);
            midibox_sid_set_parameter(mb, base + 7, 0.0f);

            // Special presets with sync/ring
            if (index == 11 || index == 24 || index == 25) {
                midibox_sid_set_parameter(mb, base + 7, 1.0f);
            }
            if (index == 29 || index == 68) {
                midibox_sid_set_parameter(mb, base + 6, 1.0f);
            }
        }
    } else {
        int base = voice * 8;

        midibox_sid_set_parameter(mb, base + 0, (float)preset->waveform);
        midibox_sid_set_parameter(mb, base + 1, preset->pulseWidth);
        midibox_sid_set_parameter(mb, base + 2, preset->attack);
        midibox_sid_set_parameter(mb, base + 3, preset->decay);
        midibox_sid_set_parameter(mb, base + 4, preset->sustain);
        midibox_sid_set_parameter(mb, base + 5, preset->release);
        midibox_sid_set_parameter(mb, base + 6, 0.0f);
        midibox_sid_set_parameter(mb, base + 7, 0.0f);

        if (index == 11 || index == 24 || index == 25) {
            midibox_sid_set_parameter(mb, base + 7, 1.0f);
        }
        if (index == 29 || index == 68) {
            midibox_sid_set_parameter(mb, base + 6, 1.0f);
        }
    }

    // Filter (global)
    if (voice == 0) {
        midibox_sid_set_parameter(mb, 24, (float)preset->filterMode);
        midibox_sid_set_parameter(mb, 25, preset->filterCutoff);
        midibox_sid_set_parameter(mb, 26, preset->filterResonance);

        uint8_t filter_enabled = preset->filterVoice1 || preset->filterVoice2 || preset->filterVoice3;
        midibox_sid_set_parameter(mb, 27, filter_enabled ? 1.0f : 0.0f);
        midibox_sid_set_parameter(mb, 28, filter_enabled ? 1.0f : 0.0f);
        midibox_sid_set_parameter(mb, 29, filter_enabled ? 1.0f : 0.0f);
    } else {
        midibox_sid_set_parameter(mb, 27 + voice,
            voice == 0 ? preset->filterVoice1 :
            voice == 1 ? preset->filterVoice2 : preset->filterVoice3);
    }

    // Volume
    if (voice == 0) {
        midibox_sid_set_parameter(mb, 30, 0.7f);
    }
}
