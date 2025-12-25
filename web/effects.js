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
        this.masterGain = null;
        this.playbackRate = 1.0; // Tempo control

        this.wasmModule = null;
        this.workletNode = null;

        // Stereo peaks from worklet for VU meter
        this.stereoPeaks = { left: 0, right: 0 };

        // Streaming playback
        this.mediaElementSource = null;
        this.audioElement = null;
        this.isStreaming = false;
    }

    async init() {
        console.log('🚀 Initializing Regroove Effects (WASM ONLY)');

        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();

        // Create master gain node
        this.masterGain = this.audioContext.createGain();
        this.masterGain.gain.value = 1.0; // Unity gain (0 dB)

        // Create analyser
        this.analyser = this.audioContext.createAnalyser();
        this.analyser.fftSize = 2048;

        // Audio graph: worklet → masterGain → analyser → destination
        this.masterGain.connect(this.analyser);
        this.analyser.connect(this.audioContext.destination);

        await this.initWasm();

        console.log('✅ WASM READY - Using Real C Code!');

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
                        data: {
                            jsCode: jsCode,
                            wasmBytes: wasmBytes
                        }
                    }, [wasmBytes]);
                } else if (e.data.type === 'ready') {
                    clearTimeout(timeout);
                    console.log('✅ Worklet ready!');
                    resolve();
                } else if (e.data.type === 'error') {
                    clearTimeout(timeout);
                    reject(new Error(`Worklet: ${e.data.error}`));
                } else if (e.data.type === 'peakLevel') {
                    // Update M1 TRIM LED indicator
                    this.updateTrimLED(e.data.level);
                } else if (e.data.type === 'stereoPeaks') {
                    // Update stereo peaks for VU meter
                    this.stereoPeaks.left = e.data.left;
                    this.stereoPeaks.right = e.data.right;
                }
            };
        });
        
        this.workletNode.connect(this.masterGain);

        console.log('🎉 COMPLETE!');
        console.log('🔊 Audio: Source → WASM → Master Gain → Speakers');
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
                // Show mic selector by default (no file loaded yet)
                document.getElementById('audioSourceInfo').style.display = 'block';
                document.getElementById('micDeviceList').style.display = 'block';
                document.getElementById('currentFileName').style.display = 'none';
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

    setMasterGain(value) {
        // value is 0-127 from fader
        // Map to 0-100% linearly (0 = 0%, 127 = 100%)
        const percentage = (value / 127) * 100;
        const gainLinear = value / 127; // 0.0 to 1.0

        this.masterGain.gain.value = gainLinear;

        return percentage;
    }

    setTempo(value) {
        // value is 0-127 from fader
        // 64 = 100% (neutral)
        // 0 = 90% (10% slower)
        // 127 = 110% (10% faster)

        // Map with exact center at 64 = 100%
        let percentage;
        if (value <= 64) {
            // 0-64 maps to 90-100%
            percentage = 90 + (value / 64) * 10;
        } else {
            // 65-127 maps to 100-110%
            percentage = 100 + ((value - 64) / 63) * 10;
        }
        const playbackRate = percentage / 100;

        // Apply tempo to streaming audio
        if (this.audioElement && this.isStreaming) {
            this.audioElement.playbackRate = playbackRate;
        }

        // For buffered audio (BufferSource), we can't change tempo on the fly
        // It would require restarting with a new playback rate
        // We'll just store it for the next time play() is called
        this.playbackRate = playbackRate;

        return percentage;
    }

    updateTrimLED(peakLevel) {
        const led = document.getElementById('trim-drive-led');
        if (!led) return;

        // LED glows based on peak level (matching plugin behavior)
        // Starts glowing at 0.5 (-6dB), full red at 1.0+ (0dB/clipping)
        const threshold = 0.5;
        let glow = (peakLevel - threshold) / (1.0 - threshold);
        glow = Math.max(0, Math.min(glow, 1)); // Clamp 0-1

        // Fill the circle based on glow level
        if (glow > 0.01) {
            const r = Math.round(180 + glow * 75); // 180 -> 255
            led.style.backgroundColor = `rgb(${r}, 0, 0)`;
            const shadowIntensity = 3 + glow * 8;
            led.style.boxShadow = `
                0 0 ${shadowIntensity}px rgba(255, 0, 0, ${glow * 0.8}),
                inset 0 0 3px rgba(255, 255, 255, ${glow * 0.3})
            `;
        } else {
            // Dark/off state
            led.style.backgroundColor = '#300';
            led.style.boxShadow = 'inset 0 1px 2px rgba(0,0,0,0.5)';
        }
    }

    async loadAudioFile(file) {
        console.log(`📂 Loading: ${file.name} (${(file.size / 1024 / 1024).toFixed(2)} MB)`);

        this.cleanupAudioElement();

        this.audioElement = new Audio();
        this.audioElement.loop = true;

        // Try blob URL first, fallback to data URL for immutable systems
        try {
            this.audioElement.src = URL.createObjectURL(file);

            await new Promise((resolve, reject) => {
                const timeout = setTimeout(() => reject(new Error('Blob URL timeout')), 2000);
                this.audioElement.oncanplay = () => {
                    clearTimeout(timeout);
                    resolve();
                };
                this.audioElement.onerror = () => {
                    clearTimeout(timeout);
                    reject(new Error('Blob URL failed'));
                };
                this.audioElement.load();
            });

            console.log(`✅ Loaded via blob URL: ${file.name}`);
        } catch (error) {
            console.warn('Blob URL failed, using data URL fallback:', error.message);

            // Fallback: Read file as ArrayBuffer and convert to data URL
            const arrayBuffer = await file.arrayBuffer();
            const blob = new Blob([arrayBuffer], { type: file.type || 'audio/wav' });
            const reader = new FileReader();

            const dataUrl = await new Promise((resolve, reject) => {
                reader.onload = () => resolve(reader.result);
                reader.onerror = reject;
                reader.readAsDataURL(blob);
            });

            this.audioElement.src = dataUrl;

            await new Promise((resolve, reject) => {
                this.audioElement.oncanplay = resolve;
                this.audioElement.onerror = reject;
                this.audioElement.load();
            });

            console.log(`✅ Loaded via data URL: ${file.name}`);
        }

        this.isStreaming = true;
        this.loadedFile = file;  // Store for offline rendering

        // Show file name, hide mic selector
        document.getElementById('audioSourceInfo').style.display = 'block';
        document.getElementById('micDeviceList').style.display = 'none';
        document.getElementById('currentFileName').style.display = 'block';
        document.getElementById('fileNameText').textContent = file.name;

        // Show RENDER button, hide TEST SIGNAL selector
        document.getElementById('renderBtn').style.display = 'inline-block';
        document.getElementById('testSignal').style.display = 'none';

        // Update page title
        document.title = `RFX: ${file.name}`;
    }
    
    cleanupAudioElement() {
        if (this.mediaElementSource) {
            this.mediaElementSource.disconnect();
            this.mediaElementSource = null;
        }
        if (this.audioElement) {
            this.audioElement.pause();
            if (this.audioElement.src) {
                URL.revokeObjectURL(this.audioElement.src);
            }
            this.audioElement = null;
        }
        this.isStreaming = false;
        this.loadedFile = null;

        // Hide RENDER button, show TEST SIGNAL selector
        document.getElementById('renderBtn').style.display = 'none';
        document.getElementById('testSignal').style.display = 'inline-block';

        // Hide file name, show mic selector (if available)
        const micDeviceList = document.getElementById('micDeviceList');
        if (micDeviceList && micDeviceList.options.length > 0) {
            document.getElementById('audioSourceInfo').style.display = 'block';
            document.getElementById('micDeviceList').style.display = 'block';
            document.getElementById('currentFileName').style.display = 'none';
        } else {
            document.getElementById('audioSourceInfo').style.display = 'none';
        }

        // Reset page title
        document.title = 'Regroove Effects Tester';
    }

    async startMicrophone() {
        // Stop any playing audio first
        if (this.sourceNode && this.sourceNode.stop) {
            try {
                this.sourceNode.stop();
                this.sourceNode.disconnect();
            } catch (e) {
                // Already stopped
            }
            this.sourceNode = null;
        }

        // Stop any existing microphone
        if (this.micStream) {
            this.stopMicrophone();
        }

        // CRITICAL: Resume AudioContext (browsers suspend it until user interaction)
        if (this.audioContext.state !== 'running') {
            console.log(`⚠️ AudioContext state: ${this.audioContext.state}`);
            await this.audioContext.resume();
            console.log(`✅ AudioContext resumed: ${this.audioContext.state}`);
        }

        // Request stereo audio input
        const constraints = {
            audio: {
                deviceId: this.selectedMicDeviceId ? { exact: this.selectedMicDeviceId } : undefined,
                channelCount: 2,  // Request stereo
                echoCancellation: false,
                noiseSuppression: false,
                autoGainControl: false
            }
        };

        console.log('🎤 Starting microphone...');
        this.micStream = await navigator.mediaDevices.getUserMedia(constraints);

        // Log actual track settings
        const track = this.micStream.getAudioTracks()[0];
        const settings = track.getSettings();
        console.log(`   Track settings: ${settings.channelCount} channels @ ${settings.sampleRate}Hz`);

        const source = this.audioContext.createMediaStreamSource(this.micStream);

        // CRITICAL: Preserve stereo channels
        source.channelCount = 2;
        source.channelCountMode = 'explicit';
        source.channelInterpretation = 'speakers';

        this.sourceNode = source;

        console.log('🔗 Mic → WASM');
        console.log(`   Source channels: ${source.channelCount}`);
        this.sourceNode.connect(this.workletNode);

        this.isPlaying = true;
        console.log('✅ Microphone active');
        console.log(`   AudioContext: ${this.audioContext.state}`);
        console.log(`   Sample rate: ${this.audioContext.sampleRate}Hz`);
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

        // Reset Drive LED when microphone stops
        this.updateTrimLED(0);
    }

    pause() {
        if (!this.isPlaying) return;

        if (this.isStreaming && this.audioElement) {
            // Pause streaming audio
            this.audioElement.pause();
            this.isPlaying = false;
            console.log('⏸ Paused (streaming)');
        } else if (this.sourceNode) {
            // For buffer sources, we can't truly pause - just suspend the context
            this.audioContext.suspend();
            this.isPlaying = false;
            console.log('⏸ Paused (buffer)');
        }
    }

    async resume() {
        if (this.isPlaying) return;

        if (this.isStreaming && this.audioElement) {
            // Resume streaming audio
            await this.audioElement.play();
            this.isPlaying = true;
            console.log('▶️ Resumed (streaming)');
        } else if (this.audioBuffer) {
            // Resume buffer playback
            await this.audioContext.resume();
            this.isPlaying = true;
            console.log('▶️ Resumed (buffer)');
        }
    }

    async play() {
        // Stop microphone if active
        if (this.micStream) {
            this.stopMicrophone();
        }

        // Stop any existing playback
        if (this.sourceNode && this.sourceNode.stop) {
            try {
                this.sourceNode.stop();
                this.sourceNode.disconnect();
            } catch (e) {
                // Already stopped
            }
        }

        // Resume AudioContext if suspended
        if (this.audioContext.state !== 'running') {
            await this.audioContext.resume();
            console.log('✅ AudioContext resumed');
        }

        if (this.isStreaming && this.audioElement) {
            console.log('▶️ Streaming playback...');

            if (!this.mediaElementSource) {
                this.mediaElementSource = this.audioContext.createMediaElementSource(this.audioElement);
                console.log('🔗 Stream → WASM → Speakers');
                this.mediaElementSource.connect(this.workletNode);
            }

            // Apply tempo (playback rate)
            this.audioElement.playbackRate = this.playbackRate;

            await this.audioElement.play();
            this.isPlaying = true;
            console.log(`✅ Streaming at ${(this.playbackRate * 100).toFixed(1)}% tempo`);
            
        } else if (this.audioBuffer) {
            console.log('▶️ Playing (looped)...');
            console.log(`   Buffer: ${this.audioBuffer.duration.toFixed(1)}s, ${this.audioBuffer.numberOfChannels}ch`);
            console.log(`   AudioContext state: ${this.audioContext.state}`);
            console.log(`   WorkletNode: ${this.workletNode ? 'READY' : 'MISSING!'}`);
            
            this.sourceNode = this.audioContext.createBufferSource();
            this.sourceNode.buffer = this.audioBuffer;
            this.sourceNode.loop = true;
            this.sourceNode.playbackRate.value = this.playbackRate; // Apply tempo

            console.log('🔗 Audio graph: BufferSource → WorkletNode → Analyser → Speakers');
            console.log(`   Worklet connected to: ${this.workletNode.numberOfOutputs} outputs`);
            this.sourceNode.connect(this.workletNode);

            this.sourceNode.start(0);
            this.isPlaying = true;
            this.startTime = this.audioContext.currentTime;
            console.log(`✅ Playback started at ${this.startTime.toFixed(3)}s, ${(this.playbackRate * 100).toFixed(1)}% tempo`);

            this.sourceNode.onended = () => {
                this.isPlaying = false;
                updatePlaybackButtons();
                console.log('⏹ Playback ended');
            };
        }
    }

    generateTestSignal(type) {
        console.log(`🔊 Generating ${type} signal...`);

        // Clean up any loaded audio file first
        this.cleanupAudioElement();

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
                const logRatio = Math.log(endFreq / startFreq);

                // Exponential sweep with continuous phase
                for (let i = 0; i < data.length; i++) {
                    const t = i / sampleRate;
                    const progress = t / duration;

                    // Phase accumulation for exponential sweep (integral of frequency)
                    // phase(t) = 2π * f0 * T / ln(f1/f0) * [(f1/f0)^(t/T) - 1]
                    const phase = 2 * Math.PI * startFreq * duration / logRatio *
                                  (Math.exp(logRatio * progress) - 1);

                    data[i] = Math.sin(phase) * 0.3;
                }
            }
        }
        
        this.audioBuffer = buffer;
        console.log('✅ Test signal ready');
    }

    stop() {
        // Clean up audio file/stream
        this.cleanupAudioElement();
        
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

        // Clear audio buffer (for test signals)
        this.audioBuffer = null;

        // Reset Drive LED when playback stops
        this.updateTrimLED(0);
    }
    
    getCurrentPosition() {
        if (this.audioElement && this.isStreaming) {
            return {
                current: this.audioElement.currentTime,
                duration: this.audioElement.duration || 0
            };
        }
        if (this.audioBuffer && this.isPlaying && this.startTime !== undefined) {
            const elapsed = this.audioContext.currentTime - this.startTime;
            const position = elapsed % this.audioBuffer.duration;
            return {
                current: position,
                duration: this.audioBuffer.duration
            };
        }
        return { current: 0, duration: 0 };
    }

    seek(time) {
        // Only streaming audio (audioElement) supports seeking
        if (this.audioElement && this.isStreaming) {
            this.audioElement.currentTime = time;
            console.log(`⏩ Seeked to ${time.toFixed(1)}s`);
        } else if (this.audioBuffer && this.isPlaying) {
            // For buffered audio, we need to restart from the new position
            const wasPlaying = this.isPlaying;
            this.stop();
            if (wasPlaying) {
                this.sourceNode = this.audioContext.createBufferSource();
                this.sourceNode.buffer = this.audioBuffer;
                this.sourceNode.loop = true;
                this.sourceNode.playbackRate.value = this.playbackRate; // Apply tempo
                this.sourceNode.connect(this.workletNode);
                this.sourceNode.start(0, time % this.audioBuffer.duration);
                this.isPlaying = true;
                this.startTime = this.audioContext.currentTime - time;
                console.log(`⏩ Seeked to ${time.toFixed(1)}s (restarted buffer)`);
            }
        }
    }

    getAnalyserData() {
        const bufferLength = this.analyser.frequencyBinCount;
        const dataArray = new Uint8Array(bufferLength);
        this.analyser.getByteTimeDomainData(dataArray);
        return dataArray;
    }

    async renderToWav() {
        if (!this.loadedFile) {
            console.error('❌ No file loaded to render!');
            return;
        }

        try {
            console.log('🎬 Starting offline render...');

            // Decode the audio file
            const arrayBuffer = await this.loadedFile.arrayBuffer();
            const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);

            const sampleRate = audioBuffer.sampleRate;
            const duration = audioBuffer.duration;
            const channels = audioBuffer.numberOfChannels;

            console.log(`📊 Input: ${duration.toFixed(2)}s, ${sampleRate}Hz, ${channels}ch`);

            // Create offline context
            const offlineContext = new OfflineAudioContext(channels, audioBuffer.length, sampleRate);

            // Load WASM into offline context
            console.log('📡 Loading worklet into offline context...');
            try {
                // Fetch the worklet code and create a blob URL for offline context
                const workletResponse = await fetch('audio-worklet-processor.js');
                const workletCode = await workletResponse.text();
                const workletBlob = new Blob([workletCode], { type: 'application/javascript' });
                const workletURL = URL.createObjectURL(workletBlob);

                await offlineContext.audioWorklet.addModule(workletURL);
                URL.revokeObjectURL(workletURL);
            } catch (err) {
                console.error('❌ Failed to load worklet module:', err);
                throw new Error('Failed to load audio worklet: ' + err.message);
            }

            // Create worklet node
            const offlineWorklet = new AudioWorkletNode(offlineContext, 'wasm-effects-processor');

            // Send WASM to offline worklet
            const [jsResponse, wasmResponse] = await Promise.all([
                fetch('regroove-effects.js'),
                fetch('regroove-effects.wasm')
            ]);
            const jsCode = await jsResponse.text();
            const wasmBytes = await wasmResponse.arrayBuffer();

            await new Promise((resolve) => {
                offlineWorklet.port.onmessage = (e) => {
                    if (e.data.type === 'needWasm') {
                        offlineWorklet.port.postMessage({
                            type: 'wasmBytes',
                            data: {
                                jsCode: jsCode,
                                wasmBytes: wasmBytes
                            }
                        }, [wasmBytes]);
                    } else if (e.data.type === 'ready') {
                        resolve();
                    }
                };
            });

            // Copy all effect parameters from live worklet to offline worklet
            const state = await new Promise((resolve) => {
                const handler = (e) => {
                    if (e.data.type === 'state') {
                        this.workletNode.port.removeEventListener('message', handler);
                        console.log('📊 Got state:', e.data.state);
                        resolve(e.data.state);
                    }
                };
                this.workletNode.port.addEventListener('message', handler);
                this.workletNode.port.postMessage({ type: 'getState' });
            });

            console.log('📤 Sending state to offline worklet...');

            // Wait for confirmation that state was applied
            await new Promise((resolve) => {
                const handler = (e) => {
                    if (e.data.type === 'stateApplied') {
                        offlineWorklet.port.removeEventListener('message', handler);
                        console.log('✅ Offline worklet state applied');
                        resolve();
                    }
                };
                offlineWorklet.port.addEventListener('message', handler);
                offlineWorklet.port.postMessage({ type: 'setState', state });
            });

            // Create source and connect
            const source = offlineContext.createBufferSource();
            source.buffer = audioBuffer;
            source.playbackRate.value = this.playbackRate; // Apply tempo
            source.connect(offlineWorklet);
            offlineWorklet.connect(offlineContext.destination);

            // Render
            source.start();
            console.log(`🎵 Rendering at ${(this.playbackRate * 100).toFixed(1)}% tempo`);
            const renderedBuffer = await offlineContext.startRendering();

            console.log('✅ Rendering complete!');

            // Convert to WAV
            const wavBlob = this.audioBufferToWav(renderedBuffer);

            // Download
            const fileName = this.loadedFile.name.replace(/\.[^.]+$/, '_processed.wav');
            const url = URL.createObjectURL(wavBlob);
            const a = document.createElement('a');
            a.href = url;
            a.download = fileName;
            a.click();
            URL.revokeObjectURL(url);

            console.log(`💾 Downloaded: ${fileName}`);

        } catch (error) {
            console.error('❌ Render error:', error);
        }
    }

    audioBufferToWav(buffer) {
        const numChannels = buffer.numberOfChannels;
        const sampleRate = buffer.sampleRate;
        const format = 1; // PCM
        const bitDepth = 16;

        const bytesPerSample = bitDepth / 8;
        const blockAlign = numChannels * bytesPerSample;

        const data = [];
        for (let i = 0; i < buffer.length; i++) {
            for (let channel = 0; channel < numChannels; channel++) {
                let sample = buffer.getChannelData(channel)[i];
                // Clamp
                sample = Math.max(-1, Math.min(1, sample));
                // Convert to 16-bit PCM
                sample = sample < 0 ? sample * 0x8000 : sample * 0x7FFF;
                data.push(sample);
            }
        }

        const dataLength = data.length * bytesPerSample;
        const buffer_array = new ArrayBuffer(44 + dataLength);
        const view = new DataView(buffer_array);

        // Write WAV header
        const writeString = (offset, string) => {
            for (let i = 0; i < string.length; i++) {
                view.setUint8(offset + i, string.charCodeAt(i));
            }
        };

        writeString(0, 'RIFF');
        view.setUint32(4, 36 + dataLength, true);
        writeString(8, 'WAVE');
        writeString(12, 'fmt ');
        view.setUint32(16, 16, true); // fmt chunk size
        view.setUint16(20, format, true);
        view.setUint16(22, numChannels, true);
        view.setUint32(24, sampleRate, true);
        view.setUint32(28, sampleRate * blockAlign, true); // byte rate
        view.setUint16(32, blockAlign, true);
        view.setUint16(34, bitDepth, true);
        writeString(36, 'data');
        view.setUint32(40, dataLength, true);

        // Write audio data
        let offset = 44;
        for (let i = 0; i < data.length; i++) {
            view.setInt16(offset, data[i], true);
            offset += 2;
        }

        return new Blob([buffer_array], { type: 'audio/wav' });
    }
}

