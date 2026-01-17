/**
 * Auto-generating Synth UI Web Component
 * Generates UI controls from parameter metadata (like LV2 host UI generation)
 */

class SynthUI extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        this.synth = null;
        this.synthInstance = null;
    }

    connectedCallback() {
        const synthId = this.getAttribute('synth');
        if (!synthId) {
            console.error('[SynthUI] Missing "synth" attribute');
            return;
        }

        console.log(`[SynthUI] Component connected for synth: ${synthId}`);

        this.synth = SynthRegistry.get(synthId);
        if (!this.synth) {
            console.error(`[SynthUI] Synth '${synthId}' not found in registry`);
            console.log('[SynthUI] Available synths:', SynthRegistry.getIds());
            return;
        }

        console.log(`[SynthUI] Found synth descriptor:`, this.synth.name);
        this.render();
    }

    /**
     * Set synth instance (called after synth is initialized)
     */
    setSynthInstance(instance) {
        this.synthInstance = instance;

        // Get parameter metadata
        let params = null;
        if (this.synthInstance && typeof this.synthInstance.getParameterInfo === 'function') {
            params = this.synthInstance.getParameterInfo();
        } else if (this.synth.getParameterInfo) {
            // Fallback: static metadata
            params = this.synth.getParameterInfo();
        }

        if (params) {
            // Render UI
            this.renderControls(params);

            // Auto-initialize all parameters to their default values
            this.initializeParameterDefaults(params);
        }

        // Setup preset loading (if synth supports presets)
        if (this.synthInstance && typeof this.synthInstance.onPresetsReady === 'function') {
            this.synthInstance.onPresetsReady((presetNames) => {
                this.renderPresetSelector(presetNames);
            });

            // Listen for preset changes to update UI
            this.synthInstance.on('presetLoaded', (data) => {
                if (data.parameters && data.parameters.length > 0) {
                    this.updateUIFromParameters(data.parameters);
                }
            });
        }
    }

    /**
     * Initialize all parameters to their default values
     */
    initializeParameterDefaults(params) {
        console.log('[SynthUI] Initializing parameters to defaults...');

        for (const param of params) {
            let value = param.default;

            // Scale normalized parameters
            if (param.scale === 'normalized' && param.max === 100) {
                value = value / 100.0;
            }

            // Send to synth
            this.updateParameter(param.index, value);
        }

        console.log(`[SynthUI] Initialized ${params.length} parameters`);
    }

    render() {
        this.shadowRoot.innerHTML = `
            <style>
                :host {
                    display: block;
                    margin: 20px auto;
                    max-width: 900px;
                    padding: 25px;
                    background: rgba(30, 30, 30, 0.5);
                    border-radius: 8px;
                    border: 1px solid rgba(0, 102, 255, 0.3);
                }

                .synth-header {
                    text-align: center;
                    margin-bottom: 20px;
                    color: #0066FF;
                    font-size: 1.1em;
                    font-weight: bold;
                    text-transform: uppercase;
                    letter-spacing: 1px;
                }

                .synth-description {
                    text-align: center;
                    margin-bottom: 20px;
                    color: #888;
                    font-size: 0.85em;
                }

                .preset-selector {
                    margin-bottom: 25px;
                    padding: 15px;
                    background: rgba(0, 0, 0, 0.3);
                    border-radius: 6px;
                    border: 1px solid rgba(0, 102, 255, 0.2);
                }

                .preset-label {
                    color: #888;
                    font-size: 0.7em;
                    text-transform: uppercase;
                    letter-spacing: 1px;
                    margin-bottom: 8px;
                }

                .preset-select {
                    width: 100%;
                    padding: 10px;
                    background: #1a1a1a;
                    color: #d0d0d0;
                    border: 1px solid #0066FF;
                    border-radius: 4px;
                    font-size: 0.95em;
                    font-weight: bold;
                    cursor: pointer;
                }

                .preset-select:hover {
                    border-color: #0088FF;
                    background: #222;
                }

                .preset-select option {
                    background: #1a1a1a;
                    color: #d0d0d0;
                }

                .param-group {
                    margin-bottom: 25px;
                }

                .group-header {
                    color: #888;
                    font-size: 0.7em;
                    text-transform: uppercase;
                    letter-spacing: 1px;
                    margin-bottom: 15px;
                    border-bottom: 1px solid #333;
                    padding-bottom: 5px;
                }

                .controls-row {
                    display: flex;
                    gap: 15px;
                    justify-content: center;
                    align-items: flex-start;
                    flex-wrap: wrap;
                }

                .control-wrapper {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    gap: 6px;
                }

                .control-label {
                    color: #d0d0d0;
                    font-size: 0.65em;
                    font-weight: bold;
                    text-transform: uppercase;
                    text-align: center;
                    margin-bottom: 4px;
                }

                .control-value {
                    color: #d0d0d0;
                    font-size: 0.75em;
                    font-weight: bold;
                    min-height: 1em;
                }

                select {
                    width: 120px;
                    padding: 6px;
                    background: #1a1a1a;
                    color: #d0d0d0;
                    border: 1px solid #444;
                    border-radius: 4px;
                    font-size: 0.85em;
                }

                input[type="checkbox"] {
                    width: 20px;
                    height: 20px;
                }

                .loading {
                    text-align: center;
                    color: #888;
                    padding: 20px;
                }
            </style>

            <div class="synth-header">${this.synth.displayName} <span style="font-size: 0.5em; color: #00CC66;">✓ AUTO-GENERATED</span></div>
            ${this.synth.description ? `<div class="synth-description">${this.synth.description}</div>` : ''}
            <div class="preset-container"></div>
            <div class="controls-container">
                <div class="loading">Waiting for synth initialization...</div>
            </div>
        `;
    }

    renderControls(params) {
        console.log(`[SynthUI] Rendering ${params.length} parameters`);

        // Group parameters
        const groups = this.groupParameters(params);
        console.log(`[SynthUI] Grouped into ${groups.length} groups:`, groups.map(g => g.name));

        const controlsContainer = this.shadowRoot.querySelector('.controls-container');
        controlsContainer.innerHTML = groups.map(group => this.renderGroup(group)).join('');

        // Bind events
        this.bindEvents();

        console.log('[SynthUI] Controls rendered and events bound');
    }

    renderPresetSelector(presetNames) {
        console.log(`[SynthUI] Rendering preset selector with ${presetNames.length} presets`);

        const presetContainer = this.shadowRoot.querySelector('.preset-container');
        if (!presetContainer) {
            console.warn('[SynthUI] Preset container not found');
            return;
        }

        const options = presetNames.map((name, index) =>
            `<option value="${index}">${index}: ${name}</option>`
        ).join('');

        presetContainer.innerHTML = `
            <div class="preset-selector">
                <div class="preset-label">Target Voice</div>
                <select class="preset-select" id="voiceSelect" style="width: 110px;">
                    <option value="0">Voice 1</option>
                    <option value="1">Voice 2</option>
                    <option value="2">Voice 3</option>
                </select>
                <div class="preset-label" style="margin-left: 10px;">Preset</div>
                <select class="preset-select" id="presetSelect">
                    ${options}
                </select>
            </div>
        `;

        // Bind preset change event
        const presetSelect = this.shadowRoot.getElementById('presetSelect');
        const voiceSelect = this.shadowRoot.getElementById('voiceSelect');

        // Debug: log voice selector changes
        if (voiceSelect) {
            voiceSelect.addEventListener('change', (e) => {
                console.log(`[SynthUI] Voice selector changed to: ${e.target.value}`);
            });
        }

        if (presetSelect) {
            // Only load preset when preset selector changes, using currently selected voice
            presetSelect.addEventListener('change', (e) => {
                const index = parseInt(e.target.value);
                const voice = voiceSelect ? parseInt(voiceSelect.value) : 0;
                console.log(`[SynthUI] Preset changed. Voice selector value: "${voiceSelect.value}", parsed: ${voice}`);
                console.log(`[SynthUI] Loading preset ${index}: ${presetNames[index]} to voice ${voice}`);
                if (this.synthInstance && typeof this.synthInstance.loadPreset === 'function') {
                    this.synthInstance.loadPreset(index, voice);
                }
            });

            // Load first preset by default to Voice 1
            if (presetNames.length > 0) {
                this.synthInstance.loadPreset(0, 0);
            }
        }

        console.log('[SynthUI] Preset selector ready');
    }

    groupParameters(params) {
        const groupMap = new Map();

        for (const param of params) {
            const groupName = param.group || 'Parameters';
            if (!groupMap.has(groupName)) {
                groupMap.set(groupName, []);
            }
            groupMap.get(groupName).push(param);
        }

        return Array.from(groupMap.entries()).map(([name, params]) => ({
            name,
            params
        }));
    }

    renderGroup(group) {
        return `
            <div class="param-group">
                <div class="group-header">${group.name}</div>
                <div class="controls-row">
                    ${group.params.map(p => this.renderControl(p)).join('')}
                </div>
            </div>
        `;
    }

    renderControl(param) {
        switch (param.type) {
            case 'enum':
                return this.renderEnum(param);
            case 'float':
            case 'int':
                return this.renderSlider(param);
            case 'boolean':
                return this.renderCheckbox(param);
            default:
                return '';
        }
    }

    renderEnum(param) {
        const options = param.options.map(opt =>
            `<option value="${opt.value}" ${opt.value === param.default ? 'selected' : ''}>${opt.label}</option>`
        ).join('');

        return `
            <div class="control-wrapper">
                <div class="control-label">${param.name}</div>
                <select data-param-index="${param.index}" data-param-type="enum">
                    ${options}
                </select>
            </div>
        `;
    }

    renderSlider(param) {
        const color = param.color || this.getColorForParam(param);
        const width = param.width || 40;
        const height = param.height || 120;
        const displayValue = this.formatValue(param.default, param);

        return `
            <div class="control-wrapper">
                <div class="control-label">${param.name}</div>
                <svg-slider
                    data-param-index="${param.index}"
                    data-param-type="float"
                    value="${param.default || 0}"
                    min="${param.min || 0}"
                    max="${param.max || 100}"
                    width="${width}"
                    color="${color}"
                    style="height: ${height}px;">
                </svg-slider>
                <div class="control-value" data-value-for="${param.index}">${displayValue}</div>
            </div>
        `;
    }

    renderCheckbox(param) {
        const checked = param.default ? 'checked' : '';
        return `
            <div class="control-wrapper" style="padding: 0 10px; text-align: center;">
                <div class="control-label">${param.name}</div>
                <input type="checkbox"
                    data-param-index="${param.index}"
                    data-param-type="boolean"
                    ${checked}>
            </div>
        `;
    }

    bindEvents() {
        // Enum controls
        this.shadowRoot.querySelectorAll('select[data-param-index]').forEach(select => {
            select.addEventListener('change', (e) => {
                const index = parseInt(e.target.dataset.paramIndex);
                const value = parseFloat(e.target.value);
                this.updateParameter(index, value);
            });
        });

        // Slider controls
        this.shadowRoot.querySelectorAll('svg-slider[data-param-index]').forEach(slider => {
            slider.addEventListener('input', (e) => {
                const index = parseInt(slider.dataset.paramIndex);
                const value = parseInt(e.detail.value);

                // Update display
                const valueDisplay = this.shadowRoot.querySelector(`[data-value-for="${index}"]`);
                if (valueDisplay) {
                    const param = this.getParamByIndex(index);
                    valueDisplay.textContent = this.formatValue(value, param);
                }

                // Update synth (scale to 0-1 if needed)
                const scaledValue = this.scaleValue(value, index);
                this.updateParameter(index, scaledValue);
            });
        });

        // Checkbox controls
        this.shadowRoot.querySelectorAll('input[type="checkbox"][data-param-index]').forEach(checkbox => {
            checkbox.addEventListener('change', (e) => {
                const index = parseInt(e.target.dataset.paramIndex);
                const value = e.target.checked ? 1.0 : 0.0;
                this.updateParameter(index, value);
            });
        });
    }

    updateParameter(index, value) {
        if (this.synthInstance && typeof this.synthInstance.setParameter === 'function') {
            this.synthInstance.setParameter(index, value);
        }
    }

    /**
     * Update UI controls from parameter values (called when preset loads)
     */
    updateUIFromParameters(parameters) {
        console.log('[SynthUI] Updating UI from', parameters.length, 'parameters:', parameters);

        for (let i = 0; i < parameters.length; i++) {
            const value = parameters[i];
            const param = this.getParamByIndex(i);
            if (!param) {
                console.warn(`[SynthUI] No param definition for index ${i}`);
                continue;
            }

            console.log(`[SynthUI] Param ${i} (${param.name}): value=${value}, type=${param.type}`);

            // Update UI control based on type
            if (param.type === 'enum') {
                const select = this.shadowRoot.querySelector(`select[data-param-index="${i}"]`);
                if (select) {
                    console.log(`[SynthUI] Setting select ${i} to ${value}`);
                    select.value = value;
                } else {
                    console.warn(`[SynthUI] Select not found for param ${i}`);
                }
            } else if (param.type === 'boolean') {
                const checkbox = this.shadowRoot.querySelector(`input[type="checkbox"][data-param-index="${i}"]`);
                if (checkbox) {
                    const checked = value > 0.5;
                    console.log(`[SynthUI] Setting checkbox ${i} to ${checked}`);
                    checkbox.checked = checked;
                } else {
                    console.warn(`[SynthUI] Checkbox not found for param ${i}`);
                }
            } else if (param.type === 'float' || param.type === 'int') {
                // Scale value back to UI range (0-100)
                const uiValue = param.max === 100 && param.scale === 'normalized' ? value * 100 : value;

                console.log(`[SynthUI] Param ${i}: value=${value}, uiValue=${uiValue}, max=${param.max}, scale=${param.scale}`);

                // Update slider
                const slider = this.shadowRoot.querySelector(`svg-slider[data-param-index="${i}"]`);
                if (slider) {
                    console.log(`[SynthUI] Setting slider ${i} to ${uiValue}`);
                    slider.value = uiValue;
                    // Force attribute update for visibility
                    slider.setAttribute('value', uiValue);
                } else {
                    console.warn(`[SynthUI] Slider not found for param ${i}`);
                }

                // Update value display
                const valueDisplay = this.shadowRoot.querySelector(`[data-value-for="${i}"]`);
                if (valueDisplay) {
                    const formatted = this.formatValue(uiValue, param);
                    console.log(`[SynthUI] Setting value display ${i} to "${formatted}"`);
                    valueDisplay.textContent = formatted;
                }
            }
        }

        console.log('[SynthUI] ✅ UI synchronized with preset');
    }

    getParamByIndex(index) {
        if (!this.synthInstance) return null;
        const params = this.synthInstance.getParameterInfo ?
            this.synthInstance.getParameterInfo() :
            (this.synth.getParameterInfo ? this.synth.getParameterInfo() : []);
        return params.find(p => p.index === index);
    }

    scaleValue(value, index) {
        const param = this.getParamByIndex(index);
        if (!param) return value;

        // If param expects 0-1 range but slider is 0-100, scale it
        if (param.type === 'float' && param.max === 100 && param.scale === 'normalized') {
            return value / 100.0;
        }

        return value;
    }

    formatValue(value, param) {
        if (!param) return value;

        // Special format for sustain (0 = ∞)
        if (param.format === 'sustain' && value === 0) {
            return '∞';
        }

        if (param.unit === '%') {
            return Math.round(value) + '%';
        }

        if (param.type === 'int') {
            return Math.round(value).toString();
        }

        if (param.type === 'float') {
            return Math.round(value).toString();
        }

        return value.toString();
    }

    getColorForParam(param) {
        // Smart color selection based on parameter name
        const name = param.name.toLowerCase();

        if (name.includes('attack')) return '#CF1A37';
        if (name.includes('decay')) return '#E29E1A';
        if (name.includes('sustain')) return '#50C878';
        if (name.includes('release')) return '#8E44AD';
        if (name.includes('filter')) return '#E74C3C';
        if (name.includes('volume')) return '#00CC66';
        if (name.includes('pulse') || name.includes('pw')) return '#0066FF';

        // Voice colors
        if (name.includes('v1') || name.includes('voice 1')) return '#0066FF';
        if (name.includes('v2') || name.includes('voice 2')) return '#00CC66';
        if (name.includes('v3') || name.includes('voice 3')) return '#FF6B6B';

        return '#4A90E2'; // Default blue
    }
}

customElements.define('synth-ui', SynthUI);

export { SynthUI };
