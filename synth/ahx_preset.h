/*
 * AHX Preset System
 *
 * Save/load AHX instrument presets
 * Import instruments from .ahx files
 */

#ifndef AHX_PRESET_H
#define AHX_PRESET_H

#include "ahx_instrument.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Preset metadata
typedef struct {
    char name[64];                  // Preset name
    char author[64];                // Author name
    char description[256];          // Description
    AhxInstrumentParams params;     // Instrument parameters
} AhxPreset;

/**
 * Save preset to file
 * @param preset Preset to save
 * @param filepath Path to save to (.ahxp extension recommended)
 * @return true on success
 */
bool ahx_preset_save(const AhxPreset* preset, const char* filepath);

/**
 * Load preset from file
 * @param preset Output preset
 * @param filepath Path to load from
 * @return true on success
 */
bool ahx_preset_load(AhxPreset* preset, const char* filepath);

/**
 * Import instrument from AHX file
 * @param preset Output preset
 * @param ahx_filepath Path to .ahx file
 * @param instrument_index Instrument index to extract (0-63)
 * @return true on success
 */
bool ahx_preset_import_from_ahx(AhxPreset* preset, const char* ahx_filepath, uint8_t instrument_index);

/**
 * Get number of instruments in AHX file
 * @param ahx_filepath Path to .ahx file
 * @return Number of instruments, or 0 on error
 */
uint8_t ahx_preset_get_ahx_instrument_count(const char* ahx_filepath);

/**
 * Get instrument name from AHX file
 * @param ahx_filepath Path to .ahx file
 * @param instrument_index Instrument index (0-63)
 * @param name_out Output buffer (min 64 bytes)
 * @return true on success
 */
bool ahx_preset_get_ahx_instrument_name(const char* ahx_filepath, uint8_t instrument_index, char* name_out);

/**
 * Create default preset
 */
AhxPreset ahx_preset_create_default(void);

/**
 * Get built-in preset by index
 * @param index Preset index (0-5)
 * @return Preset, or default if index invalid
 */
AhxPreset ahx_preset_get_builtin(uint8_t index);

/**
 * Get number of built-in presets
 */
uint8_t ahx_preset_get_builtin_count(void);

/**
 * Free preset resources (PList memory)
 * @param preset Preset to clean up
 */
void ahx_preset_free(AhxPreset* preset);

#ifdef __cplusplus
}
#endif

#endif // AHX_PRESET_H