// UI Controller
const processor = new AudioEffectsProcessor();
let visualizerAnimationId = null;

// VU Meter state (peak hold with decay)
let vuLeftPeak = 0;
let vuRightPeak = 0;
let vuLeftSmoothed = 0;
let vuRightSmoothed = 0;
const VU_DECAY_RATE = 0.95;  // Slower decay (more dampening)
const VU_SMOOTHING = 0.7;  // Heavy smoothing on incoming values (0.0 = no smoothing, 1.0 = max smoothing)

// MODEL 1 input effects (all enabled by default)
// Display order: TRIM → HPF → SCULPT → LPF
const model1EffectDefinitions = [
    { name: 'model1_trim', title: 'Trim', params: ['drive'], enabledByDefault: true },
    { name: 'model1_hpf', title: 'Contour (HPF)', params: ['cutoff'], enabledByDefault: true },
    { name: 'model1_sculpt', title: 'Sculpt (Cut/Boost)', params: ['frequency', 'gain'], enabledByDefault: true },
    { name: 'model1_lpf', title: 'Contour (LPF)', params: ['cutoff'], enabledByDefault: true }
];

// Standard effects (with on/off toggle)
const effectDefinitions = [
    { name: 'distortion', title: 'Distortion', params: ['drive', 'mix'] },
    { name: 'limiter', title: 'Limiter', params: ['threshold', 'release', 'ceiling', 'lookahead'] },
    { name: 'filter', title: 'Filter', params: ['cutoff', 'resonance'] },
    { name: 'eq', title: 'EQ', params: ['low', 'mid', 'high'] },
    { name: 'compressor', title: 'Compressor', params: ['threshold', 'ratio', 'attack', 'release', 'makeup'] },
    { name: 'delay', title: 'Delay', params: ['time', 'feedback', 'mix'] },
    { name: 'reverb', title: 'Reverb', params: ['size', 'damping', 'mix'] },
    { name: 'phaser', title: 'Phaser', params: ['rate', 'depth', 'feedback'] },
    { name: 'stereo_widen', title: 'Stereo Widening', params: ['width', 'mix'] },
    { name: 'ring_mod', title: 'Ring Modulator', params: ['frequency', 'mix'] },
    { name: 'pitchshift', title: 'Pitch Shift', params: ['pitch', 'mix'] }
];

