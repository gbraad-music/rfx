/**
 * Desktop MIDI Handler (Linux/Windows)
 * Uses RtMIDI for cross-platform MIDI support
 */

#include "midi_handler.h"
#include <rtmidi_c.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

static RtMidiInPtr g_midi_in = nullptr;
static midi_callback_t g_callback = nullptr;
static void* g_user_data = nullptr;
static int g_current_device = -1;

// RtMIDI callback wrapper
static void rtmidi_callback(double timeStamp, const unsigned char* message, size_t messageSize, void* userData) {
    if (!g_callback || messageSize < 1) return;

    midi_message_t msg;
    msg.status = message[0];
    msg.data1 = (messageSize >= 2) ? message[1] : 0;
    msg.data2 = (messageSize >= 3) ? message[2] : 0;
    msg.timestamp = (uint32_t)(timeStamp * 1000.0);  // Convert to milliseconds

    g_callback(&msg, g_user_data);
}

bool midi_handler_init(void) {
#ifndef _WIN32
    // On Linux, check if ALSA sequencer is available
    if (access("/dev/snd/seq", F_OK) != 0) {
        fprintf(stderr, "MIDI: ALSA sequencer not available\n");
        return true;  // Return true to allow app to continue without MIDI
    }
#endif

    g_midi_in = rtmidi_in_create_default();
    if (!g_midi_in) {
        fprintf(stderr, "MIDI: Failed to create RtMIDI input\n");
        return false;
    }

    printf("MIDI: Initialized desktop MIDI handler\n");
    return true;
}

void midi_handler_cleanup(void) {
    midi_handler_close_device();

    if (g_midi_in) {
        rtmidi_in_free(g_midi_in);
        g_midi_in = nullptr;
    }

    g_callback = nullptr;
    g_user_data = nullptr;
    printf("MIDI: Cleanup complete\n");
}

void midi_handler_set_callback(midi_callback_t callback, void* user_data) {
    g_callback = callback;
    g_user_data = user_data;
}

bool midi_handler_open_device(int device_index) {
    if (!g_midi_in) {
        fprintf(stderr, "MIDI: Handler not initialized\n");
        return false;
    }

    // Close current device if open
    midi_handler_close_device();

    unsigned int port_count = rtmidi_get_port_count(g_midi_in);
    if (device_index < 0 || device_index >= (int)port_count) {
        fprintf(stderr, "MIDI: Invalid device index %d (available: %u)\n", device_index, port_count);
        return false;
    }

    // Open the port (rtmidi_open_port returns void)
    char port_name[64];
    snprintf(port_name, sizeof(port_name), "junglizer-midi-in");
    rtmidi_open_port(g_midi_in, device_index, port_name);

    // Set callback
    rtmidi_in_set_callback(g_midi_in, rtmidi_callback, nullptr);

    // Don't ignore any message types
    rtmidi_in_ignore_types(g_midi_in, false, false, false);

    g_current_device = device_index;

    char device_name[256];
    int bufsize = sizeof(device_name);
    rtmidi_get_port_name(g_midi_in, device_index, device_name, &bufsize);
    printf("MIDI: Opened device %d: %s\n", device_index, device_name);

    return true;
}

void midi_handler_close_device(void) {
    if (g_midi_in && g_current_device >= 0) {
        rtmidi_close_port(g_midi_in);
        g_current_device = -1;
        printf("MIDI: Closed device\n");
    }
}

int midi_handler_get_device_count(void) {
    if (!g_midi_in) return 0;

#ifndef _WIN32
    // On Linux, check if ALSA sequencer is available
    if (access("/dev/snd/seq", F_OK) != 0) {
        return 0;
    }
#endif

    return (int)rtmidi_get_port_count(g_midi_in);
}

const char* midi_handler_get_device_name(int device_index) {
    static char device_name[256];

    if (!g_midi_in) {
        snprintf(device_name, sizeof(device_name), "MIDI not initialized");
        return device_name;
    }

    unsigned int port_count = rtmidi_get_port_count(g_midi_in);
    if (device_index < 0 || device_index >= (int)port_count) {
        snprintf(device_name, sizeof(device_name), "Invalid device");
        return device_name;
    }

    int bufsize = sizeof(device_name);
    rtmidi_get_port_name(g_midi_in, device_index, device_name, &bufsize);

    return device_name;
}

bool midi_handler_send_message(const midi_message_t* message) {
    // Desktop version doesn't support MIDI output (input only for now)
    // Could be implemented later using RtMidiOut if needed
    (void)message;
    return false;
}
