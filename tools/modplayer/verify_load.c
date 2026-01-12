#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../../synth/mod_player.h"

int main() {
    FILE* f = fopen("/mnt/e/Modules/genchorus2.mod", "rb");
    if (!f) return 1;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);
    
    // Load using mod_player
    ModPlayer* player = mod_player_create();
    if (!mod_player_load(player, data, size)) {
        printf("Failed to load MOD\n");
        return 1;
    }
    
    // Check sample 15 (index 14)
    const ModSample* smp15 = &player->samples[14];
    printf("Sample 15 from player:\n");
    printf("  Length: %u words (%u bytes)\n", smp15->length, smp15->length * 2);
    printf("  Volume: %u\n", smp15->volume);
    printf("  First 16 bytes: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", (uint8_t)smp15->data[i]);
    }
    printf("\n");
    
    mod_player_destroy(player);
    free(data);
    return 0;
}