function createModel1UI() {
    const container = document.getElementById('model1-effects');
    if (!container) return;

    model1EffectDefinitions.forEach(def => {
        const card = document.createElement('div');
        card.className = 'effect-card';
        card.id = `effect-${def.name}`;

        const header = document.createElement('div');
        header.className = 'effect-header';

        const title = document.createElement('div');
        title.className = 'effect-title';
        title.textContent = def.title;

        // Add toggle switch for all MODEL 1 effects
        const toggle = document.createElement('div');
        const enabledByDefault = def.enabledByDefault || false;
        toggle.className = enabledByDefault ? 'toggle-switch active' : 'toggle-switch';
        toggle.dataset.enabled = enabledByDefault ? 'true' : 'false';
        toggle.onclick = () => {
            const enabled = toggle.dataset.enabled !== 'true';
            toggle.dataset.enabled = enabled;
            processor.toggleEffect(def.name, enabled);
            toggle.classList.toggle('active', enabled);
            card.classList.toggle('enabled', enabled);

            // CRITICAL: Re-send current parameter values when enabling
            if (enabled) {
                def.params.forEach(paramName => {
                    const knob = document.getElementById(`${def.name}-${paramName}-knob`);
                    if (knob) {
                        const value = parseFloat(knob.getAttribute('value')) / 100;
                        processor.setParameter(def.name, paramName, value);
                        console.log(`[Toggle] Restoring ${def.name}.${paramName} = ${value}`);
                    }
                });
            }
        };
        header.appendChild(toggle);
        if (enabledByDefault) {
            card.classList.add('enabled');
        }

        header.appendChild(title);
        card.appendChild(header);

        // Create knobs container for parameters
        const knobsContainer = document.createElement('div');
        knobsContainer.style.cssText = 'display: flex; gap: 10px; margin-top: 10px; flex-wrap: wrap; justify-content: center;';

        def.params.forEach((paramName, idx) => {
            // Set defaults based on effect and parameter
            let defaultValue = 50;
            if (def.name === 'model1_trim' && paramName === 'drive') {
                defaultValue = 70; // 0.7 = neutral, no drive
            } else if (def.name === 'model1_hpf' && paramName === 'cutoff') {
                defaultValue = 0; // FLAT (20Hz)
            } else if (def.name === 'model1_lpf' && paramName === 'cutoff') {
                defaultValue = 100; // FLAT (20kHz)
            } else if (def.name === 'model1_sculpt') {
                if (paramName === 'frequency') {
                    defaultValue = 50; // Mid frequency
                } else if (paramName === 'gain') {
                    defaultValue = 50; // 0dB (neutral)
                }
            }

            // Container for knob + LED
            const knobWrapper = document.createElement('div');
            const isTrim = def.name === 'model1_trim' && paramName === 'drive';
            knobWrapper.style.cssText = isTrim
                ? 'display: flex; flex-direction: row; align-items: center; gap: 20px;'
                : 'display: flex; flex-direction: column; align-items: center;';

            // Use pad-knob component for each parameter
            const knob = document.createElement('pad-knob');
            knob.id = `${def.name}-${paramName}-knob`;
            // For TRIM effect, label it as "TRIM" not "DRIVE"
            knob.setAttribute('label', isTrim ? 'TRIM' : paramName.toUpperCase());
            knob.setAttribute('cc', String(idx + 1));
            knob.setAttribute('value', String(defaultValue));
            knob.setAttribute('min', '0');
            knob.setAttribute('max', '100');
            knob.style.cssText = 'width: 100px; height: 140px;';

            // Listen for value changes
            knob.addEventListener('cc-change', (e) => {
                const value = e.detail.value / 100;
                processor.setParameter(def.name, paramName, value);
            });

            knobWrapper.appendChild(knob);

            // Add LED indicator for TRIM drive
            if (isTrim) {
                const ledContainer = document.createElement('div');
                ledContainer.style.cssText = 'display: flex; flex-direction: column; align-items: center; gap: 5px;';

                const led = document.createElement('div');
                led.id = 'trim-drive-led';
                led.style.cssText = `
                    width: 18px;
                    height: 18px;
                    border-radius: 50%;
                    background-color: #300;
                    border: 2px solid #666;
                    box-shadow: inset 0 1px 2px rgba(0,0,0,0.5);
                    transition: background-color 0.05s, box-shadow 0.05s;
                `;

                const ledLabel = document.createElement('div');
                ledLabel.textContent = 'DRIVE';
                ledLabel.style.cssText = 'color: #aaa; font-size: 11px; font-weight: bold; margin-top: 2px;';

                ledContainer.appendChild(led);
                ledContainer.appendChild(ledLabel);
                knobWrapper.appendChild(ledContainer);

                // Store LED reference for later updates
                window.trimDriveLED = led;
            }

            knobsContainer.appendChild(knobWrapper);
        });

        card.appendChild(knobsContainer);
        container.appendChild(card);
    });

    // Set default values for Model 1 effects
    processor.setParameter('model1_trim', 'drive', 0.7);
    processor.setParameter('model1_hpf', 'cutoff', 0.0);  // FLAT (20Hz)
    processor.setParameter('model1_lpf', 'cutoff', 1.0);  // FLAT (20kHz)
    processor.setParameter('model1_sculpt', 'frequency', 0.5);  // Mid frequency
    processor.setParameter('model1_sculpt', 'gain', 0.5);  // 0dB neutral
}

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

            // CRITICAL: Re-send current parameter values when enabling
            if (enabled) {
                def.params.forEach(paramName => {
                    const knob = document.getElementById(`${def.name}-${paramName}-knob`);
                    if (knob) {
                        const value = parseFloat(knob.getAttribute('value')) / 100;
                        processor.setParameter(def.name, paramName, value);
                        console.log(`[Toggle] Restoring ${def.name}.${paramName} = ${value}`);
                    }
                });
            }
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
            knob.style.cssText = 'width: 100px; height: 140px;';

            // Listen for value changes
            knob.addEventListener('cc-change', (e) => {
                const value = e.detail.value / 100;
                processor.setParameter(def.name, paramName, value);
            });

            knobsContainer.appendChild(knob);
        });

        card.appendChild(knobsContainer);
        
        container.appendChild(card);
    });
}

