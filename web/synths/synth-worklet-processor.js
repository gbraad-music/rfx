// Generic AudioWorklet Processor for WASM Synths
// Handles MIDI events and audio generation for RGResonate1, RG1Piano, etc.
// VERSION v171 - Made generic to support multiple WASM engines

class SynthWorkletProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();
        console.log('[SynthWorklet] ✅ LOADED v173 - CommonJS capture');
        this.wasmModule = null;
        this.synthPtr = null;
        this.audioBufferPtr = null;
        this.bufferSize = 128; // AudioWorklet quantum size
        this.sampleRate = 48000;

        // WASM function name mappings (will be filled after examining WASM exports)
        this.wasmFuncs = {
            create: null,
            destroy: null,
            note_on: null,
            note_off: null,
            all_notes_off: null,
            process: null,
            reset: null,
            set_parameter_value: null
        };

        this.port.onmessage = this.handleMessage.bind(this);

        // Request WASM bytes from main thread
        this.port.postMessage({ type: 'needWasm' });
    }

    handleMessage(event) {
        const { type, data } = event.data;

        if (type === 'wasmBytes') {
            this.initWasm(data, data.sampleRate || 48000);
        } else if (type === 'noteOn') {
            this.handleNoteOn(data.note, data.velocity);
        } else if (type === 'noteOff') {
            this.handleNoteOff(data.note);
        } else if (type === 'allNotesOff') {
            this.allNotesOff();
        } else if (type === 'setParam') {
            this.setParameter(data.index, data.value);
        } else if (type === 'reset') {
            this.reset();
        } else if (type === 'plist_import') {
            this.importPreset(data.buffer);
        } else if (type === 'plist_export') {
            this.exportPreset(data.presetName || 'MyPreset');
        }
    }

    importPreset(buffer) {
        if (!this.wasmModule || !this.synthPtr) {
            console.error('[SynthWorklet] Cannot import preset: WASM not initialized');
            return;
        }

        try {
            // Stop all notes before importing
            this.allNotesOff();

            // Allocate buffer in WASM memory
            const bufferPtr = this.wasmModule._malloc(buffer.length);
            const heapU8 = new Uint8Array(this.wasmMemory.buffer, bufferPtr, buffer.length);
            heapU8.set(buffer);

            // Call C import function (handles all parsing)
            const result = this.wasmModule._regroove_synth_import_preset(this.synthPtr, bufferPtr, buffer.length);

            // Free buffer
            this.wasmModule._free(bufferPtr);

            if (result) {
                console.log('[SynthWorklet] .ahxp preset imported successfully');
                this.port.postMessage({ type: 'preset_imported', data: { success: true } });
            } else {
                console.error('[SynthWorklet] Preset import failed');
                this.port.postMessage({ type: 'preset_imported', data: { success: false } });
            }
        } catch (error) {
            console.error('[SynthWorklet] Import error:', error);
            this.port.postMessage({ type: 'preset_imported', data: { success: false, error: error.message } });
        }
    }

    exportPreset(presetName) {
        if (!this.wasmModule || !this.synthPtr) {
            console.error('[SynthWorklet] Cannot export preset: WASM not initialized');
            return;
        }

        try {
            // Allocate size output parameter
            const sizePtr = this.wasmModule._malloc(4);
            const heapU32 = new Uint32Array(this.wasmMemory.buffer, sizePtr, 1);

            // Call C export function (writes preset + PList data to buffer)
            const namePtr = this.wasmModule._malloc(presetName.length + 1);
            const heapU8Name = new Uint8Array(this.wasmMemory.buffer, namePtr, presetName.length + 1);
            for (let i = 0; i < presetName.length; i++) {
                heapU8Name[i] = presetName.charCodeAt(i);
            }
            heapU8Name[presetName.length] = 0;

            const bufferPtr = this.wasmModule._regroove_synth_export_preset(this.synthPtr, namePtr, sizePtr);
            const size = heapU32[0];

            this.wasmModule._free(namePtr);
            this.wasmModule._free(sizePtr);

            if (!bufferPtr || size === 0) {
                console.error('[SynthWorklet] Export failed - no data');
                return;
            }

            // Copy buffer to JavaScript
            const buffer = new Uint8Array(size);
            const heapU8 = new Uint8Array(this.wasmMemory.buffer, bufferPtr, size);
            buffer.set(heapU8);

            // Free C buffer
            this.wasmModule._regroove_synth_free_preset_buffer(bufferPtr);

            console.log(`[SynthWorklet] Exported preset (${size} bytes)`);

            // Send to main thread
            this.port.postMessage({
                type: 'preset_exported',
                data: {
                    name: presetName,
                    buffer: buffer,
                    format: 'binary'
                }
            });
        } catch (error) {
            console.error('[SynthWorklet] Export error:', error);
        }
    }

    async initWasm(wasmData, sampleRate) {
        try {
            console.log('[SynthWorklet] Loading Emscripten module...');
            this.sampleRate = sampleRate;

            const moduleCode = wasmData.jsCode;
            const wasmBytes = wasmData.wasmBytes;
            const engineId = wasmData.engine || 0;

            // Create fake CommonJS environment to capture the module export
            // Emscripten modules end with: if(typeof exports==="object"&&typeof module==="object"){module.exports=...}
            const fakeExports = {};
            const fakeModule = { exports: fakeExports };

            // Inject code to capture wasmMemory and eval in a scope with fake module/exports
            const modifiedCode = moduleCode.replace(
                ';return moduleRtn',
                ';globalThis.__wasmMemory=wasmMemory;return moduleRtn'
            );

            // Execute in a function scope with module and exports defined
            (function(module, exports) {
                eval(modifiedCode);
            })(fakeModule, fakeExports);

            // Get the module factory from the fake exports
            const ModuleFactory = fakeModule.exports || fakeModule.exports.default;
            if (!ModuleFactory) {
                throw new Error('Failed to capture WASM module factory');
            }

            // Call the factory with WASM bytes
            this.wasmModule = await ModuleFactory({
                wasmBinary: wasmBytes
            });

            // Capture the memory reference
            this.wasmMemory = globalThis.__wasmMemory;
            delete globalThis.__wasmMemory;

            console.log('[SynthWorklet] WASM ready');

            // Map function names
            this.mapWasmFunctions();

            // Allocate audio buffer (stereo interleaved)
            this.audioBufferPtr = this.wasmModule._malloc(this.bufferSize * 2 * 4);

            if (!this.audioBufferPtr) {
                throw new Error('_malloc returned null - memory allocation failed');
            }

            console.log(`[SynthWorklet] Buffer: 0x${this.audioBufferPtr.toString(16)}`);

            // Create synth instance (engine ID passed from main thread, sampleRate)
            if (this.wasmFuncs.create) {
                this.synthPtr = this.wasmFuncs.create(engineId, this.sampleRate);
                console.log(`[SynthWorklet] Synth created (engine ${engineId}): 0x${this.synthPtr.toString(16)}`);
            } else {
                throw new Error('regroove_synth_create not found in WASM exports');
            }

            this.port.postMessage({ type: 'ready' });
            console.log('[SynthWorklet] ✅ Ready!');
        } catch (error) {
            console.error('[SynthWorklet] ❌ Failed:', error);
            this.port.postMessage({ type: 'error', error: error.message });
        }
    }

    mapWasmFunctions() {
        // Debug: List available functions
        const exportedFuncs = Object.keys(this.wasmModule).filter(k => k.startsWith('_'));
        console.log('[SynthWorklet] Available exports:', exportedFuncs.slice(0, 20));

        // Map Emscripten Module exports
        this.wasmFuncs.create = this.wasmModule._regroove_synth_create;
        this.wasmFuncs.destroy = this.wasmModule._regroove_synth_destroy;
        this.wasmFuncs.reset = this.wasmModule._regroove_synth_reset;
        this.wasmFuncs.note_on = this.wasmModule._regroove_synth_note_on;
        this.wasmFuncs.note_off = this.wasmModule._regroove_synth_note_off;
        this.wasmFuncs.all_notes_off = this.wasmModule._regroove_synth_all_notes_off;
        this.wasmFuncs.process = this.wasmModule._regroove_synth_process_f32;
        this.wasmFuncs.set_parameter_value = this.wasmModule._regroove_synth_set_parameter;

        console.log('[SynthWorklet] ✓ Mapped synth functions');
    }

    handleNoteOn(note, velocity) {
        if (!this.synthPtr || !this.wasmFuncs.note_on) return;
        this.wasmFuncs.note_on(this.synthPtr, note, velocity);
    }

    handleNoteOff(note) {
        if (!this.synthPtr || !this.wasmFuncs.note_off) return;
        this.wasmFuncs.note_off(this.synthPtr, note);
    }

    allNotesOff() {
        if (!this.synthPtr || !this.wasmFuncs.all_notes_off) return;
        this.wasmFuncs.all_notes_off(this.synthPtr);
    }

    setParameter(index, value) {
        if (!this.synthPtr || !this.wasmFuncs.set_parameter_value) return;
        this.wasmFuncs.set_parameter_value(this.synthPtr, index, value);
    }

    reset() {
        if (!this.synthPtr || !this.wasmFuncs.reset) return;
        this.wasmFuncs.reset(this.synthPtr);
    }

    process(inputs, outputs, parameters) {
        if (!this.wasmModule || !this.synthPtr || !this.audioBufferPtr || !this.wasmFuncs.process) {
            // Output silence
            return true;
        }

        const output = outputs[0];
        if (!output || output.length === 0) {
            return true;
        }

        const frames = output[0].length;

        // Update heap view (use saved memory reference)
        const heapF32 = new Float32Array(
            this.wasmMemory.buffer,
            this.audioBufferPtr,
            frames * 2
        );

        // Zero the buffer
        heapF32.fill(0);

        // Process audio through synth
        this.wasmFuncs.process(this.synthPtr, this.audioBufferPtr, frames, this.sampleRate);

        // De-interleave output
        const outputL = output[0];
        const outputR = output[1] || output[0];

        for (let i = 0; i < frames; i++) {
            outputL[i] = heapF32[i * 2];
            outputR[i] = heapF32[i * 2 + 1];
        }

        return true;
    }
}

registerProcessor('synth-worklet-processor', SynthWorkletProcessor);
