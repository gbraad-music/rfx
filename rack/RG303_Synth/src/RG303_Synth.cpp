#include "plugin.hpp"
#include "../../regroove_components.hpp"
#include <midi.hpp>

extern "C" {
	#include "../../../synth/synth_oscillator.h"
	#include "../../../synth/synth_filter.h"
	#include "../../../synth/synth_envelope.h"
}

struct RG303_Synth : Module {
	enum ParamId {
		WAVEFORM_PARAM,
		CUTOFF_PARAM,
		RESONANCE_PARAM,
		ENVMOD_PARAM,
		DECAY_PARAM,
		ACCENT_PARAM,
		SLIDE_TIME_PARAM,
		VOLUME_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_L_OUTPUT,
		AUDIO_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	// Synth components
	SynthOscillator* osc;
	SynthFilter* filter;
	SynthEnvelope* amp_env;
	SynthEnvelope* filter_env;

	// Voice state
	int currentNote;
	bool gate;
	float currentFreq;
	float targetFreq;
	bool sliding;
	float velocity;

	// MIDI
	midi::InputQueue midiInput;

	int sampleRate;

	RG303_Synth() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters
		configSwitch(WAVEFORM_PARAM, 0.f, 1.f, 0.f, "Waveform", {"Sawtooth", "Square"});
		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff");
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.5f, "Resonance");
		configParam(ENVMOD_PARAM, 0.f, 1.f, 0.5f, "Env Mod");
		configParam(DECAY_PARAM, 0.f, 1.f, 0.3f, "Decay");
		configParam(ACCENT_PARAM, 0.f, 1.f, 0.0f, "Accent");
		configParam(SLIDE_TIME_PARAM, 0.f, 1.f, 0.1f, "Slide Time");
		configParam(VOLUME_PARAM, 0.f, 1.f, 0.7f, "Volume", "%", 0.f, 100.f);

		// Configure ports
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create synth components
		osc = synth_oscillator_create();
		filter = synth_filter_create();
		amp_env = synth_envelope_create();
		filter_env = synth_envelope_create();

		// Initialize voice state
		currentNote = -1;
		gate = false;
		currentFreq = 440.f;
		targetFreq = 440.f;
		sliding = false;
		velocity = 1.f;
		sampleRate = 44100;

		// Set up TB303-style envelopes
		synth_envelope_set_attack(amp_env, 0.003f);
		synth_envelope_set_decay(amp_env, 0.2f);
		synth_envelope_set_sustain(amp_env, 0.0f);  // No sustain
		synth_envelope_set_release(amp_env, 0.01f);

		synth_envelope_set_attack(filter_env, 0.003f);
		synth_envelope_set_decay(filter_env, 0.2f);
		synth_envelope_set_sustain(filter_env, 0.0f);
		synth_envelope_set_release(filter_env, 0.01f);

