/**
 * SDL Audio Initialization
 * Shared initialization code for SDL3 audio across all apps
 */

#ifndef SDL_AUDIO_INIT_H
#define SDL_AUDIO_INIT_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SDL audio subsystem
 * Call this AFTER SDL_Init(SDL_INIT_VIDEO)
 *
 * Returns: true on success, false on failure
 */
static inline bool sdl_audio_init(void) {
    // SDL3: SDL_Init returns bool (true=success, false=failure)
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to initialize SDL audio: %s", SDL_GetError());
        return false;
    }
    return true;
}

/**
 * Create and configure an audio output stream
 *
 * @param sample_rate Sample rate (e.g., 48000)
 * @param callback Audio callback function
 * @param userdata User data for callback
 * @param out_device Pointer to store device ID (can be NULL)
 *
 * Returns: SDL_AudioStream* on success, NULL on failure
 */
static inline SDL_AudioStream* sdl_audio_create_output_stream(
    int sample_rate,
    SDL_AudioStreamCallback callback,
    void* userdata,
    SDL_AudioDeviceID* out_device)
{
    SDL_AudioSpec spec;
    spec.freq = sample_rate;
    spec.format = SDL_AUDIO_F32;  // 32-bit float
    spec.channels = 2;            // Stereo

    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        nullptr,
        nullptr
    );

    if (!stream) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio stream: %s", SDL_GetError());
        return nullptr;
    }

    SDL_SetAudioStreamGetCallback(stream, callback, userdata);

    SDL_AudioDeviceID device = SDL_GetAudioStreamDevice(stream);
    if (out_device) {
        *out_device = device;
    }

    if (!SDL_ResumeAudioDevice(device)) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to resume audio device: %s", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        return nullptr;
    }

    SDL_Log("Audio initialized: %d Hz, stereo, F32", sample_rate);
    return stream;
}

#ifdef __cplusplus
}
#endif

#endif // SDL_AUDIO_INIT_H
