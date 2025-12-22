/*
 * RGResonate1 WebAssembly Wrapper for JavaScript
 *
 * Usage in ../revision/ to replace midi-audio-synth.js:
 *
 * 1. Load WASM module:
 *    <script src="synths/rgresonate1-synth.js"></script>
 *
 * 2. Use in AudioContext:
 *    const synth = new RGResonate1Synth(audioContext);
 *    synth.handleNoteOn(60, 100);  // Middle C
 */

class RGResonate1Synth {
    constructor(audioContext) {
        this.audioContext = audioContext;
        this.wasmModule = null;
        this.synthInstance = null;
        this.isReady = false;

        // Audio processing
        this.analyser = null;
        this.scriptProcessor = null;
        this.audioBuffer = null;
        this.audioBufferPtr = null;

        // Initialize WASM module
        this.init();
    }

    async init() {
        console.log('[RGResonate1] Initializing WASM module...');

        // Load WASM module
        try {
            this.wasmModule = await RGResonate1Module();
            console.log('[RGResonate1] WASM module loaded');
        } catch (error) {
            console.error('[RGResonate1] Failed to load WASM:', error);
            return;
        }

        // Create synth instance (engine 0 = RESONATE1, sample rate)
        const SYNTH_ENGINE_RESONATE1 = 0;
        this.synthInstance = this.wasmModule._regroove_synth_create(
            SYNTH_ENGINE_RESONATE1,
            this.audioContext.sampleRate
        );

        if (!this.synthInstance) {
            console.error('[RGResonate1] Failed to create synth instance');
            return;
        }

        // Create audio buffer (512 frames stereo)
        const bufferSize = 512;
        const bufferSizeBytes = this.wasmModule._synth_get_buffer_size_bytes(bufferSize);
        this.audioBufferPtr = this.wasmModule._malloc(bufferSizeBytes);
        this.audioBuffer = new Float32Array(
            this.wasmModule.HEAPF32.buffer,
            this.audioBufferPtr,
            bufferSize * 2  // Stereo interleaved
        );

        // Create analyser (for Milkdrop visualization)
        this.analyser = this.audioContext.createAnalyser();
        this.analyser.fftSize = 8192;
        this.analyser.smoothingTimeConstant = 0.0;

        // Create ScriptProcessor for audio generation
        this.scriptProcessor = this.audioContext.createScriptProcessor(bufferSize, 0, 2);
        this.scriptProcessor.onaudioprocess = (e) => this.processAudio(e);

        // Connect: ScriptProcessor -> Analyser -> Destination
        this.scriptProcessor.connect(this.analyser);
        this.analyser.connect(this.audioContext.destination);

        this.isReady = true;
        console.log('[RGResonate1] ✓ Synth ready');

        // Log parameter info
        const paramCount = this.wasmModule._regroove_synth_get_parameter_count(this.synthInstance);
        console.log(`[RGResonate1] ${paramCount} parameters available`);
    }

    processAudio(event) {
        if (!this.isReady || !this.synthInstance) return;

        const outputL = event.outputBuffer.getChannelData(0);
        const outputR = event.outputBuffer.getChannelData(1);
        const frames = event.outputBuffer.length;

        // Generate audio from WASM synth
        this.wasmModule._regroove_synth_process_f32(
            this.synthInstance,
            this.audioBufferPtr,
            frames,
            this.audioContext.sampleRate
        );

        // De-interleave to output channels
        for (let i = 0; i < frames; i++) {
            outputL[i] = this.audioBuffer[i * 2 + 0];
            outputR[i] = this.audioBuffer[i * 2 + 1];
        }
    }

    // MIDI Event Handlers (compatible with midi-audio-synth.js API)

    handleNoteOn(note, velocity) {
        if (!this.isReady) return;
        this.wasmModule._regroove_synth_note_on(this.synthInstance, note, velocity);
        console.log(`[RGResonate1] Note ON: ${note} vel: ${velocity}`);
    }

    handleNoteOff(note) {
        if (!this.isReady) return;
        this.wasmModule._regroove_synth_note_off(this.synthInstance, note);
        console.log(`[RGResonate1] Note OFF: ${note}`);
    }

    handleControl(cc, value) {
        if (!this.isReady) return;
        this.wasmModule._regroove_synth_control_change(this.synthInstance, cc, value);
    }

    handleBeat(intensity = 1.0) {
        // Optional: trigger a note for beat visualization
        // (Can be customized based on your needs)
    }

    // Parameter Control

    setParameter(index, value) {
        if (!this.isReady) return;
        this.wasmModule._regroove_synth_set_parameter(this.synthInstance, index, value);
    }

    getParameter(index) {
        if (!this.isReady) return 0.0;
        return this.wasmModule._regroove_synth_get_parameter(this.synthInstance, index);
    }

    getParameterName(index) {
        if (!this.isReady) return '';
        const ptr = this.wasmModule._regroove_synth_get_parameter_name(this.synthInstance, index);
        return this.wasmModule.UTF8ToString(ptr);
    }

    getParameterCount() {
        if (!this.isReady) return 0;
        return this.wasmModule._regroove_synth_get_parameter_count(this.synthInstance);
    }

    // Get analyser for Milkdrop (compatible with midi-audio-synth.js)
    getAnalyser() {
        return this.analyser;
    }

    // Cleanup
    destroy() {
        if (this.scriptProcessor) {
            this.scriptProcessor.disconnect();
        }
        if (this.analyser) {
            this.analyser.disconnect();
        }
        if (this.audioBufferPtr) {
            this.wasmModule._free(this.audioBufferPtr);
        }
        if (this.synthInstance) {
            this.wasmModule._regroove_synth_destroy(this.synthInstance);
        }
        this.isReady = false;
        console.log('[RGResonate1] Destroyed');
    }

    // All notes off (panic)
    stopAll() {
        if (!this.isReady) return;
        this.wasmModule._regroove_synth_all_notes_off(this.synthInstance);
    }
}

// Make available globally (for compatibility with existing code)
if (typeof window !== 'undefined') {
    window.RGResonate1Synth = RGResonate1Synth;
}
