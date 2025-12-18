// AudioWorklet Processor for WASM effects
// The URL to the WASM file is passed via the constructor

class WasmEffectsProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();
        this.wasmModule = null;
        this.effects = new Map();
        this.audioBufferPtr = null;
        this.bufferSize = 8192;
        this.lastFrame = [0, 0]; // Store last frame to prevent clicks
        this.dcBlockerX = [0, 0]; // Previous input for DC blocker
        this.dcBlockerY = [0, 0]; // Previous output for DC blocker
        this.peakLevelFrameCounter = 0; // Counter for peak level polling

        this.port.onmessage = this.handleMessage.bind(this);

        // Get WASM bytes from the main thread
        this.port.postMessage({ type: 'needWasm' });
    }
    
    handleMessage(event) {
        const { type, data } = event.data;
        
        if (type === 'wasmBytes') {
            this.initWasm(data);  // Just the bytes, not JS code
        } else if (type === 'toggle') {
            this.toggleEffect(data.name, data.enabled);
        } else if (type === 'setParam') {
            this.setParameter(data.effect, data.param, data.value);
        }
    }
    
    async initWasm(wasmBytes) {
        try {
            console.log('[Worklet] Compiling WASM...');
            
            // Compile and instantiate WASM directly
            const wasmModule = await WebAssembly.compile(wasmBytes);
            const instance = await WebAssembly.instantiate(wasmModule, {
                a: {
                    a: (size) => { // _emscripten_resize_heap
                        console.warn('[Worklet] Heap resize requested:', size);
                        return false;
                    }
                }
            });
            
            console.log('[Worklet] WASM instantiated');
            
            // Create module wrapper
            this.wasmModule = {
                ...instance.exports,
                HEAPF32: new Float32Array(instance.exports.b.buffer), // memory.buffer
                _malloc: instance.exports.e,
                _free: instance.exports.h
            };
            
            console.log('[Worklet] WASM ready');
            
            // Allocate audio buffer
            this.audioBufferPtr = this.wasmModule._malloc(this.bufferSize * 2 * 4);
            console.log(`[Worklet] Buffer: 0x${this.audioBufferPtr.toString(16)}`);
            
            // Initialize all effects
            this.initEffects();
            
            this.port.postMessage({ type: 'ready' });
            console.log('[Worklet] ✅ Ready!');
        } catch (error) {
            console.error('[Worklet] ❌ Failed:', error);
            this.port.postMessage({ type: 'error', error: error.message });
        }
    }
    
    initEffects() {
        // Processing order: TRIM → SCULPT → LPF → HPF → other effects
        const effects = [
            { name: 'model1_trim', create: 'Ha', prefix: 'model1_trim' },
            { name: 'model1_sculpt', create: 'eb', prefix: 'model1_sculpt' },
            { name: 'model1_lpf', create: 'Ya', prefix: 'model1_lpf' },
            { name: 'model1_hpf', create: 'Qa', prefix: 'model1_hpf' },
            { name: 'distortion', create: 'd', prefix: 'distortion' },
            { name: 'filter', create: 'p', prefix: 'filter' },
            { name: 'eq', create: 'z', prefix: 'eq' },
            { name: 'compressor', create: 'L', prefix: 'compressor' },
            { name: 'delay', create: 'Z', prefix: 'delay' },
            { name: 'reverb', create: 'ja', prefix: 'reverb' },
            { name: 'phaser', create: 'va', prefix: 'phaser' }
        ];

        // Map for set_enabled functions
        const toggleMap = {
            model1_trim: 'Na',
            model1_hpf: 'Ua',
            model1_lpf: 'ab',
            model1_sculpt: 'ib',
            distortion: 'j', filter: 't', eq: 'D', compressor: 'P',
            delay: 'ba', reverb: 'na', phaser: 'za'
        };

        for (const effect of effects) {
            const createFn = this.wasmModule[effect.create];
            if (createFn) {
                const ptr = createFn();
                // All MODEL 1 effects enabled by default
                const isModel1 = effect.name.startsWith('model1_');
                const defaultEnabled = isModel1;

                this.effects.set(effect.name, {
                    ptr: ptr,
                    name: effect.name,
                    enabled: defaultEnabled
                });

                // Actually enable MODEL 1 effects in WASM
                if (defaultEnabled) {
                    const setEnabledFn = this.wasmModule[toggleMap[effect.name]];
                    if (setEnabledFn) {
                        setEnabledFn(ptr, 1);
                    }
                }

                console.log(`[Worklet] ${effect.name}: 0x${ptr.toString(16)} (${defaultEnabled ? 'enabled' : 'disabled'})`);
            }
        }
    }
    
    toggleEffect(name, enabled) {
        const effect = this.effects.get(name);
        if (!effect || !this.wasmModule) return;

        // Map to mangled names
        const toggleMap = {
            model1_trim: 'Na',
            model1_hpf: 'Ua',
            model1_lpf: 'ab',
            model1_sculpt: 'ib',
            distortion: 'j', filter: 't', eq: 'D', compressor: 'P',
            delay: 'ba', reverb: 'na', phaser: 'za'
        };

        const resetMap = {
            model1_trim: 'Ja',
            model1_hpf: 'Sa',
            model1_lpf: '_a',
            model1_sculpt: 'gb',
            distortion: 'f', filter: 'q', eq: 'A', compressor: 'M',
            delay: '$', reverb: 'la', phaser: 'xa'
        };

        // Reset filter state BEFORE changing enabled state to clear any artifacts
        const resetFn = this.wasmModule[resetMap[name]];
        if (resetFn) {
            resetFn(effect.ptr);
        }

        // Set enabled/disabled state
        const setEnabledFn = this.wasmModule[toggleMap[name]];
        if (setEnabledFn) {
            setEnabledFn(effect.ptr, enabled ? 1 : 0);
            effect.enabled = enabled;
        }

        // Reset DC blocker state when toggling to prevent clicks
        this.dcBlockerX = [0, 0];
        this.dcBlockerY = [0, 0];
    }
    
    setParameter(effectName, paramName, value) {
        const effect = this.effects.get(effectName);
        if (!effect || !this.wasmModule) return;
        
        // Map to mangled function names
        const paramMap = {
            model1_trim: { drive: 'La' },
            model1_hpf: { cutoff: 'Va' },
            model1_lpf: { cutoff: 'bb' },
            model1_sculpt: { frequency: 'jb', gain: 'kb' },
            distortion: { drive: 'k', mix: 'l' },
            filter: { cutoff: 'u', resonance: 'v' },
            eq: { low: 'E', mid: 'F', high: 'G' },
            compressor: { threshold: 'Q', ratio: 'R', attack: 'S', release: 'T' },
            delay: { time: 'ca', feedback: 'da', mix: 'ea' },
            reverb: { size: 'oa', damping: 'pa', mix: 'qa' },
            phaser: { rate: 'Aa', depth: 'Ba', feedback: 'Ca' }
        };
        
        const funcName = paramMap[effectName]?.[paramName];
        const setFn = funcName && this.wasmModule[funcName];
        if (setFn) {
            setFn(effect.ptr, value);
        }
    }
    
    process(inputs, outputs, parameters) {
        if (!this.wasmModule || !this.audioBufferPtr) {
            return true;
        }
        
        const input = inputs[0];
        const output = outputs[0];
        
        if (!input || !output || input.length === 0) {
            return true;
        }
        
        const frames = input[0].length;
        
        // Update heap view
        const heapF32 = new Float32Array(
            this.wasmModule.b.buffer,  // memory
            this.audioBufferPtr, 
            frames * 2
        );
        
        // Interleave input with DC blocker
        const inputL = input[0];
        const inputR = input[1] || input[0];
        
        for (let i = 0; i < frames; i++) {
            heapF32[i * 2] = inputL[i];
            heapF32[i * 2 + 1] = inputR[i];
        }
        
        // Process through enabled effects
        const processMap = {
            model1_trim: 'Ka',
            model1_hpf: 'Ta',
            model1_lpf: '$a',
            model1_sculpt: 'hb',
            distortion: 'i', filter: 's', eq: 'C', compressor: 'O',
            delay: 'aa', reverb: 'ma', phaser: 'ya'
        };
        
        for (const [name, effect] of this.effects) {
            if (effect.enabled) {
                const processFn = this.wasmModule[processMap[name]];
                if (processFn) {
                    processFn(effect.ptr, this.audioBufferPtr, frames, sampleRate);
                }
            }
        }

        // Poll M1 TRIM peak level periodically (every 128 frames ~= 2.7ms at 48kHz for responsive LED)
        this.peakLevelFrameCounter += frames;
        if (this.peakLevelFrameCounter >= 128) {
            this.peakLevelFrameCounter = 0;
            const trimEffect = this.effects.get('model1_trim');
            if (trimEffect && trimEffect.enabled) {
                const getPeakLevelFn = this.wasmModule['Pa']; // fx_model1_trim_get_peak_level
                if (getPeakLevelFn) {
                    const peakLevel = getPeakLevelFn(trimEffect.ptr);
                    this.port.postMessage({ type: 'peakLevel', level: peakLevel });
                }
            }
        }

        // De-interleave output with soft clipping and DC removal
        const outputL = output[0];
        const outputR = output[1] || output[0];

        const alpha = 0.995; // DC blocker coefficient

        for (let i = 0; i < frames; i++) {
            let l = heapF32[i * 2];
            let r = heapF32[i * 2 + 1];

            // Only soft clip if signal is actually clipping (> 1.0)
            if (Math.abs(l) > 1.0) {
                l = Math.sign(l) * Math.tanh(Math.abs(l));
            }
            if (Math.abs(r) > 1.0) {
                r = Math.sign(r) * Math.tanh(Math.abs(r));
            }

            outputL[i] = l;
            outputR[i] = r;
        }
        
        return true;
    }
}

registerProcessor('wasm-effects-processor', WasmEffectsProcessor);