function drawSpectrum() {
    const canvas = document.getElementById('spectrum');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    const bufferLength = processor.analyser.frequencyBinCount;
    const dataArray = new Uint8Array(bufferLength);

    processor.analyser.getByteFrequencyData(dataArray);

    ctx.fillStyle = '#0a0a0a';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    const barWidth = (canvas.width / bufferLength) * 2.5;
    let barHeight;
    let x = 0;

    for (let i = 0; i < bufferLength; i++) {
        barHeight = (dataArray[i] / 255) * canvas.height;

        //const hue = (i / bufferLength) * 20; // Red spectrum
        ctx.fillStyle = `rgb(207, 26, 55)`;

        ctx.fillRect(x, canvas.height - barHeight, barWidth, barHeight);
        x += barWidth + 1;
    }
}

function drawVUMeter() {
    const canvas = document.getElementById('vumeter');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    const width = canvas.width;
    const height = canvas.height;

    // Get stereo peaks from worklet (proper stereo separation!)
    const leftPeak = processor.stereoPeaks.left;
    const rightPeak = processor.stereoPeaks.right;

    // Convert to dB and map to needle position
    // -20dB at rest (bottom), 0dBFS at max (RED/clipping)
    const peakToDb = (peak) => {
        if (peak < 0.00001) return -100; // Silence
        return 20 * Math.log10(peak);
    };

    const dbToNeedle = (db) => {
        // Map -20dB to 0.0 (bottom), 0dBFS to 1.0 (top/RED)
        return Math.max(0, Math.min(1, (db + 20) / 20));
    };

    const leftDb = peakToDb(leftPeak);
    const rightDb = peakToDb(rightPeak);
    const leftNeedle = dbToNeedle(leftDb);
    const rightNeedle = dbToNeedle(rightDb);

    // Apply exponential smoothing to incoming values first
    vuLeftSmoothed = vuLeftSmoothed * VU_SMOOTHING + leftNeedle * (1 - VU_SMOOTHING);
    vuRightSmoothed = vuRightSmoothed * VU_SMOOTHING + rightNeedle * (1 - VU_SMOOTHING);

    // Peak hold with decay on smoothed values
    vuLeftPeak = Math.max(vuLeftSmoothed, vuLeftPeak * VU_DECAY_RATE);
    vuRightPeak = Math.max(vuRightSmoothed, vuRightPeak * VU_DECAY_RATE);

    // Clear canvas
    ctx.fillStyle = '#0a0a0a';
    ctx.fillRect(0, 0, width, height);

    // Dimensions
    const pivotY = height / 2;  // Center vertically
    const arcRadius = Math.min(width * 0.45, height * 0.6);  // Bigger arc for taller meters
    const needleLength = arcRadius * 0.85;  // Needle shorter than arc

    // LEFT METER (pivot on left edge)
    const leftPivotX = width * 0.05;

    // Draw left arc scale - from +90° to -90° counterclockwise (bottom, through right, to top) - INWARD
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(leftPivotX, pivotY, arcRadius, Math.PI / 2, -Math.PI / 2, true);
    ctx.stroke();

    // Scale marks for left
    ctx.lineWidth = 1.5;
    for (let i = 0; i <= 10; i++) {
        const angle = Math.PI / 2 - (Math.PI * i / 10);
        const x1 = leftPivotX + arcRadius * Math.cos(angle);
        const y1 = pivotY + arcRadius * Math.sin(angle);
        const x2 = leftPivotX + (arcRadius - 8) * Math.cos(angle);
        const y2 = pivotY + (arcRadius - 8) * Math.sin(angle);

        // Red zone for top ticks (last 20%)
        ctx.strokeStyle = i >= 8 ? '#CF1A37' : '#666';

        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.stroke();
    }

    // LEFT needle: rests at +90° (bottom), swings toward -90°/270° (top)
    const leftAngle = Math.PI / 2 - vuLeftPeak * Math.PI;
    ctx.save();
    ctx.translate(leftPivotX, pivotY);
    ctx.rotate(leftAngle);

    // Needle shadow
    ctx.strokeStyle = 'rgba(207, 26, 55, 0.3)';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(needleLength, 0);
    ctx.stroke();

    // Needle
    ctx.strokeStyle = '#CF1A37';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(needleLength, 0);
    ctx.stroke();

    // Needle tip
    ctx.fillStyle = '#CF1A37';
    ctx.beginPath();
    ctx.arc(needleLength, 0, 2, 0, Math.PI * 2);
    ctx.fill();

    ctx.restore();

    // Pivot point
    ctx.fillStyle = '#666';
    ctx.beginPath();
    ctx.arc(leftPivotX, pivotY, 6, 0, Math.PI * 2);
    ctx.fill();

    // RIGHT METER (pivot on right edge)
    const rightPivotX = width * 0.95;

    // Draw right arc scale - from +90° to +270° (bottom, through left, to top) - INWARD
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(rightPivotX, pivotY, arcRadius, Math.PI / 2, Math.PI * 3 / 2);
    ctx.stroke();

    // Scale marks for right
    ctx.lineWidth = 1.5;
    for (let i = 0; i <= 10; i++) {
        const angle = Math.PI / 2 + (Math.PI * i / 10);
        const x1 = rightPivotX + arcRadius * Math.cos(angle);
        const y1 = pivotY + arcRadius * Math.sin(angle);
        const x2 = rightPivotX + (arcRadius - 8) * Math.cos(angle);
        const y2 = pivotY + (arcRadius - 8) * Math.sin(angle);

        // Red zone for top ticks (last 20%)
        ctx.strokeStyle = i >= 8 ? '#CF1A37' : '#666';

        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.stroke();
    }

    // RIGHT needle: rests at +90° (bottom), swings toward +270° (top)
    const rightAngle = Math.PI / 2 + vuRightPeak * Math.PI;
    ctx.save();
    ctx.translate(rightPivotX, pivotY);
    ctx.rotate(rightAngle);

    // Needle shadow
    ctx.strokeStyle = 'rgba(207, 26, 55, 0.3)';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(needleLength, 0);
    ctx.stroke();

    // Needle
    ctx.strokeStyle = '#CF1A37';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(needleLength, 0);
    ctx.stroke();

    // Needle tip
    ctx.fillStyle = '#CF1A37';
    ctx.beginPath();
    ctx.arc(needleLength, 0, 2, 0, Math.PI * 2);
    ctx.fill();

    ctx.restore();

    // Pivot point
    ctx.fillStyle = '#666';
    ctx.beginPath();
    ctx.arc(rightPivotX, pivotY, 6, 0, Math.PI * 2);
    ctx.fill();
}

