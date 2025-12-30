#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_compressor.h"
}

struct RFX_Compressor : Module {
	enum ParamId {
		THRESHOLD_PARAM,
		RATIO_PARAM,
		ATTACK_PARAM,
		RELEASE_PARAM,
		MAKEUP_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_L_INPUT,
		AUDIO_R_INPUT,
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

	FXCompressor* compressor;
	int sampleRate;

	RFX_Compressor() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters (0.0 - 1.0 range)
		configParam(THRESHOLD_PARAM, 0.f, 1.f, 0.5f, "Threshold");
		configParam(RATIO_PARAM, 0.f, 1.f, 0.5f, "Ratio");
		configParam(ATTACK_PARAM, 0.f, 1.f, 0.5f, "Attack");
		configParam(RELEASE_PARAM, 0.f, 1.f, 0.5f, "Release");
		configParam(MAKEUP_PARAM, 0.f, 1.f, 0.5f, "Makeup");

		// Configure ports
		configInput(AUDIO_L_INPUT, "Left audio");
		configInput(AUDIO_R_INPUT, "Right audio");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create effect
		compressor = fx_compressor_create();
		fx_compressor_set_enabled(compressor, 1);
		sampleRate = 44100;
	}

	~RFX_Compressor() {
		if (compressor) {
			fx_compressor_destroy(compressor);
		}
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_compressor_set_threshold(compressor, params[THRESHOLD_PARAM].getValue());
		fx_compressor_set_ratio(compressor, params[RATIO_PARAM].getValue());
		fx_compressor_set_attack(compressor, params[ATTACK_PARAM].getValue());
		fx_compressor_set_release(compressor, params[RELEASE_PARAM].getValue());
		fx_compressor_set_makeup(compressor, params[MAKEUP_PARAM].getValue());

		// Get input
		float left = inputs[AUDIO_L_INPUT].getVoltage() / 5.f;
		float right = inputs[AUDIO_R_INPUT].isConnected() ?
		              inputs[AUDIO_R_INPUT].getVoltage() / 5.f : left;

		// Process
		fx_compressor_process_frame(compressor, &left, &right, sampleRate);

		// Set output
		outputs[AUDIO_L_OUTPUT].setVoltage(left * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(right * 5.f);
	}
};

struct RFX_CompressorWidget : ModuleWidget {
	RFX_CompressorWidget(RFX_Compressor* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RFX_Compressor.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "Comp";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Threshold label (position 1)
		RegrooveLabel* thresholdLabel = new RegrooveLabel();
		thresholdLabel->box.pos = mm2px(Vec(0, 14.5));
		thresholdLabel->box.size = mm2px(Vec(30.48, 4));
		thresholdLabel->text = "Thresh";
		thresholdLabel->fontSize = 9.0;
		addChild(thresholdLabel);

		// Threshold knob (position 1: 25mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 25.0)), module, RFX_Compressor::THRESHOLD_PARAM));

		// Ratio label (position 2)
		RegrooveLabel* ratioLabel = new RegrooveLabel();
		ratioLabel->box.pos = mm2px(Vec(0, 32.5));
		ratioLabel->box.size = mm2px(Vec(30.48, 4));
		ratioLabel->text = "Ratio";
		ratioLabel->fontSize = 9.0;
		addChild(ratioLabel);

		// Ratio knob (position 2: 43mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 43.0)), module, RFX_Compressor::RATIO_PARAM));

		// Attack label (position 3)
		RegrooveLabel* attackLabel = new RegrooveLabel();
		attackLabel->box.pos = mm2px(Vec(0, 50.5));
		attackLabel->box.size = mm2px(Vec(30.48, 4));
		attackLabel->text = "Attack";
		attackLabel->fontSize = 9.0;
		addChild(attackLabel);

		// Attack knob (position 3: 61mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 61.0)), module, RFX_Compressor::ATTACK_PARAM));

		// Release label (position 4)
		RegrooveLabel* releaseLabel = new RegrooveLabel();
		releaseLabel->box.pos = mm2px(Vec(0, 68.5));
		releaseLabel->box.size = mm2px(Vec(30.48, 4));
		releaseLabel->text = "Release";
		releaseLabel->fontSize = 9.0;
		addChild(releaseLabel);

		// Release knob (position 4: 79mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 79.0)), module, RFX_Compressor::RELEASE_PARAM));

		// Makeup label (position 5)
		RegrooveLabel* makeupLabel = new RegrooveLabel();
		makeupLabel->box.pos = mm2px(Vec(0, 86.5));
		makeupLabel->box.size = mm2px(Vec(30.48, 4));
		makeupLabel->text = "Makeup";
		makeupLabel->fontSize = 9.0;
		addChild(makeupLabel);

		// Makeup knob (position 5: 97mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 97.0)), module, RFX_Compressor::MAKEUP_PARAM));

		// IN label
		RegrooveLabel* inLabel = new RegrooveLabel();
		inLabel->box.pos = mm2px(Vec(2, 106.5));
		inLabel->box.size = mm2px(Vec(8, 4));
		inLabel->text = "In";
		inLabel->fontSize = 8.0;
		inLabel->align = NVG_ALIGN_LEFT;
		addChild(inLabel);

		// Inputs
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(13.5, 109.0)), module, RFX_Compressor::AUDIO_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(23.5, 109.0)), module, RFX_Compressor::AUDIO_R_INPUT));

		// OUT label
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 115.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 118.0)), module, RFX_Compressor::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 118.0)), module, RFX_Compressor::AUDIO_R_OUTPUT));
	}
};

Model* modelRFX_Compressor = createModel<RFX_Compressor, RFX_CompressorWidget>("RFX_Compressor");
