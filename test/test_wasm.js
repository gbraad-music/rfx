#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Load the WebAssembly module
const Module = require('../../web/synths/rgahxsynth.js');

async function test() {
    console.log('Loading WebAssembly module...');

    const module = await new Promise((resolve) => {
        Module({
            // Capture emscripten console output
            print: (text) => console.log('[WASM]', text),
            printErr: (text) => console.error('[WASM ERROR]', text),
            onRuntimeInitialized: function() {
                console.log('WASM runtime initialized');
                console.log('Available HEAP arrays:', Object.keys(this).filter(k => k.startsWith('HEAP')));
                resolve(this);
            }
        });
    });

    console.log('Module keys:', Object.keys(module).slice(0, 20));

    // Get function pointers
    const synth_create = module.cwrap('regroove_synth_create', 'number', ['number', 'number']); // engine, sample_rate
    const synth_destroy = module.cwrap('regroove_synth_destroy', null, ['number']);
    const synth_import_preset = module.cwrap('regroove_synth_import_preset', 'number', ['number', 'number', 'number']);
    const synth_note_on = module.cwrap('regroove_synth_note_on', null, ['number', 'number', 'number']);
    const synth_note_off = module.cwrap('regroove_synth_note_off', null, ['number', 'number']);
    const synth_process_f32 = module.cwrap('regroove_synth_process_f32', null, ['number', 'number', 'number', 'number']);
    const create_buffer = module.cwrap('synth_create_audio_buffer', 'number', ['number']);
    const destroy_buffer = module.cwrap('synth_destroy_audio_buffer', null, ['number']);

    // Create synth instance (engine=0 for AHX)
    const synth = synth_create(0, 48000);
    console.log('Synth instance created:', synth);

    // Load preset file
    const presetPath = process.argv[2] || '../../tools/ahx_preset_tool/build-windows/Chopper/chopper_13.ahxp';
    console.log('Loading preset:', presetPath);

    const presetData = Array.from(fs.readFileSync(presetPath));

    // Use ccall with array parameter (Emscripten will handle memory copying)
    const result = module.ccall(
        'regroove_synth_import_preset',
        'number',
        ['number', 'array', 'number'],
        [synth, presetData, presetData.length]
    );

    if (result !== 1) {
        console.error('Failed to import preset');
        return;
    }
    console.log('Preset loaded successfully\n');

    // Trigger MIDI note
    const note = parseInt(process.argv[3]) || 60;
    const velocity = 100;
    console.log(`Triggering note ${note} with velocity ${velocity}\n`);
    synth_note_on(synth, note, velocity);

    // Process audio frames to see debug output
    const bufferSize = 512;
    const bufferPtr = create_buffer(bufferSize * 2); // Stereo interleaved

    console.log('Processing audio frames...\n');

    // Process enough samples to see several PList entries
    const numFrames = 100;
    for (let i = 0; i < numFrames; i++) {
        synth_process_f32(synth, bufferPtr, bufferSize, 48000);

        // Small delay to see frame-by-frame logs
        if (i % 10 === 0) {
            console.log(`--- Processed ${i * bufferSize} samples (${(i * bufferSize / 48000 * 1000).toFixed(1)}ms) ---\n`);
        }
    }

    // Optional: Write WAV file
    if (process.argv[4]) {
        console.log('\nRendering full audio to WAV...');

        const sampleRate = 48000;
        const duration = 2.0; // seconds
        const totalSamples = Math.floor(sampleRate * duration);

        const leftBuffer = new Float32Array(totalSamples);
        const rightBuffer = new Float32Array(totalSamples);

        // Reset synth
        synth_note_off(synth, note);
        synth_destroy(synth);
        const synth2 = synth_create(0, sampleRate);
        module.ccall(
            'regroove_synth_import_preset',
            'number',
            ['number', 'array', 'number'],
            [synth2, presetData, presetData.length]
        );
        synth_note_on(synth2, note, velocity);

        // Render
        let pos = 0;
        while (pos < totalSamples) {
            const chunk = Math.min(bufferSize, totalSamples - pos);
            synth_process_f32(synth2, leftPtr, rightPtr, chunk, sampleRate);

            const left = new Float32Array(module.HEAPF32.buffer, leftPtr, chunk);
            const right = new Float32Array(module.HEAPF32.buffer, rightPtr, chunk);

            leftBuffer.set(left, pos);
            rightBuffer.set(right, pos);
            pos += chunk;
        }

        // Write WAV
        writeWav(process.argv[4], leftBuffer, rightBuffer, sampleRate);
        console.log('Wrote WAV file:', process.argv[4]);

        synth_destroy(synth2);
    }

    // Cleanup
    destroy_buffer(bufferPtr);
    synth_destroy(synth);

    console.log('\nDone!');
}

function writeWav(filename, leftChannel, rightChannel, sampleRate) {
    const numChannels = 2;
    const numSamples = leftChannel.length;
    const bytesPerSample = 2; // 16-bit
    const dataSize = numSamples * numChannels * bytesPerSample;

    const buffer = Buffer.alloc(44 + dataSize);
    let offset = 0;

    // RIFF header
    buffer.write('RIFF', offset); offset += 4;
    buffer.writeUInt32LE(36 + dataSize, offset); offset += 4;
    buffer.write('WAVE', offset); offset += 4;

    // fmt chunk
    buffer.write('fmt ', offset); offset += 4;
    buffer.writeUInt32LE(16, offset); offset += 4; // chunk size
    buffer.writeUInt16LE(1, offset); offset += 2; // PCM
    buffer.writeUInt16LE(numChannels, offset); offset += 2;
    buffer.writeUInt32LE(sampleRate, offset); offset += 4;
    buffer.writeUInt32LE(sampleRate * numChannels * bytesPerSample, offset); offset += 4;
    buffer.writeUInt16LE(numChannels * bytesPerSample, offset); offset += 2;
    buffer.writeUInt16LE(bytesPerSample * 8, offset); offset += 2;

    // data chunk
    buffer.write('data', offset); offset += 4;
    buffer.writeUInt32LE(dataSize, offset); offset += 4;

    // Interleaved samples
    for (let i = 0; i < numSamples; i++) {
        const left = Math.max(-1, Math.min(1, leftChannel[i]));
        const right = Math.max(-1, Math.min(1, rightChannel[i]));
        buffer.writeInt16LE(Math.floor(left * 32767), offset); offset += 2;
        buffer.writeInt16LE(Math.floor(right * 32767), offset); offset += 2;
    }

    fs.writeFileSync(filename, buffer);
}

test().catch(err => {
    console.error('Error:', err);
    process.exit(1);
});
