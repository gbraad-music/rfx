#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    char chunkID[4];      // "RIFF"
    uint32_t chunkSize;
    char format[4];       // "WAVE"
    char subchunk1ID[4];  // "fmt "
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char subchunk2ID[4];  // "data"
    uint32_t subchunk2Size;
} WavHeader;
#pragma pack(pop)

int main() {
    FILE *f = fopen("e:\\Modules\\gen-33.mod", "rb");
    if (!f) { printf("Failed to open file\n"); return 1; }

    // Read sample lengths and metadata
    uint32_t lengths[31];
    uint32_t loop_starts[31];
    uint32_t loop_lengths[31];
    char names[31][23];

    for(int i=0; i<31; i++) {
        fseek(f, 20 + i*30, SEEK_SET);
        fread(names[i], 1, 22, f);
        names[i][22] = 0;

        uint8_t len_bytes[2];
        fread(len_bytes, 1, 2, f);
        lengths[i] = ((len_bytes[0]<<8)|len_bytes[1]);

        fseek(f, 20 + i*30 + 26, SEEK_SET);
        fread(len_bytes, 1, 2, f);
        loop_starts[i] = ((len_bytes[0]<<8)|len_bytes[1]);

        fread(len_bytes, 1, 2, f);
        loop_lengths[i] = ((len_bytes[0]<<8)|len_bytes[1]);
    }

    // Get num patterns
    fseek(f, 950, SEEK_SET);
    uint8_t song_len;
    fread(&song_len, 1, 1, f);

    fseek(f, 952, SEEK_SET);
    uint8_t positions[128];
    fread(positions, 1, 128, f);

    // Scan ALL 128 positions to find max pattern number (not just song_len!)
    uint8_t max_pattern = 0;
    for(int i=0; i<128; i++) {
        if(positions[i] > max_pattern) max_pattern = positions[i];
    }
    uint32_t num_patterns = max_pattern + 1;
    printf("Max pattern in file: %d, total patterns: %d\n", max_pattern, num_patterns);

    // Sample data offset
    uint32_t offset = 1084 + (num_patterns * 64 * 4 * 4);

    // Dump each sample
    for(int i=0; i<31; i++) {
        if(lengths[i] == 0) {
            // Even zero-length samples take up space in the numbering
            printf("Sample %02X (index %d): ZERO LENGTH, skipping\n", i+1, i);
            continue;
        }

        uint32_t sample_len_bytes = lengths[i] * 2;

        // Debug output for samples 14 and 15
        if(i == 13 || i == 14) {
            printf("\n=== EXTRACTING Sample %02X (index %d) ===\n", i+1, i);
            printf("  Header says: length=%d words (%d bytes)\n", lengths[i], sample_len_bytes);
            printf("  Reading from file offset: %u (0x%X)\n", offset, offset);
        }

        // Read sample data
        int8_t *data = (int8_t*)malloc(sample_len_bytes);
        fseek(f, offset, SEEK_SET);
        fread(data, 1, sample_len_bytes, f);

        // Show first 4 bytes
        if(i == 13 || i == 14) {
            printf("  First 4 bytes read: %02X %02X %02X %02X\n",
                    (uint8_t)data[0], (uint8_t)data[1], (uint8_t)data[2], (uint8_t)data[3]);
        }

        // Create WAV file
        char filename[256];
        sprintf(filename, "e:\\Samples\\gen33-sample%02X.wav", i+1);

        FILE *wav = fopen(filename, "wb");
        if(wav) {
            WavHeader hdr;
            memcpy(hdr.chunkID, "RIFF", 4);
            memcpy(hdr.format, "WAVE", 4);
            memcpy(hdr.subchunk1ID, "fmt ", 4);
            memcpy(hdr.subchunk2ID, "data", 4);

            hdr.subchunk1Size = 16;
            hdr.audioFormat = 1;
            hdr.numChannels = 1;
            hdr.sampleRate = 16574;  // C-3
            hdr.bitsPerSample = 8;
            hdr.byteRate = hdr.sampleRate * hdr.numChannels * hdr.bitsPerSample / 8;
            hdr.blockAlign = hdr.numChannels * hdr.bitsPerSample / 8;
            hdr.subchunk2Size = sample_len_bytes;
            hdr.chunkSize = 36 + hdr.subchunk2Size;

            fwrite(&hdr, sizeof(WavHeader), 1, wav);

            // Convert signed to unsigned for WAV
            for(uint32_t j=0; j<sample_len_bytes; j++) {
                uint8_t val = (uint8_t)(data[j] + 128);
                fwrite(&val, 1, 1, wav);
            }

            fclose(wav);
            printf("Wrote %s (sample #%d, %d bytes, loop=%d+%d)\n",
                    filename, i+1, sample_len_bytes, loop_starts[i], loop_lengths[i]);
        }

        free(data);
        offset += sample_len_bytes;
    }

    fclose(f);
    printf("\nDone! Samples written to e:\\Samples\\gen33-sample01.wav through gen33-sample1F.wav\n");
    return 0;
}
