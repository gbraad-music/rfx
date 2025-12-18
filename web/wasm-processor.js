// Audio Effects Processor using WebAssembly (actual C code)
import RegrooveEffectsModule from './regroove-effects.js';

class WasmAudioProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.module = null;
        this.effects = new Map();
        this.audioBufferPtr = null;
        this.bufferSize = 512;
        this.sampleRate = sampleRate;
        
        this.port.onmessage = this.handleMessage.bind(this);
        this.initWasm();
    }
    
    async initWasm() {
        this.module = await RegrooveEffectsModule();
        this.audioBufferPtr = this.module._malloc(this.bufferSize * 2 * 4); // stereo float32
        
        // Initialize all effects
        this.initEffect('distortion', 'fx_distortion');
        this.initEffect('filter', 'fx_filter');
        this.initEffect('eq', 'fx_eq');
        this.initEffect('compressor', 'fx_compressor');
        this.initEffect('delay', 'fx_delay');
        this.initEffect('reverb', 'fx_reverb');
        this.initEffect('phaser', 'fx_phaser');
        this.initEffect('stereo_widen', 'fx_stereo_widen');
        
        this.port.postMessage({ type: 'ready' });
    }
    
    initEffect(name, prefix) {
        const createFn = this.module[`_${prefix}_create`];
        const instancePtr = createFn();
        
        this.effects.set(name, {
            ptr: instancePtr,
            prefix: prefix,
            enabled: false
        });
    }
    
    handleMessage(event) {
        const { type, effect, param, value } = event.data;
        
        if (type === 'toggle') {
            this.toggleEffect(effect, value);
        } else if (type === 'setParam') {
            this.setParameter(effect, param, value);
        }
    }
    
    toggleEffect(name, enabled) {
        const effect = this.effects.get(name);
        if (!effect) return;
        
        const setEnabledFn = this.module[`_${effect.prefix}_set_enabled`];
        setEnabledFn(effect.ptr, enabled ? 1 : 0);
        effect.enabled = enabled;
    }
    
    setParameter(effectName, paramName, value) {
        const effect = this.effects.get(effectName);
        if (!effect) return;
        
        const setFn = this.module[`_${effect.prefix}_set_${paramName}`];
        if (setFn) {
            setFn(effect.ptr, value);
        }
    }
    
    process(inputs, outputs, parameters) {
        if (!this.module || !inputs[0] || !outputs[0]) {
            return true;
        }
        
        const input = inputs[0];
        const output = outputs[0];
        
        if (input.length === 0) return true;
        
        const frames = input[0].length;
        
        // Interleave input to WASM buffer
        const heapF32 = new Float32Array(this.module.HEAPF32.buffer, this.audioBufferPtr, frames * 2);
        for (let i = 0; i < frames; i++) {
            heapF32[i * 2] = input[0][i] || 0;
            heapF32[i * 2 + 1] = input[1] ? input[1][i] : input[0][i];
        }
        
        // Process through enabled effects
        for (const [name, effect] of this.effects) {
            if (effect.enabled) {
                const processFn = this.module[`_${effect.prefix}_process_f32`];
                processFn(effect.ptr, this.audioBufferPtr, frames, this.sampleRate);
            }
        }
        
        // De-interleave output from WASM buffer
        for (let i = 0; i < frames; i++) {
            output[0][i] = heapF32[i * 2];
            if (output[1]) {
                output[1][i] = heapF32[i * 2 + 1];
            }
        }
        
        return true;
    }
}

registerProcessor('wasm-audio-processor', WasmAudioProcessor);
