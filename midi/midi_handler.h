#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// MIDI message types
typedef enum {
    MIDI_NOTE_OFF = 0x80,
    MIDI_NOTE_ON = 0x90,
    MIDI_POLY_PRESSURE = 0xA0,
    MIDI_CONTROL_CHANGE = 0xB0,
    MIDI_PROGRAM_CHANGE = 0xC0,
    MIDI_CHANNEL_PRESSURE = 0xD0,
    MIDI_PITCH_BEND = 0xE0,
    MIDI_SYSTEM = 0xF0
} midi_message_type_t;

// MIDI message structure
typedef struct {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint32_t timestamp;
} midi_message_t;

// MIDI callback function type
typedef void (*midi_callback_t)(const midi_message_t* message, void* user_data);

// Initialize MIDI handler
bool midi_handler_init(void);

// Cleanup MIDI handler
void midi_handler_cleanup(void);

// Set callback for incoming MIDI messages
void midi_handler_set_callback(midi_callback_t callback, void* user_data);

// Open MIDI device by index
bool midi_handler_open_device(int device_index);

// Close current MIDI device
void midi_handler_close_device(void);

// Get number of available MIDI devices
int midi_handler_get_device_count(void);

// Get device name by index
const char* midi_handler_get_device_name(int device_index);

// Send MIDI message
bool midi_handler_send_message(const midi_message_t* message);

// Helper functions to parse MIDI messages
static inline uint8_t midi_get_channel(uint8_t status) {
    return status & 0x0F;
}

static inline uint8_t midi_get_type(uint8_t status) {
    return status & 0xF0;
}

#ifdef __cplusplus
}
#endif

#endif // MIDI_HANDLER_H