function updatePlaybackPosition() {
    const pos = processor.getCurrentPosition();
    const progressContainer = document.getElementById('playbackProgress');

    if (pos.duration > 0) {
        progressContainer.style.visibility = 'visible';

        const formatTime = (seconds) => {
            const mins = Math.floor(seconds / 60);
            const secs = Math.floor(seconds % 60);
            return `${mins}:${secs.toString().padStart(2, '0')}`;
        };

        document.getElementById('currentTime').textContent = formatTime(pos.current);
        document.getElementById('totalTime').textContent = formatTime(pos.duration);
        document.getElementById('progressBar').style.width = `${(pos.current / pos.duration) * 100}%`;
    } else {
        progressContainer.style.visibility = 'hidden';
    }
}

function drawVisualizer() {
    const canvas = document.getElementById('visualizer');
    const ctx = canvas.getContext('2d');
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;

    // Initialize spectrum canvas
    const spectrumCanvas = document.getElementById('spectrum');
    if (spectrumCanvas) {
        spectrumCanvas.width = spectrumCanvas.offsetWidth;
        spectrumCanvas.height = spectrumCanvas.offsetHeight;
    }

    // Initialize VU meter canvas
    const vuCanvas = document.getElementById('vumeter');
    if (vuCanvas) {
        vuCanvas.width = vuCanvas.offsetWidth;
        vuCanvas.height = vuCanvas.offsetHeight;
    }

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

        // Draw spectrum analyzer
        drawSpectrum();

        // Draw VU meter
        drawVUMeter();
        
        // Update playback position
        updatePlaybackPosition();
    };

    draw();
}

