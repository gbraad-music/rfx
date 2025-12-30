#include "plugin.hpp"
#include "../../regroove_components.hpp"
#include <midi.hpp>

extern "C" {
	#include "../../../synth/rg909_drum_synth.h"
}

// RG808 uses same synth engine as RG909 but with different tuning/defaults
struct RG808_Drum : Module {
	enum ParamId {
		BD_LEVEL_PARAM,
		BD_TUNE_PARAM,
		BD_DECAY_PARAM,
		SD_LEVEL_PARAM,
		SD_TUNE_PARAM,
		SD_SNAPPY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		BD_TRIG_INPUT,
		SD_TRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_L_OUTPUT,
		AUDIO_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		BD_LIGHT,
		SD_LIGHT,
		LIGHTS_LEN
	};

	RG909Synth* synth;
	int sampleRate;
	dsp::SchmittTrigger bdTrigger;
	dsp::SchmittTrigger sdTrigger;
	float bdLight = 0.f;
	float sdLight = 0.f;

	RG808_Drum() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure bass drum parameters (808-style defaults)
		configParam(BD_LEVEL_PARAM, 0.f, 1.f, 0.8f, "BD Level", "%", 0.f, 100.f);
		configParam(BD_TUNE_PARAM, 0.f, 1.f, 0.5f, "BD Tune");
		configParam(BD_DECAY_PARAM, 0.f, 1.f, 0.5f, "BD Decay");

		// Configure snare drum parameters (808-style defaults)
		configParam(SD_LEVEL_PARAM, 0.f, 1.f, 0.7f, "SD Level", "%", 0.f, 100.f);
		configParam(SD_TUNE_PARAM, 0.f, 1.f, 0.5f, "SD Tune");
		configParam(SD_SNAPPY_PARAM, 0.f, 1.f, 0.5f, "SD Snappy");

		// Configure ports
		configInput(BD_TRIG_INPUT, "Bass Drum Trigger");
		configInput(SD_TRIG_INPUT, "Snare Drum Trigger");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create synth
		synth = rg909_synth_create();
		sampleRate = 44100;
	}

	~RG808_Drum() {
		if (synth) {
			rg909_synth_destroy(synth);
		}
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update synth parameters
		if (synth) {
			// BD parameters (0, 1, 2)
			rg909_synth_set_parameter(synth, 0, params[BD_LEVEL_PARAM].getValue());  // PARAM_BD_LEVEL
			rg909_synth_set_parameter(synth, 1, params[BD_TUNE_PARAM].getValue());   // PARAM_BD_TUNE
			rg909_synth_set_parameter(synth, 2, params[BD_DECAY_PARAM].getValue());  // PARAM_BD_DECAY

			// SD parameters (4, 5, 6, 7)
			rg909_synth_set_parameter(synth, 4, params[SD_LEVEL_PARAM].getValue());   // PARAM_SD_LEVEL
			rg909_synth_set_parameter(synth, 5, params[SD_TUNE_PARAM].getValue());    // PARAM_SD_TONE (reused for tune)
			rg909_synth_set_parameter(synth, 6, params[SD_SNAPPY_PARAM].getValue());  // PARAM_SD_SNAPPY
		}

		// Check for triggers
		if (inputs[BD_TRIG_INPUT].isConnected()) {
			if (bdTrigger.process(inputs[BD_TRIG_INPUT].getVoltage(), 0.1f, 2.f)) {
				// Trigger bass drum (MIDI note 36, velocity 127)
				rg909_synth_trigger_drum(synth, 36, 127, sampleRate);
				bdLight = 1.f;
			}
		}

		if (inputs[SD_TRIG_INPUT].isConnected()) {
			if (sdTrigger.process(inputs[SD_TRIG_INPUT].getVoltage(), 0.1f, 2.f)) {
				// Trigger snare drum (MIDI note 38, velocity 127)
				rg909_synth_trigger_drum(synth, 38, 127, sampleRate);
				sdLight = 1.f;
			}
		}

		// Process audio (single frame)
		float buffer[2] = {0.f, 0.f};
		rg909_synth_process_interleaved(synth, buffer, 1, sampleRate);

		// Set outputs (convert from unit scale to Rack 5V scale)
		outputs[AUDIO_L_OUTPUT].setVoltage(buffer[0] * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(buffer[1] * 5.f);

		// Update lights
		bdLight -= args.sampleTime * 5.f;
		sdLight -= args.sampleTime * 5.f;
		if (bdLight < 0.f) bdLight = 0.f;
		if (sdLight < 0.f) sdLight = 0.f;
		lights[BD_LIGHT].setBrightness(bdLight);
		lights[SD_LIGHT].setBrightness(sdLight);
	}
};

struct RG808_DrumWidget : ModuleWidget {
	RG808_DrumWidget(RG808_Drum* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RG808_Drum.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "808";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Bass Drum section label
		RegrooveLabel* bdLabel = new RegrooveLabel();
		bdLabel->box.pos = mm2px(Vec(0, 18));
		bdLabel->box.size = mm2px(Vec(30.48, 4));
		bdLabel->text = "BD";
		bdLabel->fontSize = 10.0;
		addChild(bdLabel);

		// BD knobs: Level, Tune, Decay (at positions 1, 2, 3)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(7.5, 25)), module, RG808_Drum::BD_LEVEL_PARAM));
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 25)), module, RG808_Drum::BD_TUNE_PARAM));
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(23, 25)), module, RG808_Drum::BD_DECAY_PARAM));

		// BD Trigger input with light
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(15.24, 36)), module, RG808_Drum::BD_TRIG_INPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.24, 42)), module, RG808_Drum::BD_LIGHT));

		// Separator line
		// (Will be drawn in SVG)

		// Snare Drum section label
		RegrooveLabel* sdLabel = new RegrooveLabel();
		sdLabel->box.pos = mm2px(Vec(0, 50));
		sdLabel->box.size = mm2px(Vec(30.48, 4));
		sdLabel->text = "SD";
		sdLabel->fontSize = 10.0;
		addChild(sdLabel);

		// SD knobs: Level, Tune, Snappy (at positions 1, 2, 3)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(7.5, 57)), module, RG808_Drum::SD_LEVEL_PARAM));
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 57)), module, RG808_Drum::SD_TUNE_PARAM));
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(23, 57)), module, RG808_Drum::SD_SNAPPY_PARAM));

		// SD Trigger input with light
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(15.24, 68)), module, RG808_Drum::SD_TRIG_INPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.24, 74)), module, RG808_Drum::SD_LIGHT));

		// Separator line
		// (Will be drawn in SVG)

		// OUT label
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 115.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 118.0)), module, RG808_Drum::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 118.0)), module, RG808_Drum::AUDIO_R_OUTPUT));
	}
};

Model* modelRG808_Drum = createModel<RG808_Drum, RG808_DrumWidget>("RG808_Drum");
