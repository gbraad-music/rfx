// RGSIDSynth - WASM-based SID synthesizer for Regroove visualization
// Wraps the RGSID WASM synth in an AudioWorklet

class RGSIDSynth {
    constructor(audioContext) {
        this.audioContext = audioContext;
        this.workletNode = null;
        this.masterGain = null;
        this.speakerGain = null;
        this.frequencyAnalyzer = null;
        this.isActive = false;
        this.isAudible = false;

        // Event emitter (for InputManager integration)
        this.listeners = new Map();

        // WASM state
        this.wasmReady = false;
        this.pendingNotes = [];
    }

    /**
     * Get parameter metadata (LV2-style descriptor)
     */
    static getParameterInfo() {
        return [
            // Voice 1 (0-7)
            { index: 0, name: "Waveform", type: "enum", group: "Voice 1", default: 4,
              options: [
                {value: 0, label: "Off"}, {value: 1, label: "Triangle"},
                {value: 2, label: "Sawtooth"}, {value: 4, label: "Pulse"},
                {value: 8, label: "Noise"}, {value: 3, label: "Tri+Saw"},
                {value: 5, label: "Tri+Pulse"}, {value: 6, label: "Saw+Pulse"}
              ]
            },
            { index: 1, name: "PW", type: "float", min: 0, max: 100, default: 50, unit: "%", group: "Voice 1", scale: "normalized", width: 40 },
            { index: 2, name: "Attack", type: "float", min: 0, max: 100, default: 0, group: "Voice 1", scale: "normalized", width: 35 },
            { index: 3, name: "Decay", type: "float", min: 0, max: 100, default: 50, group: "Voice 1", scale: "normalized", width: 35 },
            { index: 4, name: "Sustain", type: "float", min: 0, max: 100, default: 70, group: "Voice 1", scale: "normalized", width: 35 },
            { index: 5, name: "Release", type: "float", min: 0, max: 100, default: 30, group: "Voice 1", scale: "normalized", width: 35 },
            { index: 6, name: "Ring Mod", type: "boolean", default: false, group: "Voice 1" },
            { index: 7, name: "Sync", type: "boolean", default: false, group: "Voice 1" },

            // Voice 2 (8-15)
            { index: 8, name: "Waveform", type: "enum", group: "Voice 2", default: 2,
              options: [
                {value: 0, label: "Off"}, {value: 1, label: "Triangle"},
                {value: 2, label: "Sawtooth"}, {value: 4, label: "Pulse"},
                {value: 8, label: "Noise"}, {value: 3, label: "Tri+Saw"},
                {value: 5, label: "Tri+Pulse"}, {value: 6, label: "Saw+Pulse"}
              ]
            },
            { index: 9, name: "PW", type: "float", min: 0, max: 100, default: 50, unit: "%", group: "Voice 2", scale: "normalized", width: 40 },
            { index: 10, name: "Attack", type: "float", min: 0, max: 100, default: 0, group: "Voice 2", scale: "normalized", width: 35 },
            { index: 11, name: "Decay", type: "float", min: 0, max: 100, default: 50, group: "Voice 2", scale: "normalized", width: 35 },
            { index: 12, name: "Sustain", type: "float", min: 0, max: 100, default: 70, group: "Voice 2", scale: "normalized", width: 35 },
            { index: 13, name: "Release", type: "float", min: 0, max: 100, default: 30, group: "Voice 2", scale: "normalized", width: 35 },
            { index: 14, name: "Ring Mod", type: "boolean", default: false, group: "Voice 2" },
            { index: 15, name: "Sync", type: "boolean", default: false, group: "Voice 2" },

            // Voice 3 (16-23)
            { index: 16, name: "Waveform", type: "enum", group: "Voice 3", default: 4,
              options: [
                {value: 0, label: "Off"}, {value: 1, label: "Triangle"},
                {value: 2, label: "Sawtooth"}, {value: 4, label: "Pulse"},
                {value: 8, label: "Noise"}, {value: 3, label: "Tri+Saw"},
                {value: 5, label: "Tri+Pulse"}, {value: 6, label: "Saw+Pulse"}
              ]
            },
            { index: 17, name: "PW", type: "float", min: 0, max: 100, default: 50, unit: "%", group: "Voice 3", scale: "normalized", width: 40 },
            { index: 18, name: "Attack", type: "float", min: 0, max: 100, default: 0, group: "Voice 3", scale: "normalized", width: 35 },
            { index: 19, name: "Decay", type: "float", min: 0, max: 100, default: 50, group: "Voice 3", scale: "normalized", width: 35 },
            { index: 20, name: "Sustain", type: "float", min: 0, max: 100, default: 70, group: "Voice 3", scale: "normalized", width: 35 },
            { index: 21, name: "Release", type: "float", min: 0, max: 100, default: 30, group: "Voice 3", scale: "normalized", width: 35 },
            { index: 22, name: "Ring Mod", type: "boolean", default: false, group: "Voice 3" },
            { index: 23, name: "Sync", type: "boolean", default: false, group: "Voice 3" },

            // Filter (24-29)
            { index: 24, name: "Mode", type: "enum", group: "Filter", default: 1,
              options: [
                {value: 0, label: "Off"}, {value: 1, label: "Low Pass"},
                {value: 2, label: "Band Pass"}, {value: 3, label: "High Pass"}
              ]
            },
            { index: 25, name: "Cutoff", type: "float", min: 0, max: 100, default: 50, group: "Filter", scale: "normalized", width: 45, height: 140 },
            { index: 26, name: "Resonance", type: "float", min: 0, max: 100, default: 0, group: "Filter", scale: "normalized", width: 45, height: 140 },
            { index: 27, name: "V1 → Flt", type: "boolean", default: true, group: "Filter" },
            { index: 28, name: "V2 → Flt", type: "boolean", default: false, group: "Filter" },
            { index: 29, name: "V3 → Flt", type: "boolean", default: false, group: "Filter" },

            // Global (30)
            { index: 30, name: "Volume", type: "float", min: 0, max: 100, default: 70, group: "Global", scale: "normalized", width: 50, height: 150 }
        ];
    }