function updatePlaybackButtons() {
    const hasAudio = processor.audioBuffer !== null || processor.isStreaming;
    const playBtn = document.getElementById('playBtn');
    const stopBtn = document.getElementById('stopBtn');

    if (processor.isPlaying) {
        playBtn.textContent = '⏸ Pause';
        playBtn.disabled = false;
    } else {
        playBtn.textContent = '▶ Play';
        playBtn.disabled = !hasAudio;
    }

    stopBtn.disabled = !processor.isPlaying;
}

// Event Listeners
document.getElementById('audioFile').addEventListener('change', async (e) => {
    const file = e.target.files[0];
    if (file) {
        try {
            // Stop microphone if active when loading new audio
            if (processor.micStream) {
                processor.stopMicrophone();
            }
            processor.stop();

            await processor.loadAudioFile(file);

            updatePlaybackButtons();
        } catch (error) {
            console.error('Error loading file:', error);
        }
    }
    // Clear the input value to allow reloading the same file
    e.target.value = '';
});

document.getElementById('micBtn').addEventListener('click', async () => {
    try {
        if (processor.micStream) {
            processor.stopMicrophone();
        } else {
            await processor.startMicrophone();
        }
        updatePlaybackButtons();
    } catch (error) {
        console.error('Mic error:', error);
    }
});

