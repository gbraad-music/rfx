#ifndef WAV_LOADER_H
#define WAV_LOADER_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// WAV file header structures
#pragma pack(push, 1)
typedef struct {
  char riff[4];           // "RIFF"
  uint32_t file_size;
  char wave[4];           // "WAVE"
} WAVHeader;

typedef struct {
  char fmt[4];            // "fmt "
  uint32_t fmt_size;
  uint16_t audio_format;  // 1 = PCM
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
} WAVFmt;

typedef struct {
  char data[4];           // "data"
  uint32_t data_size;
} WAVData;
#pragma pack(pop)

typedef struct {
  int16_t* pcm_data;      // Mono int16 PCM data
  uint32_t num_samples;   // Number of samples
  uint32_t sample_rate;   // Sample rate in Hz
} WAVSample;

/**
 * Load WAV file from path
 * Returns allocated WAVSample on success, NULL on failure
 * Caller must free result->pcm_data when done
 */
static inline WAVSample* wav_load_file(const char* wav_path) {
  FILE* wav_file = fopen(wav_path, "rb");
  if (!wav_file) {
    return NULL;
  }

  // Read WAV header
  WAVHeader header;
  if (fread(&header, sizeof(WAVHeader), 1, wav_file) != 1) {
    fclose(wav_file);
    return NULL;
  }

  // Verify RIFF/WAVE
  if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0) {
    fclose(wav_file);
    return NULL;
  }

  // Read fmt chunk
  WAVFmt fmt;
  if (fread(&fmt, sizeof(WAVFmt), 1, wav_file) != 1) {
    fclose(wav_file);
    return NULL;
  }

  // Verify PCM format
  if (memcmp(fmt.fmt, "fmt ", 4) != 0 || fmt.audio_format != 1) {
    fclose(wav_file);
    return NULL;
  }

  // Skip extra fmt bytes if any
  if (fmt.fmt_size > 16) {
    fseek(wav_file, fmt.fmt_size - 16, SEEK_CUR);
  }

  // Find data chunk
  WAVData data_chunk;
  while (fread(&data_chunk, sizeof(WAVData), 1, wav_file) == 1) {
    if (memcmp(data_chunk.data, "data", 4) == 0) {
      break;
    }
    // Skip unknown chunk
    fseek(wav_file, data_chunk.data_size, SEEK_CUR);
  }

  if (memcmp(data_chunk.data, "data", 4) != 0) {
    fclose(wav_file);
    return NULL;
  }

  // Calculate number of samples
  uint32_t bytes_per_sample = fmt.bits_per_sample / 8;
  uint32_t num_samples = data_chunk.data_size / (bytes_per_sample * fmt.num_channels);

  if (num_samples == 0 || num_samples > 10000000) {
    fclose(wav_file);
    return NULL;  // Invalid or too large
  }

  // Allocate PCM buffer (mono int16)
  int16_t* pcm_data = (int16_t*)malloc(num_samples * sizeof(int16_t));
  if (!pcm_data) {
    fclose(wav_file);
    return NULL;
  }

  // Read and convert samples
  if (fmt.bits_per_sample == 16) {
    // 16-bit PCM
    if (fmt.num_channels == 1) {
      // Mono - direct read
      fread(pcm_data, sizeof(int16_t), num_samples, wav_file);
    } else {
      // Stereo - downmix to mono
      int16_t stereo_samples[2];
      for (uint32_t i = 0; i < num_samples; i++) {
        fread(stereo_samples, sizeof(int16_t), 2, wav_file);
        pcm_data[i] = (stereo_samples[0] / 2) + (stereo_samples[1] / 2);
      }
    }
  } else if (fmt.bits_per_sample == 8) {
    // 8-bit PCM (unsigned)
    uint8_t* temp_buffer = (uint8_t*)malloc(num_samples * fmt.num_channels);
    if (temp_buffer) {
      fread(temp_buffer, 1, num_samples * fmt.num_channels, wav_file);
      for (uint32_t i = 0; i < num_samples; i++) {
        if (fmt.num_channels == 1) {
          pcm_data[i] = ((int16_t)temp_buffer[i] - 128) << 8;
        } else {
          pcm_data[i] = (((int16_t)temp_buffer[i * 2] - 128) << 7) +
                         (((int16_t)temp_buffer[i * 2 + 1] - 128) << 7);
        }
      }
      free(temp_buffer);
    }
  }

  fclose(wav_file);

  // Allocate result structure
  WAVSample* result = (WAVSample*)malloc(sizeof(WAVSample));
  if (!result) {
    free(pcm_data);
    return NULL;
  }

  result->pcm_data = pcm_data;
  result->num_samples = num_samples;
  result->sample_rate = fmt.sample_rate;

  return result;
}

/**
 * Free WAVSample structure
 */
static inline void wav_free_sample(WAVSample* sample) {
  if (sample) {
    if (sample->pcm_data) {
      free(sample->pcm_data);
    }
    free(sample);
  }
}

#ifdef __cplusplus
}
#endif

#endif // WAV_LOADER_H