		// Set filter type
		synth_filter_set_type(filter, SYNTH_FILTER_LPF);
		synth_oscillator_set_waveform(osc, SYNTH_OSC_SAW);
	}

	~RG303_Synth() {
		if (osc) synth_oscillator_destroy(osc);
		if (filter) synth_filter_destroy(filter);
		if (amp_env) synth_envelope_destroy(amp_env);
		if (filter_env) synth_envelope_destroy(filter_env);
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void processNoteOn(int note, int vel) {
		float freq = dsp::FREQ_C4 * std::pow(2.f, (note - 60) / 12.f);

		// Check if we should slide
		if (gate && params[SLIDE_TIME_PARAM].getValue() > 0.001f) {
			// Slide from current to new note
			sliding = true;
			targetFreq = freq;
		} else {
			// Hard retrigger
			currentFreq = freq;
			targetFreq = freq;
			sliding = false;
			synth_envelope_trigger(amp_env);
			synth_envelope_trigger(filter_env);
		}

		currentNote = note;
		gate = true;
		velocity = vel / 127.f;
	}

	void processNoteOff(int note) {
		if (note == currentNote) {
			gate = false;
			synth_envelope_release(amp_env);
			synth_envelope_release(filter_env);
		}
	}

	void process(const ProcessArgs& args) override {
		// Process MIDI messages
		midi::Message msg;
		while (midiInput.tryPop(&msg, args.frame)) {
			if (msg.getStatus() == 0x9) {  // Note On
				int note = msg.getNote();
				int vel = msg.getValue();
				if (vel > 0) {
					processNoteOn(note, vel);
				} else {
					processNoteOff(note);
				}
			}
			else if (msg.getStatus() == 0x8) {  // Note Off
				processNoteOff(msg.getNote());
			}
		}

		// Update synth parameters
		float waveform = params[WAVEFORM_PARAM].getValue();
		synth_oscillator_set_waveform(osc, waveform > 0.5f ? SYNTH_OSC_SQUARE : SYNTH_OSC_SAW);

		synth_filter_set_cutoff(filter, params[CUTOFF_PARAM].getValue());
		synth_filter_set_resonance(filter, params[RESONANCE_PARAM].getValue());

		// Update decay time based on parameter
		float decay = 0.01f + params[DECAY_PARAM].getValue() * 2.0f;
		synth_envelope_set_decay(amp_env, decay);
		synth_envelope_set_decay(filter_env, decay);

		// Handle portamento/slide
		if (sliding) {
			float slideTime = params[SLIDE_TIME_PARAM].getValue();
			float slideRate = 1.0f / (slideTime * sampleRate + 1.0f);
			currentFreq += (targetFreq - currentFreq) * slideRate;
			if (std::abs(currentFreq - targetFreq) < 0.1f) {
				currentFreq = targetFreq;
				sliding = false;
			}
		}

		// Generate oscillator output
		synth_oscillator_set_frequency(osc, currentFreq);
		float oscOut = synth_oscillator_process(osc, sampleRate);

		// Process envelopes
		float ampEnv = synth_envelope_process(amp_env, sampleRate);
		float filterEnvValue = synth_envelope_process(filter_env, sampleRate);

		// Apply envelope modulation to filter cutoff
		float envMod = params[ENVMOD_PARAM].getValue();
		float cutoff = params[CUTOFF_PARAM].getValue();
		float modulatedCutoff = cutoff + filterEnvValue * envMod * (1.0f - cutoff);
		modulatedCutoff = clamp(modulatedCutoff, 0.f, 1.f);
		synth_filter_set_cutoff(filter, modulatedCutoff);

		// Process filter
		float filtered = synth_filter_process(filter, oscOut, sampleRate);

		// Apply amplitude envelope and accent
		float accent = params[ACCENT_PARAM].getValue();
		float accentGain = 1.0f + accent * velocity;
		float output = filtered * ampEnv * accentGain;

		// Apply volume
		output *= params[VOLUME_PARAM].getValue();

		// Set outputs (convert to Rack 5V scale)
		outputs[AUDIO_L_OUTPUT].setVoltage(output * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(output * 5.f);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "midi", midiInput.toJson());
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* midiJ = json_object_get(rootJ, "midi");
		if (midiJ)
			midiInput.fromJson(midiJ);
	}
};

struct RG303_SynthWidget : ModuleWidget {
	RG303_SynthWidget(RG303_Synth* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RG303_Synth.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "303";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Waveform switch
		RegrooveLabel* waveLabel = new RegrooveLabel();
		waveLabel->box.pos = mm2px(Vec(0, 18));
		waveLabel->box.size = mm2px(Vec(30.48, 4));
		waveLabel->text = "Wave";
		waveLabel->fontSize = 9.0;
		addChild(waveLabel);

		addParam(createParamCentered<RegrooveSwitch>(mm2px(Vec(15.24, 22.5)), module, RG303_Synth::WAVEFORM_PARAM));

		// Knobs section - 5 knob positions (25, 43, 61, 79, 97mm)
		// Row 1: Cutoff, Resonance
		addParam(createParamCentered<RegrooveKnob>(mm2px(Vec(7.5, 43)), module, RG303_Synth::CUTOFF_PARAM));
		addParam(createParamCentered<RegrooveKnob>(mm2px(Vec(23, 43)), module, RG303_Synth::RESONANCE_PARAM));

		// Row 2: EnvMod, Decay
		addParam(createParamCentered<RegrooveKnob>(mm2px(Vec(7.5, 61)), module, RG303_Synth::ENVMOD_PARAM));
		addParam(createParamCentered<RegrooveKnob>(mm2px(Vec(23, 61)), module, RG303_Synth::DECAY_PARAM));

		// Row 3: Accent, Slide
		addParam(createParamCentered<RegrooveKnob>(mm2px(Vec(7.5, 79)), module, RG303_Synth::ACCENT_PARAM));
		addParam(createParamCentered<RegrooveKnob>(mm2px(Vec(23, 79)), module, RG303_Synth::SLIDE_TIME_PARAM));

		// Row 4: Volume (centered)
		addParam(createParamCentered<RegrooveKnob>(mm2px(Vec(15.24, 97)), module, RG303_Synth::VOLUME_PARAM));

		// MIDI indicator
		MidiWidget* midiWidget = createWidget<MidiWidget>(mm2px(Vec(3, 103)));
		midiWidget->box.size = mm2px(Vec(24.48, 7));
		if (module) {
			midiWidget->setMidiPort(&module->midiInput);
		}
		addChild(midiWidget);

		// OUT label
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 115.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 118.0)), module, RG303_Synth::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 118.0)), module, RG303_Synth::AUDIO_R_OUTPUT));
	}
};

Model* modelRG303_Synth = createModel<RG303_Synth, RG303_SynthWidget>("RG303_Synth");