document.getElementById('playBtn').addEventListener('click', async () => {
    if (processor.isPlaying) {
        processor.pause();
    } else {
        // Check if we need to start from beginning or just resume
        const hasBeenStopped = (processor.audioBuffer && !processor.sourceNode) ||
                               (processor.isStreaming && processor.audioElement && processor.audioElement.paused);

        if (hasBeenStopped || (!processor.sourceNode && !processor.audioElement)) {
            await processor.play();
        } else {
            await processor.resume();
        }
    }
    updatePlaybackButtons();
});

document.getElementById('stopBtn').addEventListener('click', () => {
    processor.stop();
    updatePlaybackButtons();
});

document.getElementById('renderBtn').addEventListener('click', async () => {
    await processor.renderToWav();
});

document.getElementById('testSignal').addEventListener('change', (e) => {
    if (e.target.value) {
        // Stop microphone if active (generating test signal is like loading a file)
        if (processor.micStream) {
            processor.stopMicrophone();
        }
        processor.generateTestSignal(e.target.value);

        // Show test signal name
        document.getElementById('audioSourceInfo').style.display = 'block';
        document.getElementById('micDeviceList').style.display = 'none';
        document.getElementById('currentFileName').style.display = 'block';
        document.getElementById('fileNameText').textContent = `Test: ${e.target.value}`;

        // Update page title
        document.title = `RFX: Test ${e.target.value}`;

        updatePlaybackButtons();
        e.target.value = '';
    }
});

