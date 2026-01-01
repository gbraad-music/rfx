#include "audio_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void audio_cache_get_paths(const char* audio_path, char* rtx_path_out, char* cache_path_out) {
    // Create RTX path: audio.mp3 -> audio.mp3.rtx
    snprintf(rtx_path_out, 512, "%s.rtx", audio_path);

    // Create cache path: audio.mp3 -> audio.mp3.rtxcache
    snprintf(cache_path_out, 512, "%s.rtxcache", audio_path);
}

// Helper: Get filename from filepath
static const char* get_filename(const char *filepath) {
    const char *last_sep = NULL;
    for (const char *p = filepath; *p; p++) {
        if (*p == '/' || *p == '\\') {
            last_sep = p;
        }
    }
    return last_sep ? (last_sep + 1) : filepath;
}

bool audio_cache_is_valid(
    const char* audio_path,
    size_t num_samples,
    int sample_rate
) {
    char rtx_path[512];
    char cache_path[512];
    audio_cache_get_paths(audio_path, rtx_path, cache_path);

    // Check if both RTX and cache files exist
    struct stat rtx_stat, cache_stat, audio_stat;
    if (stat(rtx_path, &rtx_stat) != 0 || stat(cache_path, &cache_stat) != 0) {
        return false;
    }

    // Check if cache is newer than source file
    if (stat(audio_path, &audio_stat) != 0) {
        return false;
    }

    if (rtx_stat.st_mtime < audio_stat.st_mtime || cache_stat.st_mtime < audio_stat.st_mtime) {
        // Cache is older than source - invalid
        return false;
    }

    // Read RTX metadata to verify it matches
    FILE* f = fopen(rtx_path, "r");
    if (!f) {
        return false;
    }

    char line[1024];
    int found_samples = 0;
    int found_sr = 0;

    while (fgets(line, sizeof(line), f)) {
        // Trim newline
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines, comments, and section headers
        if (line[0] == 0 || line[0] == ';' || line[0] == '#' || line[0] == '[') {
            continue;
        }

        // Parse key=value
        char key[128], value[512];
        if (sscanf(line, "%127[^=]=%511[^\n]", key, value) == 2) {
            // Trim whitespace
            char* k = key;
            while (*k == ' ' || *k == '\t') k++;
            char* k_end = k + strlen(k) - 1;
            while (k_end > k && (*k_end == ' ' || *k_end == '\t')) *k_end-- = 0;

            char* v = value;
            while (*v == ' ' || *v == '\t') v++;
            char* v_end = v + strlen(v) - 1;
            while (v_end > v && (*v_end == ' ' || *v_end == '\t')) *v_end-- = 0;

            if (strcmp(k, "total_frames") == 0) {
                size_t cached_samples = (size_t)atoll(v);
                if (cached_samples == num_samples) found_samples = 1;
            } else if (strcmp(k, "sample_rate") == 0) {
                int cached_sr = atoi(v);
                if (cached_sr == sample_rate) found_sr = 1;
            }
        }
    }

    fclose(f);

    return found_samples && found_sr;
}

