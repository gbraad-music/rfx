// Audio Effects Processor using Web Audio API
class AudioEffectsProcessor {
    constructor() {
        this.audioContext = null;
        this.sourceNode = null;
        this.analyser = null;
        this.effects = new Map();
        this.isPlaying = false;
        this.audioBuffer = null;
        this.micStream = null;
        
        this.effectsChain = [];
        this.inputNode = null;
        this.outputNode = null;
    }

    async init() {
        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        this.analyser = this.audioContext.createAnalyser();
        this.analyser.fftSize = 2048;
        
        this.inputNode = this.audioContext.createGain();
        this.outputNode = this.audioContext.createGain();
        
        this.initEffects();
        this.rebuildEffectsChain();
    }

    initEffects() {
        // Distortion
        this.effects.set('distortion', {
            enabled: false,
            node: this.audioContext.createWaveShaper(),
            params: {
                drive: { value: 0.5, min: 0, max: 1 },
                mix: { value: 0.5, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                const amount = effect.params.drive.value * 100;
                const k = amount;
                const samples = 44100;
                const curve = new Float32Array(samples);
                const deg = Math.PI / 180;
                
                for (let i = 0; i < samples; i++) {
                    const x = (i * 2) / samples - 1;
                    curve[i] = ((3 + k) * x * 20 * deg) / (Math.PI + k * Math.abs(x));
                }
                effect.node.curve = curve;
            }
        });

        // Filter (Low-pass)
        this.effects.set('filter', {
            enabled: false,
            node: this.audioContext.createBiquadFilter(),
            params: {
                cutoff: { value: 0.8, min: 0, max: 1 },
                resonance: { value: 0.5, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                effect.node.type = 'lowpass';
                const cutoffHz = 20 + (effect.params.cutoff.value * (20000 - 20));
                effect.node.frequency.value = cutoffHz;
                effect.node.Q.value = effect.params.resonance.value * 30;
            }
        });

        // EQ (3-band)
        const lowBand = this.audioContext.createBiquadFilter();
        lowBand.type = 'lowshelf';
        lowBand.frequency.value = 320;
        
        const midBand = this.audioContext.createBiquadFilter();
        midBand.type = 'peaking';
        midBand.frequency.value = 1000;
        midBand.Q.value = 0.5;
        
        const highBand = this.audioContext.createBiquadFilter();
        highBand.type = 'highshelf';
        highBand.frequency.value = 3200;
        
        lowBand.connect(midBand);
        midBand.connect(highBand);
        
        this.effects.set('eq', {
            enabled: false,
            node: lowBand,
            outputNode: highBand,
            bands: { low: lowBand, mid: midBand, high: highBand },
            params: {
                low: { value: 0.5, min: 0, max: 1 },
                mid: { value: 0.5, min: 0, max: 1 },
                high: { value: 0.5, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                effect.bands.low.gain.value = (effect.params.low.value - 0.5) * 24;
                effect.bands.mid.gain.value = (effect.params.mid.value - 0.5) * 24;
                effect.bands.high.gain.value = (effect.params.high.value - 0.5) * 24;
            }
        });

        // Compressor
        this.effects.set('compressor', {
            enabled: false,
            node: this.audioContext.createDynamicsCompressor(),
            params: {
                threshold: { value: 0.7, min: 0, max: 1 },
                ratio: { value: 0.5, min: 0, max: 1 },
                attack: { value: 0.003, min: 0, max: 1 },
                release: { value: 0.25, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                effect.node.threshold.value = -40 + (effect.params.threshold.value * 40);
                effect.node.ratio.value = 1 + (effect.params.ratio.value * 19);
                effect.node.attack.value = 0.001 + (effect.params.attack.value * 0.999);
                effect.node.release.value = 0.01 + (effect.params.release.value * 0.99);
            }
        });

        // Delay
        const delayNode = this.audioContext.createDelay(2.0);
        const feedbackGain = this.audioContext.createGain();
        const mixGain = this.audioContext.createGain();
        const dryGain = this.audioContext.createGain();
        
        this.effects.set('delay', {
            enabled: false,
            node: dryGain,
            outputNode: mixGain,
            delayNode: delayNode,
            feedbackGain: feedbackGain,
            mixGain: mixGain,
            dryGain: dryGain,
            params: {
                time: { value: 0.375, min: 0, max: 1 },
                feedback: { value: 0.3, min: 0, max: 1 },
                mix: { value: 0.3, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                effect.delayNode.delayTime.value = 0.001 + (effect.params.time.value * 1.999);
                effect.feedbackGain.gain.value = effect.params.feedback.value;
                const wetLevel = effect.params.mix.value;
                effect.mixGain.gain.value = wetLevel;
                effect.dryGain.gain.value = 1 - wetLevel;
            },
            setupFn: (effect) => {
                effect.dryGain.connect(effect.mixGain);
                effect.dryGain.connect(effect.delayNode);
                effect.delayNode.connect(effect.feedbackGain);
                effect.feedbackGain.connect(effect.delayNode);
                effect.delayNode.connect(effect.mixGain);
            }
        });

        // Reverb
        this.effects.set('reverb', {
            enabled: false,
            node: this.audioContext.createConvolver(),
            params: {
                roomSize: { value: 0.5, min: 0, max: 1 },
                mix: { value: 0.3, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                this.createReverbImpulse(effect, effect.params.roomSize.value);
            }
        });

        // Phaser
        const phaserStages = 4;
        const allpassFilters = [];
        for (let i = 0; i < phaserStages; i++) {
            allpassFilters.push(this.audioContext.createBiquadFilter());
            allpassFilters[i].type = 'allpass';
        }
        
        this.effects.set('phaser', {
            enabled: false,
            node: allpassFilters[0],
            outputNode: allpassFilters[phaserStages - 1],
            filters: allpassFilters,
            lfo: null,
            params: {
                rate: { value: 0.5, min: 0, max: 1 },
                depth: { value: 0.5, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                const rate = 0.1 + (effect.params.rate.value * 10);
                const depth = effect.params.depth.value * 1000;
                
                if (!effect.lfo) {
                    effect.lfo = this.audioContext.createOscillator();
                    effect.lfo.type = 'sine';
                    effect.lfo.start();
                }
                
                effect.lfo.frequency.value = rate;
                
                effect.filters.forEach((filter, i) => {
                    const baseFreq = 200 + (i * 200);
                    filter.frequency.value = baseFreq;
                    filter.Q.value = 1;
                });
            },
            setupFn: (effect) => {
                for (let i = 0; i < phaserStages - 1; i++) {
                    effect.filters[i].connect(effect.filters[i + 1]);
                }
            }
        });

        // Stereo Widener
        const splitter = this.audioContext.createChannelSplitter(2);
        const merger = this.audioContext.createChannelMerger(2);
        const leftDelay = this.audioContext.createDelay(0.1);
        const rightDelay = this.audioContext.createDelay(0.1);
        
        this.effects.set('stereo_widen', {
            enabled: false,
            node: splitter,
            outputNode: merger,
            leftDelay: leftDelay,
            rightDelay: rightDelay,
            params: {
                width: { value: 0.5, min: 0, max: 1 }
            },
            updateFn: (effect) => {
                const delayTime = effect.params.width.value * 0.02;
                effect.leftDelay.delayTime.value = delayTime;
                effect.rightDelay.delayTime.value = delayTime;
            },
            setupFn: (effect) => {
                splitter.connect(effect.leftDelay, 0);
                splitter.connect(effect.rightDelay, 1);
                effect.leftDelay.connect(merger, 0, 0);
                effect.rightDelay.connect(merger, 0, 1);
            }
        });

        // Setup special effects
        const delayEffect = this.effects.get('delay');
        if (delayEffect.setupFn) delayEffect.setupFn(delayEffect);
        
        const phaserEffect = this.effects.get('phaser');
        if (phaserEffect.setupFn) phaserEffect.setupFn(phaserEffect);
        
        const stereoEffect = this.effects.get('stereo_widen');
        if (stereoEffect.setupFn) stereoEffect.setupFn(stereoEffect);
    }

    createReverbImpulse(effect, roomSize) {
        const sampleRate = this.audioContext.sampleRate;
        const length = sampleRate * (0.5 + roomSize * 3);
        const impulse = this.audioContext.createBuffer(2, length, sampleRate);
        
        for (let channel = 0; channel < 2; channel++) {
            const channelData = impulse.getChannelData(channel);
            for (let i = 0; i < length; i++) {
                channelData[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / length, 2);
            }
        }
        
        effect.node.buffer = impulse;
    }

    rebuildEffectsChain() {
        this.inputNode.disconnect();
        
        let currentNode = this.inputNode;
        const enabledEffects = Array.from(this.effects.entries())
            .filter(([_, effect]) => effect.enabled);
        
        if (enabledEffects.length === 0) {
            this.inputNode.connect(this.outputNode);
        } else {
            enabledEffects.forEach(([name, effect], index) => {
                currentNode.connect(effect.node);
                currentNode = effect.outputNode || effect.node;
            });
            currentNode.connect(this.outputNode);
        }
        
        this.outputNode.connect(this.analyser);
        this.analyser.connect(this.audioContext.destination);
    }

    toggleEffect(name, enabled) {
        const effect = this.effects.get(name);
        if (effect) {
            effect.enabled = enabled;
            this.rebuildEffectsChain();
        }
    }

    setParameter(effectName, paramName, value) {
        const effect = this.effects.get(effectName);
        if (effect && effect.params[paramName]) {
            effect.params[paramName].value = value;
            if (effect.updateFn) {
                effect.updateFn(effect);
            }
        }
    }

    async loadAudioFile(file) {
        const arrayBuffer = await file.arrayBuffer();
        this.audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
    }

    async startMicrophone() {
        if (this.micStream) {
            this.stopMicrophone();
        }
        
        this.micStream = await navigator.mediaDevices.getUserMedia({ audio: true });
        const source = this.audioContext.createMediaStreamSource(this.micStream);
        this.sourceNode = source;
        this.sourceNode.connect(this.inputNode);
        this.isPlaying = true;
    }

    stopMicrophone() {
        if (this.micStream) {
            this.micStream.getTracks().forEach(track => track.stop());
            this.micStream = null;
        }
        if (this.sourceNode) {
            this.sourceNode.disconnect();
            this.sourceNode = null;
        }
        this.isPlaying = false;
    }

    play() {
        if (this.sourceNode) {
            this.sourceNode.stop();
        }
        
        if (this.audioBuffer) {
            this.sourceNode = this.audioContext.createBufferSource();
            this.sourceNode.buffer = this.audioBuffer;
            this.sourceNode.connect(this.inputNode);
            this.sourceNode.start(0);
            this.isPlaying = true;
            
            this.sourceNode.onended = () => {
                this.isPlaying = false;
                updatePlaybackButtons();
            };
        }
    }

    generateTestSignal(type) {
        const duration = 5;
        const sampleRate = this.audioContext.sampleRate;
        const buffer = this.audioContext.createBuffer(2, duration * sampleRate, sampleRate);
        
        for (let channel = 0; channel < 2; channel++) {
            const data = buffer.getChannelData(channel);
            
            if (type === 'sine') {
                const freq = 440;
                for (let i = 0; i < data.length; i++) {
                    data[i] = Math.sin(2 * Math.PI * freq * i / sampleRate) * 0.5;
                }
            } else if (type === 'noise') {
                for (let i = 0; i < data.length; i++) {
                    data[i] = (Math.random() * 2 - 1) * 0.3;
                }
            } else if (type === 'sweep') {
                const startFreq = 20;
                const endFreq = 20000;
                for (let i = 0; i < data.length; i++) {
                    const t = i / sampleRate;
                    const freq = startFreq * Math.pow(endFreq / startFreq, t / duration);
                    data[i] = Math.sin(2 * Math.PI * freq * t) * 0.3;
                }
            }
        }
        
        this.audioBuffer = buffer;
    }

    stop() {
        if (this.sourceNode) {
            this.sourceNode.stop();
            this.sourceNode.disconnect();
            this.sourceNode = null;
        }
        this.stopMicrophone();
        this.isPlaying = false;
    }

    getAnalyserData() {
        const bufferLength = this.analyser.frequencyBinCount;
        const dataArray = new Uint8Array(bufferLength);
        this.analyser.getByteTimeDomainData(dataArray);
        return dataArray;
    }
}

// UI Controller
const processor = new AudioEffectsProcessor();
let visualizerAnimationId = null;

const effectDefinitions = [
    { name: 'distortion', title: 'Distortion', params: ['drive', 'mix'] },
    { name: 'filter', title: 'Filter', params: ['cutoff', 'resonance'] },
    { name: 'eq', title: 'EQ', params: ['low', 'mid', 'high'] },
    { name: 'compressor', title: 'Compressor', params: ['threshold', 'ratio', 'attack', 'release'] },
    { name: 'delay', title: 'Delay', params: ['time', 'feedback', 'mix'] },
    { name: 'reverb', title: 'Reverb', params: ['roomSize', 'mix'] },
    { name: 'phaser', title: 'Phaser', params: ['rate', 'depth'] },
    { name: 'stereo_widen', title: 'Stereo Widener', params: ['width'] }
];

function createEffectUI() {
    const container = document.getElementById('effects');
    
    effectDefinitions.forEach(def => {
        const effect = processor.effects.get(def.name);
        if (!effect) return;
        
        const card = document.createElement('div');
        card.className = 'effect-card';
        card.id = `effect-${def.name}`;
        
        const header = document.createElement('div');
        header.className = 'effect-header';
        
        const title = document.createElement('div');
        title.className = 'effect-title';
        title.textContent = def.title;
        
        const toggle = document.createElement('div');
        toggle.className = 'toggle-switch';
        toggle.onclick = () => {
            const enabled = !effect.enabled;
            processor.toggleEffect(def.name, enabled);
            toggle.classList.toggle('active', enabled);
            card.classList.toggle('enabled', enabled);
        };
        
        header.appendChild(title);
        header.appendChild(toggle);
        card.appendChild(header);
        
        def.params.forEach(paramName => {
            const param = effect.params[paramName];
            if (!param) return;
            
            const control = document.createElement('div');
            control.className = 'param-control';
            
            const label = document.createElement('div');
            label.className = 'param-label';
            label.innerHTML = `
                <span>${paramName.charAt(0).toUpperCase() + paramName.slice(1)}</span>
                <span class="param-value" id="${def.name}-${paramName}-value">${(param.value * 100).toFixed(0)}%</span>
            `;
            
            const slider = document.createElement('input');
            slider.type = 'range';
            slider.min = '0';
            slider.max = '100';
            slider.value = param.value * 100;
            slider.oninput = (e) => {
                const value = e.target.value / 100;
                processor.setParameter(def.name, paramName, value);
                document.getElementById(`${def.name}-${paramName}-value`).textContent = 
                    `${Math.round(value * 100)}%`;
            };
            
            control.appendChild(label);
            control.appendChild(slider);
            card.appendChild(control);
        });
        
        container.appendChild(card);
    });
}

function drawVisualizer() {
    const canvas = document.getElementById('visualizer');
    const ctx = canvas.getContext('2d');
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
    
    const draw = () => {
        visualizerAnimationId = requestAnimationFrame(draw);
        
        const dataArray = processor.getAnalyserData();
        const bufferLength = dataArray.length;
        
        ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        ctx.lineWidth = 2;
        ctx.strokeStyle = '#00d9ff';
        ctx.beginPath();
        
        const sliceWidth = canvas.width / bufferLength;
        let x = 0;
        
        for (let i = 0; i < bufferLength; i++) {
            const v = dataArray[i] / 128.0;
            const y = v * canvas.height / 2;
            
            if (i === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }
            
            x += sliceWidth;
        }
        
        ctx.lineTo(canvas.width, canvas.height / 2);
        ctx.stroke();
    };
    
    draw();
}

function updatePlaybackButtons() {
    document.getElementById('playBtn').disabled = !processor.audioBuffer || processor.isPlaying;
    document.getElementById('stopBtn').disabled = !processor.isPlaying;
}

function setStatus(message, type = 'info') {
    const status = document.getElementById('status');
    status.textContent = message;
    status.className = `status ${type}`;
}

// Event Listeners
document.getElementById('audioFile').addEventListener('change', async (e) => {
    const file = e.target.files[0];
    if (file) {
        try {
            setStatus('Loading audio file...');
            await processor.loadAudioFile(file);
            setStatus(`Loaded: ${file.name}`);
            updatePlaybackButtons();
        } catch (error) {
            setStatus(`Error loading file: ${error.message}`, 'error');
        }
    }
});

document.getElementById('micBtn').addEventListener('click', async () => {
    try {
        if (processor.micStream) {
            processor.stopMicrophone();
            setStatus('Microphone stopped');
            document.getElementById('micBtn').textContent = '🎤 Use Microphone';
        } else {
            await processor.startMicrophone();
            setStatus('Microphone active');
            document.getElementById('micBtn').textContent = '🎤 Stop Microphone';
        }
        updatePlaybackButtons();
    } catch (error) {
        setStatus(`Microphone error: ${error.message}`, 'error');
    }
});

document.getElementById('playBtn').addEventListener('click', () => {
    processor.play();
    updatePlaybackButtons();
});

document.getElementById('stopBtn').addEventListener('click', () => {
    processor.stop();
    updatePlaybackButtons();
    document.getElementById('micBtn').textContent = '🎤 Use Microphone';
    setStatus('Stopped');
});

document.getElementById('testSignal').addEventListener('change', (e) => {
    if (e.target.value) {
        processor.generateTestSignal(e.target.value);
        setStatus(`Generated ${e.target.value} test signal`);
        updatePlaybackButtons();
        e.target.value = '';
    }
});

// Initialize
(async () => {
    try {
        await processor.init();
        createEffectUI();
        drawVisualizer();
        setStatus('Ready! Load audio or use microphone to test effects');
    } catch (error) {
        setStatus(`Initialization error: ${error.message}`, 'error');
    }
})();