// Drag/click to seek on progress bar
(function() {
    const progressContainer = document.getElementById('playbackProgress');
    const progressBarBg = progressContainer.querySelector('div[style*="background: var(--bg-tertiary)"]');
    let isDragging = false;

    function seekToPosition(clientX) {
        if (!progressBarBg) return;

        const rect = progressBarBg.getBoundingClientRect();
        const x = clientX - rect.left;
        const percentage = Math.max(0, Math.min(1, x / rect.width));

        const pos = processor.getCurrentPosition();
        if (pos.duration > 0) {
            const seekTime = percentage * pos.duration;
            processor.seek(seekTime);
        }
    }

    // Mouse events
    progressContainer.addEventListener('mousedown', (e) => {
        isDragging = true;
        seekToPosition(e.clientX);
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (isDragging) {
            seekToPosition(e.clientX);
        }
    });

    document.addEventListener('mouseup', () => {
        isDragging = false;
    });

    // Touch events
    progressContainer.addEventListener('touchstart', (e) => {
        isDragging = true;
        seekToPosition(e.touches[0].clientX);
        e.preventDefault();
    });

    document.addEventListener('touchmove', (e) => {
        if (isDragging) {
            seekToPosition(e.touches[0].clientX);
        }
    });

    document.addEventListener('touchend', () => {
        isDragging = false;
    });
})();

// Initialize
(async () => {
    try {
        await processor.init();
        createModel1UI();
        createEffectUI();
        drawVisualizer();

        // Setup master gain fader after processor is initialized
        const gainFader = document.getElementById('gainFader');
        const gainValue = document.getElementById('gainValue');

        gainFader.addEventListener('change', (e) => {
            const value = parseFloat(e.target.value);
            const percentage = processor.setMasterGain(value);

            // Update display
            gainValue.textContent = `${percentage.toFixed(0)}%`;
        });

        // Set initial volume value (127 = 100%)
        processor.setMasterGain(127);
        gainValue.textContent = '100%';

        // Setup tempo fader
        const tempoFader = document.getElementById('tempoFader');
        const tempoValue = document.getElementById('tempoValue');

        tempoFader.addEventListener('change', (e) => {
            const value = parseFloat(e.target.value);
            const percentage = processor.setTempo(value);

            // Show as delta from 100%
            const delta = percentage - 100;
            const sign = delta > 0 ? '+' : '';
            tempoValue.textContent = `${sign}${delta.toFixed(1)}%`;
        });

        // Set initial tempo value (64 = 100% neutral)
        processor.setTempo(64);
        tempoValue.textContent = '0.0%';

    } catch (error) {
        console.error('INIT ERROR:', error);
        console.error('Stack:', error.stack);
    }
})();