    /**
     * Instance method for parameter info (delegates to static)
     */
    getParameterInfo() {
        return RGSIDSynth.getParameterInfo();
    }

    async initialize() {
        console.log('[RGSIDSynth] 🎛️ Initializing WASM SID Synth...');
        try {
            // Frequency analyzer is now managed externally (shared MIDI analyzer in app.js)
            this.frequencyAnalyzer = null;

            // Master gain
            this.masterGain = this.audioContext.createGain();
            this.masterGain.gain.value = 1.0;

            // Speaker output (can be toggled on/off)
            this.speakerGain = this.audioContext.createGain();
            this.speakerGain.gain.value = 0; // Start muted
            this.speakerGain.connect(this.audioContext.destination);

            // Audio graph: worklet → masterGain → speakerGain → destination
            this.masterGain.connect(this.speakerGain);
            console.log('[RGSIDSynth] Audio graph connected: worklet → masterGain → speakerGain → destination');

            // Load and register AudioWorklet processor (with cache-busting)
            await this.audioContext.audioWorklet.addModule('synths/synth-worklet-processor.js?v=181');

            // Create worklet node
            this.workletNode = new AudioWorkletNode(this.audioContext, 'synth-worklet-processor');
            this.workletNode.connect(this.masterGain);

            // Handle worklet messages
            this.workletNode.port.onmessage = (event) => {
                const { type, data } = event.data;

                if (type === 'needWasm') {
                    this.loadWasm();
                } else if (type === 'ready') {
                    console.log('[RGSIDSynth] ✅ WASM SID Synth ready');
                    this.wasmReady = true;

                    // Process any pending notes
                    for (const note of this.pendingNotes) {
                        if (note.type === 'on') {
                            this.handleNoteOn(note.note, note.velocity);
                        } else {
                            this.handleNoteOff(note.note);
                        }
                    }
                    this.pendingNotes = [];
                } else if (type === 'error') {
                    console.error('[RGSIDSynth] WASM error:', data);
                }
            };

            this.isActive = true;

            console.log('[RGSIDSynth] Initialized - waiting for WASM...');
            return true;
        } catch (error) {
            console.error('[RGSIDSynth] Failed to initialize:', error);
            return false;
        }
    }

    async loadWasm() {
        try {
            console.log('[RGSIDSynth] Loading WASM...');

            // Fetch both JS glue code and WASM binary
            const [jsResponse, wasmResponse] = await Promise.all([
                fetch('synths/rgsidsynth.js'),
                fetch('synths/rgsidsynth.wasm')
            ]);

            const jsCode = await jsResponse.text();
            const wasmBytes = await wasmResponse.arrayBuffer();

            // Send to worklet
            this.workletNode.port.postMessage({
                type: 'wasmBytes',
                data: {
                    jsCode: jsCode,
                    wasmBytes: wasmBytes,
                    sampleRate: this.audioContext.sampleRate,
                    engine: 2 // RGSID engine ID
                }
            });

            console.log('[RGSIDSynth] WASM sent to worklet');
        } catch (error) {
            console.error('[RGSIDSynth] Failed to load WASM:', error);
        }
    }

    handleNoteOn(note, velocity) {
        if (!this.isActive) return;

        if (!this.wasmReady) {
            // Queue note for later
            this.pendingNotes.push({ type: 'on', note, velocity });
            return;
        }

        // Send to worklet
        this.workletNode.port.postMessage({
            type: 'noteOn',
            data: { note, velocity }
        });
    }