bool audio_cache_load(
    const char* audio_path,
    AudioCache* cache_out
) {
    if (!cache_out) {
        return false;
    }

    char rtx_path[512];
    char cache_path[512];
    audio_cache_get_paths(audio_path, rtx_path, cache_path);

    // Read RTX metadata (INI format)
    FILE* f = fopen(rtx_path, "r");
    if (!f) {
        return false;
    }

    // Initialize metadata
    memset(&cache_out->metadata, 0, sizeof(AudioCacheMetadata));
    strncpy(cache_out->metadata.original_path, audio_path, sizeof(cache_out->metadata.original_path) - 1);

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // Trim newline
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines, comments, and section headers
        if (line[0] == 0 || line[0] == ';' || line[0] == '#' || line[0] == '[') {
            continue;
        }

        // Parse key=value
        char key[128], value[512];
        if (sscanf(line, "%127[^=]=%511[^\n]", key, value) == 2) {
            // Trim whitespace
            char* k = key;
            while (*k == ' ' || *k == '\t') k++;
            char* k_end = k + strlen(k) - 1;
            while (k_end > k && (*k_end == ' ' || *k_end == '\t')) *k_end-- = 0;

            char* v = value;
            while (*v == ' ' || *v == '\t') v++;
            char* v_end = v + strlen(v) - 1;
            while (v_end > v && (*v_end == ' ' || *v_end == '\t')) *v_end-- = 0;

            // Parse metadata fields
            if (strcmp(k, "bpm") == 0) {
                cache_out->metadata.bpm = (float)atof(v);
            } else if (strcmp(k, "sample_rate") == 0) {
                cache_out->metadata.sample_rate = atoi(v);
            } else if (strcmp(k, "channels") == 0) {
                cache_out->metadata.channels = atoi(v);
            } else if (strcmp(k, "total_frames") == 0) {
                cache_out->metadata.num_samples = (size_t)atoll(v);
            } else if (strcmp(k, "duration") == 0) {
                cache_out->metadata.duration = (float)atof(v);
            } else if (strcmp(k, "waveform_downsample") == 0) {
                cache_out->metadata.downsample = (size_t)atoi(v);
            } else if (strcmp(k, "waveform_length") == 0) {
                cache_out->metadata.num_frames = (size_t)atoi(v);
            }
        }
    }

    fclose(f);

    // Validate we got required metadata
    if (cache_out->metadata.num_frames == 0) {
        return false;
    }

    // Read binary cache file
    FILE* cache_file = fopen(cache_path, "rb");
    if (!cache_file) {
        return false;
    }

    // Allocate frames
    cache_out->frames = (WaveformFrame*)malloc(
        cache_out->metadata.num_frames * sizeof(WaveformFrame)
    );

    if (!cache_out->frames) {
        fclose(cache_file);
        return false;
    }

    // Read waveform data
    size_t frames_read = fread(
        cache_out->frames,
        sizeof(WaveformFrame),
        cache_out->metadata.num_frames,
        cache_file
    );

    fclose(cache_file);

    if (frames_read != cache_out->metadata.num_frames) {
        free(cache_out->frames);
        cache_out->frames = NULL;
        return false;
    }

    return true;
}

bool audio_cache_save(
    const char* audio_path,
    const AudioCacheMetadata* metadata,
    const WaveformFrame* frames,
    size_t num_frames
) {
    if (!metadata || !frames || num_frames == 0) {
        return false;
    }

    char rtx_path[512];
    char cache_path[512];
    audio_cache_get_paths(audio_path, rtx_path, cache_path);

    // Write RTX metadata (INI format)
    FILE* f = fopen(rtx_path, "w");
    if (!f) {
        return false;
    }

    // Get just the filename for portability
    const char* filename = get_filename(audio_path);

    // Write metadata section
    fprintf(f, "[metadata]\n");
    fprintf(f, "version=1.0\n");
    fprintf(f, "bpm=%.2f\n", metadata->bpm);
    fprintf(f, "\n");

    // Write channel section (we treat mono/stereo files as single channel for waveform)
    fprintf(f, "[channel_0]\n");
    fprintf(f, "filepath=%s\n", filename);
    fprintf(f, "bpm=%.2f\n", metadata->bpm);
    fprintf(f, "sample_rate=%d\n", metadata->sample_rate);
    fprintf(f, "channels=%d\n", metadata->channels);
    fprintf(f, "total_frames=%zu\n", metadata->num_samples);
    fprintf(f, "duration=%.6f\n", metadata->duration);
    fprintf(f, "waveform_downsample=%zu\n", metadata->downsample);
    fprintf(f, "waveform_length=%zu\n", num_frames);
    fprintf(f, "\n");

    fclose(f);

    // Write binary cache file
    FILE* cache_file = fopen(cache_path, "wb");
    if (!cache_file) {
        // Cleanup RTX file on failure
        remove(rtx_path);
        return false;
    }

    // Write waveform frames
    size_t frames_written = fwrite(
        frames,
        sizeof(WaveformFrame),
        num_frames,
        cache_file
    );

    fclose(cache_file);

    if (frames_written != num_frames) {
        // Failed to write all frames - cleanup both files
        remove(rtx_path);
        remove(cache_path);
        return false;
    }

    return true;
}

void audio_cache_free(AudioCache* cache) {
    if (cache && cache->frames) {
        free(cache->frames);
        cache->frames = NULL;
    }
}
