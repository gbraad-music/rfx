// RGAHXSynth - WASM-based AHX synthesizer for Regroove visualization
// Wraps the RGAHX WASM synth in an AudioWorklet

class RGAHXSynth {
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

    async initialize() {
        console.log('[RGAHXSynth] 🎛️ Initializing WASM AHX Synth...');
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
            console.log('[RGAHXSynth] Audio graph connected: worklet → masterGain → speakerGain → destination');

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
                    console.log('[RGAHXSynth] ✅ WASM AHX Synth ready');
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
                    console.error('[RGAHXSynth] WASM error:', data);
                }
            };

            this.isActive = true;

            console.log('[RGAHXSynth] Initialized - waiting for WASM...');
            return true;
        } catch (error) {
            console.error('[RGAHXSynth] Failed to initialize:', error);
            return false;
        }
    }

    async loadWasm() {
        try {
            console.log('[RGAHXSynth] Loading WASM...');

            // Fetch both JS glue code and WASM binary
            const [jsResponse, wasmResponse] = await Promise.all([
                fetch('synths/rgahxsynth.js'),
                fetch('synths/rgahxsynth.wasm')
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
                    engine: 1 // RGAHX engine ID
                }
            });

            console.log('[RGAHXSynth] WASM sent to worklet');
        } catch (error) {
            console.error('[RGAHXSynth] Failed to load WASM:', error);
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

    setupBeatSynth() {
        // Simple beat oscillator (could use RG909 later)
        this.beatOscillator = this.audioContext.createOscillator();
        this.beatOscillator.type = 'sine';
        this.beatOscillator.frequency.value = 60; // Deep bass

        this.beatGain = this.audioContext.createGain();
        this.beatGain.gain.value = 0;

        this.beatOscillator.connect(this.beatGain);
        this.beatGain.connect(this.masterGain);

        this.beatOscillatorStopped = false;

        try {
            this.beatOscillator.start();
            console.log('[RGAHXSynth] Beat oscillator started');
        } catch (e) {
            console.warn('[RGAHXSynth] Beat oscillator failed to start:', e.message);
            this.beatOscillatorStopped = true;
        }
    }

    handleBeat(intensity = 1.0) {
        if (!this.beatGain || this.beatOscillatorStopped) return;

        const now = this.audioContext.currentTime;
        this.beatGain.gain.cancelScheduledValues(now);
        this.beatGain.gain.setValueAtTime(0, now);
        this.beatGain.gain.linearRampToValueAtTime(intensity * 0.8, now + 0.01);
        this.beatGain.gain.exponentialRampToValueAtTime(0.001, now + 0.3);
        this.beatGain.gain.linearRampToValueAtTime(0, now + 0.31);
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
            console.warn('[RGAHXSynth] Cannot connect - not initialized');
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
            console.error('[RGAHXSynth] ❌ setAudible called but speakerGain is null!');
            return;
        }

        this.isAudible = enabled;
        console.log(`[RGAHXSynth] setAudible(${enabled})`);

        // Resume AudioContext if needed
        if (enabled && this.audioContext.state === 'suspended') {
            try {
                await this.audioContext.resume();
                console.log('[RGAHXSynth] AudioContext resumed');
            } catch (error) {
                console.error('[RGAHXSynth] Failed to resume AudioContext:', error);
                return;
            }
        }

        // Smooth fade to avoid clicks
        const currentTime = this.audioContext.currentTime;
        this.speakerGain.gain.cancelScheduledValues(currentTime);
        this.speakerGain.gain.setValueAtTime(this.speakerGain.gain.value, currentTime);
        this.speakerGain.gain.linearRampToValueAtTime(enabled ? 1.0 : 0.0, currentTime + 0.05);

        console.log(`[RGAHXSynth] ✅ ${enabled ? 'AUDIBLE' : 'MUTED'}`);
    }

    destroy() {
        console.log('[RGAHXSynth] Destroying...');

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

        console.log('[RGAHXSynth] Destroyed');
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
                console.error(`[RGAHXSynth] Error in ${event} listener:`, error);
            }
        }
    }
}