    handleNoteOff(note) {
        if (!this.isActive) return;

        if (!this.wasmReady) {
            // Queue note for later
            this.pendingNotes.push({ type: 'off', note });
            return;
        }

        // Send to worklet
        this.workletNode.port.postMessage({
            type: 'noteOff',
            data: { note }
        });
    }

    handleControlChange(controller, value) {
        if (!this.isActive || !this.wasmReady || !this.workletNode) return;

        this.workletNode.port.postMessage({
            type: 'controlChange',
            data: { controller, value }
        });
    }

    setParameter(index, value) {
        if (!this.isActive || !this.wasmReady || !this.workletNode) return;

        this.workletNode.port.postMessage({
            type: 'setParameter',
            data: { index, value }
        });
    }

    stopAll() {
        if (!this.wasmReady || !this.workletNode) return;

        this.workletNode.port.postMessage({
            type: 'allNotesOff'
        });
    }

    /**
     * Connect this synth to a destination node (Web Audio API standard)
     */
    connect(destination) {
        if (!this.masterGain) {
            console.warn('[RGSIDSynth] Cannot connect - not initialized');
            return destination;
        }
        return this.masterGain.connect(destination);
    }

    /**
     * Disconnect this synth from all destinations
     */
    disconnect() {
        if (this.masterGain) {
            this.masterGain.disconnect();
        }
    }

    /**
     * Get analyser node for visualization
     */
    getAnalyser() {
        return this.frequencyAnalyzer ? this.frequencyAnalyzer.getAnalyser() : null;
    }

    async setAudible(enabled) {
        if (!this.speakerGain) {
            console.error('[RGSIDSynth] ❌ setAudible called but speakerGain is null!');
            return;
        }

        this.isAudible = enabled;
        console.log(`[RGSIDSynth] setAudible(${enabled})`);

        // Resume AudioContext if needed
        if (enabled && this.audioContext.state === 'suspended') {
            try {
                await this.audioContext.resume();
                console.log('[RGSIDSynth] AudioContext resumed');
            } catch (error) {
                console.error('[RGSIDSynth] Failed to resume AudioContext:', error);
                return;
            }
        }

        // Smooth fade to avoid clicks
        const currentTime = this.audioContext.currentTime;
        this.speakerGain.gain.cancelScheduledValues(currentTime);
        this.speakerGain.gain.setValueAtTime(this.speakerGain.gain.value, currentTime);
        this.speakerGain.gain.linearRampToValueAtTime(enabled ? 1.0 : 0.0, currentTime + 0.05);

        console.log(`[RGSIDSynth] ✅ ${enabled ? 'AUDIBLE' : 'MUTED'}`);
    }

    destroy() {
        console.log('[RGSIDSynth] Destroying...');

        this.isActive = false;

        // Stop all notes
        this.stopAll();

        // Destroy frequency analyzer
        if (this.frequencyAnalyzer) {
            this.frequencyAnalyzer.destroy();
            this.frequencyAnalyzer = null;
        }

        // Disconnect audio graph
        if (this.workletNode) {
            this.workletNode.disconnect();
            this.workletNode = null;
        }

        if (this.masterGain) {
            this.masterGain.disconnect();
            this.masterGain = null;
        }

        if (this.speakerGain) {
            this.speakerGain.disconnect();
            this.speakerGain = null;
        }

        // Clear event listeners
        this.listeners.clear();

        console.log('[RGSIDSynth] Destroyed');
    }

    // Event emitter interface
    on(event, callback) {
        if (!this.listeners.has(event)) {
            this.listeners.set(event, []);
        }
        this.listeners.get(event).push(callback);
    }

    off(event, callback) {
        if (!this.listeners.has(event)) return;
        const callbacks = this.listeners.get(event);
        const index = callbacks.indexOf(callback);
        if (index > -1) {
            callbacks.splice(index, 1);
        }
    }

    emit(event, data) {
        if (!this.listeners.has(event)) return;
        const callbacks = this.listeners.get(event);
        for (const callback of callbacks) {
            try {
                callback(data);
            } catch (error) {
                console.error(`[RGSIDSynth] Error in ${event} listener:`, error);
            }
        }
    }
}

// Register synth in registry (auto-discovery)
if (typeof SynthRegistry !== 'undefined') {
    SynthRegistry.register({
        id: 'rgsid',
        name: 'RGSID',
        displayName: 'RGSID - Commodore 64 SID Chip',
        description: '3-voice synthesizer with filter, ring modulation, and hard sync',
        engineId: 2,
        class: RGSIDSynth,
        wasmFiles: {
            js: 'synths/rgsidsynth.js',
            wasm: 'synths/rgsidsynth.wasm'
        },
        category: 'synthesizer',
        getParameterInfo: RGSIDSynth.getParameterInfo
    });
}
