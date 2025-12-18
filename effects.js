// Regroove Effects - WASM ONLY VERSION
// NO FALLBACK - Requires compiled WebAssembly

class AudioEffectsProcessor {
    constructor() {
        this.audioContext = null;
        this.sourceNode = null;
        this.analyser = null;
        this.isPlaying = false;
        this.audioBuffer = null;
        this.micStream = null;
        this.selectedMicDeviceId = null;
        this.playbackGain = null;  // For fade out
        
        this.wasmModule = null;
        this.workletNode = null;
    }

    async init() {
        console.log('🚀 Initializing Regroove Effects (WASM ONLY)');
        
        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        this.analyser = this.audioContext.createAnalyser();
        this.analyser.fftSize = 2048;
        this.analyser.connect(this.audioContext.destination);
        
        updateStatus('Loading WebAssembly...', 'fallback');
        
        await this.initWasm();
        
        console.log('✅ WASM READY - Using Real C Code!');
        updateStatus('✅ WASM LOADED - Real C Implementation', 'wasm');
        
        await this.enumerateDevices();
    }

    async initWasm() {
        console.log('📡 Loading WASM files...');
        
        // Load both the JS and WASM files from main thread
        const [jsResponse, wasmResponse] = await Promise.all([
            fetch('regroove-effects.js'),
            fetch('regroove-effects.wasm')
        ]);
        
        if (!jsResponse.ok || !wasmResponse.ok) {
            throw new Error('❌ WASM files not found! Run: cd web && ./build.sh');
        }
        
        console.log('📦 Reading WASM...');
        const jsCode = await jsResponse.text();
        const wasmBytes = await wasmResponse.arrayBuffer();
        
        console.log('🎛️ Registering AudioWorklet...');
        await this.audioContext.audioWorklet.addModule('audio-worklet-processor.js');
        
        console.log('🔧 Creating worklet...');
        this.workletNode = new AudioWorkletNode(this.audioContext, 'wasm-effects-processor');
        
        // Wait for worklet to ask for WASM
        await new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('Worklet timeout')), 10000);
            
            this.workletNode.port.onmessage = (e) => {
                if (e.data.type === 'needWasm') {
                    console.log('📨 Sending WASM to worklet...');
                    this.workletNode.port.postMessage({
                        type: 'wasmBytes',
                        data: wasmBytes
                    }, [wasmBytes]);
                } else if (e.data.type === 'ready') {
                    clearTimeout(timeout);
                    console.log('✅ Worklet ready!');
                    resolve();
                } else if (e.data.type === 'error') {
                    clearTimeout(timeout);
                    reject(new Error(`Worklet: ${e.data.error}`));
                }
            };
        });
        
        this.workletNode.connect(this.analyser);
        
        console.log('🎉 COMPLETE!');
        console.log('🔊 Audio: Source → WASM → Speakers');
    }

    async enumerateDevices() {
        try {
            const devices = await navigator.mediaDevices.enumerateDevices();
            const audioInputs = devices.filter(d => d.kind === 'audioinput');
            
            const selector = document.getElementById('micDeviceList');
            selector.innerHTML = '<option value="">Default Microphone</option>';
            
            audioInputs.forEach(device => {
                const option = document.createElement('option');
                option.value = device.deviceId;
                option.textContent = device.label || `Microphone ${selector.options.length}`;
                selector.appendChild(option);
            });
            
            if (audioInputs.length > 0) {
                document.getElementById('micDeviceSelector').style.display = 'block';
            }
            
            selector.onchange = () => {
                this.selectedMicDeviceId = selector.value || null;
                console.log('🎤 Selected device:', selector.options[selector.selectedIndex].text);
            };
        } catch (error) {
            console.warn('Could not enumerate devices:', error);
        }
    }

    toggleEffect(name, enabled) {
        this.workletNode.port.postMessage({
            type: 'toggle',
            data: { name, enabled }
        });
    }

    setParameter(effectName, paramName, value) {
        this.workletNode.port.postMessage({
            type: 'setParam',
            data: { effect: effectName, param: paramName, value }
        });
    }

    async loadAudioFile(file) {
        console.log(`📂 Loading: ${file.name}`);
        const arrayBuffer = await file.arrayBuffer();
        this.audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
        console.log(`✅ Loaded: ${(this.audioBuffer.duration).toFixed(1)}s @ ${this.audioBuffer.sampleRate}Hz`);
    }

    async startMicrophone() {
        if (this.micStream) {
            this.stopMicrophone();
        }
        
        const constraints = {
            audio: this.selectedMicDeviceId 
                ? { deviceId: { exact: this.selectedMicDeviceId } }
                : true
        };
        
        console.log('🎤 Starting microphone...');
        this.micStream = await navigator.mediaDevices.getUserMedia(constraints);
        const source = this.audioContext.createMediaStreamSource(this.micStream);
        this.sourceNode = source;
        
        console.log('🔗 Mic → WASM');
        this.sourceNode.connect(this.workletNode);
        
        this.isPlaying = true;
        console.log('✅ Microphone active');
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
        console.log('⏹ Microphone stopped');
    }

    play() {
        if (this.sourceNode && this.sourceNode.stop) {
            this.sourceNode.stop();
        }
        
        if (this.audioBuffer) {
            console.log('▶️ Playing...');
            this.sourceNode = this.audioContext.createBufferSource();
            this.sourceNode.buffer = this.audioBuffer;
            
            console.log('🔗 File → WASM');
            this.sourceNode.connect(this.workletNode);
            
            this.sourceNode.start(0);
            this.isPlaying = true;
            
            this.sourceNode.onended = () => {
                this.isPlaying = false;
                updatePlaybackButtons();
                console.log('⏹ Playback ended');
            };
        }
    }

    generateTestSignal(type) {
        console.log(`🔊 Generating ${type} signal...`);
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
        console.log('✅ Test signal ready');
    }

    stop() {
        if (this.sourceNode && this.sourceNode.stop) {
            try {
                // Create a fade-out gain node
                const fadeGain = this.audioContext.createGain();
                const fadeTime = 0.05;
                const now = this.audioContext.currentTime;

                // Reconnect through fade gain
                this.sourceNode.disconnect();
                this.sourceNode.connect(fadeGain);
                fadeGain.connect(this.workletNode);

                // Fade out
                fadeGain.gain.setValueAtTime(1.0, now);
                fadeGain.gain.linearRampToValueAtTime(0, now + fadeTime);

                // Stop after fade
                setTimeout(() => {
                    try {
                        if (this.sourceNode) {
                            this.sourceNode.stop();
                            this.sourceNode.disconnect();
                            this.sourceNode = null;
                        }
                        fadeGain.disconnect();
                    } catch (e) {
                        // Ignore - already stopped
                    }
                }, fadeTime * 1000 + 10);

            } catch (e) {
                // Fallback - just stop immediately
                try {
                    this.sourceNode.stop();
                    this.sourceNode.disconnect();
                    this.sourceNode = null;
                } catch (e2) {
                    // Ignore
                }
            }
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
    { name: 'reverb', title: 'Reverb', params: ['size', 'damping', 'mix'] },
    { name: 'phaser', title: 'Phaser', params: ['rate', 'depth', 'feedback'] }
];

function createEffectUI() {
    const container = document.getElementById('effects');
    
    effectDefinitions.forEach(def => {
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
        toggle.dataset.enabled = 'false';
        toggle.onclick = () => {
            const enabled = toggle.dataset.enabled !== 'true';
            toggle.dataset.enabled = enabled;
            processor.toggleEffect(def.name, enabled);
            toggle.classList.toggle('active', enabled);
            card.classList.toggle('enabled', enabled);
        };
        
        header.appendChild(title);
        header.appendChild(toggle);
        card.appendChild(header);
        
        // Create knobs container for parameters
        const knobsContainer = document.createElement('div');
        knobsContainer.style.cssText = 'display: flex; gap: 10px; margin-top: 10px; flex-wrap: wrap; justify-content: center;';

        def.params.forEach((paramName, idx) => {
            const defaultValue = 50;

            // Use pad-knob component for each parameter
            const knob = document.createElement('pad-knob');
            knob.id = `${def.name}-${paramName}-knob`;
            knob.setAttribute('label', paramName.toUpperCase());
            knob.setAttribute('cc', String(idx + 1));
            knob.setAttribute('value', String(defaultValue));
            knob.setAttribute('min', '0');
            knob.setAttribute('max', '100');
            knob.setAttribute('sublabel', '50%');
            knob.style.cssText = 'width: 100px; height: 140px;';

            // Listen for value changes
            knob.addEventListener('cc-change', (e) => {
                const value = e.detail.value / 100;
                processor.setParameter(def.name, paramName, value);
                knob.setAttribute('sublabel', `${e.detail.value}%`);
            });

            knobsContainer.appendChild(knob);
        });

        card.appendChild(knobsContainer);
        
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
        
        ctx.fillStyle = '#0a0a0a';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        // Grid
        ctx.strokeStyle = '#1a1a1a';
        ctx.lineWidth = 1;
        for (let i = 0; i < 5; i++) {
            const y = (canvas.height / 4) * i;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(canvas.width, y);
            ctx.stroke();
        }
        
        // Waveform
        ctx.lineWidth = 2;
        ctx.strokeStyle = '#CF1A37';
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

function updateStatus(message, type = 'fallback') {
    const statusBar = document.getElementById('status-bar');
    const statusText = document.getElementById('status-text');
    const modeIndicator = document.getElementById('mode-indicator');
    
    statusText.textContent = message;
    statusBar.className = `status-bar ${type}`;
    modeIndicator.textContent = type === 'wasm' ? '[WASM]' : '[LOADING...]';
}

// Event Listeners
document.getElementById('audioFile').addEventListener('change', async (e) => {
    const file = e.target.files[0];
    if (file) {
        try {
            updateStatus(`Loading: ${file.name}...`, 'wasm');
            await processor.loadAudioFile(file);
            updateStatus(`Ready: ${file.name}`, 'wasm');
            updatePlaybackButtons();
        } catch (error) {
            updateStatus(`Error: ${error.message}`, 'fallback');
        }
    }
});

document.getElementById('micBtn').addEventListener('click', async () => {
    try {
        if (processor.micStream) {
            processor.stopMicrophone();
            updateStatus('Microphone stopped', 'wasm');
            document.getElementById('micBtn').textContent = '🎤 Microphone';
        } else {
            await processor.startMicrophone();
            updateStatus('Microphone ACTIVE', 'wasm');
            document.getElementById('micBtn').textContent = '🎤 Stop Mic';
        }
        updatePlaybackButtons();
    } catch (error) {
        updateStatus(`Mic error: ${error.message}`, 'fallback');
    }
});

document.getElementById('playBtn').addEventListener('click', () => {
    processor.play();
    updatePlaybackButtons();
});

document.getElementById('stopBtn').addEventListener('click', () => {
    processor.stop();
    updatePlaybackButtons();
    document.getElementById('micBtn').textContent = '🎤 Microphone';
    updateStatus('Stopped', 'wasm');
});

document.getElementById('testSignal').addEventListener('change', (e) => {
    if (e.target.value) {
        processor.generateTestSignal(e.target.value);
        updateStatus(`Test: ${e.target.value}`, 'wasm');
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
    } catch (error) {
        updateStatus(`❌ ERROR: ${error.message}`, 'fallback');
        console.error('INIT ERROR:', error);
        console.error('Stack:', error.stack);
    }
})();
