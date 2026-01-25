#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Load the Deck Player WebAssembly module (includes AHX support)
// Note: This is an ES6 module, so we need to import it properly
const createModule = (async () => {
    const module = await import('../../web/players/deck-player.js');
    return module.default;
})();

async function test() {
    console.log('Loading Deck Player WebAssembly module (AHX support)...');

    const createDeckPlayerModule = await createModule;
    const module = await createDeckPlayerModule({
        // Capture emscripten console output
        print: (text) => console.log('[PLAYER]', text),
        printErr: (text) => console.error('[PLAYER ERROR]', text),
    });

    console.log('Player runtime initialized');

    // Get function pointers
    const player_create = module.cwrap('deck_player_create_wasm', 'number', []);
    const player_destroy = module.cwrap('deck_player_destroy_wasm', null, ['number']);
    const player_load = module.cwrap('deck_player_load_from_memory', 'number', ['number', 'array', 'number', 'string']);
    const player_start = module.cwrap('deck_player_start_wasm', null, ['number']);
    const player_process = module.cwrap('deck_player_process_f32', null, ['number', 'number', 'number', 'number']);
    const create_buffer = module.cwrap('deck_create_audio_buffer', 'number', ['number']);

    // Create player instance
    const player = player_create();
    console.log('Player instance created:', player);

    // Load AHX module file
    const modulePath = process.argv[2] || '/mnt/e/Modules/AHX/Hoffman/Chopper_13-modified.ahx';
    console.log('Loading module:', modulePath);

    const moduleData = Array.from(fs.readFileSync(modulePath));
    const filename = path.basename(modulePath);

    // Load module (deck player auto-detects format)
    const result = module.ccall(
        'deck_player_load_from_memory',
        'number',
        ['number', 'array', 'number', 'string'],
        [player, moduleData, moduleData.length, filename]
    );

    if (result !== 1) {
        console.error('Failed to load module');
        return;
    }
    console.log('Module loaded successfully\n');

    // Start playback
    player_start(player);

    // Process audio frames to see debug output
    const sampleRate = 48000;
    const bufferSize = 512;
    const bufferPtr = create_buffer(bufferSize * 2); // Stereo interleaved

    console.log('Processing audio frames...\n');

    // Process enough samples to see several PList entries
    // At 48kHz with 512 samples per buffer, each buffer = ~10.67ms
    // With SpeedMultiplier=3, frames happen every ~6.67ms (150Hz)
    // So we need ~100 buffers to see ~1 second of playback
    const numBuffers = 100;
    for (let i = 0; i < numBuffers; i++) {
        player_process(player, bufferPtr, bufferSize, sampleRate);

        // Log progress every 10 buffers
        if (i % 10 === 0) {
            const timeMs = (i * bufferSize / sampleRate * 1000).toFixed(1);
            console.log(`--- Processed ${i * bufferSize} samples (${timeMs}ms) ---\n`);
        }
    }

    // Cleanup
    player_destroy(player);

    console.log('\nDone!');
}

test().catch(err => {
    console.error('Error:', err);
    process.exit(1);
});
