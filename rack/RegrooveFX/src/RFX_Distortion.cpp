#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_distortion.h"
}

struct RFX_Distortion : Module {
	enum ParamId {
		DRIVE_PARAM,
		MIX_PARAM,
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

	FXDistortion* distortion;
	int sampleRate;

	RFX_Distortion() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters (0.0 - 1.0 range)
		configParam(DRIVE_PARAM, 0.f, 1.f, 0.5f, "Drive");
		configParam(MIX_PARAM, 0.f, 1.f, 1.0f, "Mix");

		// Configure ports
		configInput(AUDIO_L_INPUT, "Left audio");
		configInput(AUDIO_R_INPUT, "Right audio");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create effect
		distortion = fx_distortion_create();
		fx_distortion_set_enabled(distortion, 1);
		sampleRate = 44100;
	}

	~RFX_Distortion() {
		if (distortion) {
			fx_distortion_destroy(distortion);
		}
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_distortion_set_drive(distortion, params[DRIVE_PARAM].getValue());
		fx_distortion_set_mix(distortion, params[MIX_PARAM].getValue());

		// Get input
		float left = inputs[AUDIO_L_INPUT].getVoltage() / 5.f;
		float right = inputs[AUDIO_R_INPUT].isConnected() ?
		              inputs[AUDIO_R_INPUT].getVoltage() / 5.f : left;

		// Process
		fx_distortion_process_frame(distortion, &left, &right, sampleRate);

		// Set output
		outputs[AUDIO_L_OUTPUT].setVoltage(left * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(right * 5.f);
	}
};

struct RFX_DistortionWidget : ModuleWidget {
	RFX_DistortionWidget(RFX_Distortion* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RFX_Distortion.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "Dist";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Drive label (position 2)
		RegrooveLabel* driveLabel = new RegrooveLabel();
		driveLabel->box.pos = mm2px(Vec(0, 32.5));
		driveLabel->box.size = mm2px(Vec(30.48, 4));
		driveLabel->text = "Drive";
		driveLabel->fontSize = 9.0;
		addChild(driveLabel);

		// Drive knob (position 2: 43mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 43.0)), module, RFX_Distortion::DRIVE_PARAM));

		// Mix label (position 4)
		RegrooveLabel* mixLabel = new RegrooveLabel();
		mixLabel->box.pos = mm2px(Vec(0, 68.5));
		mixLabel->box.size = mm2px(Vec(30.48, 4));
		mixLabel->text = "Mix";
		mixLabel->fontSize = 9.0;
		addChild(mixLabel);

		// Mix knob (position 4: 79mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 79.0)), module, RFX_Distortion::MIX_PARAM));

		// IN label
		RegrooveLabel* inLabel = new RegrooveLabel();
		inLabel->box.pos = mm2px(Vec(2, 106.5));
		inLabel->box.size = mm2px(Vec(8, 4));
		inLabel->text = "In";
		inLabel->fontSize = 8.0;
		inLabel->align = NVG_ALIGN_LEFT;
		addChild(inLabel);

		// Inputs
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(13.5, 109.0)), module, RFX_Distortion::AUDIO_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(23.5, 109.0)), module, RFX_Distortion::AUDIO_R_INPUT));

		// OUT label
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 115.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 118.0)), module, RFX_Distortion::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 118.0)), module, RFX_Distortion::AUDIO_R_OUTPUT));
	}
};

Model* modelRFX_Distortion = createModel<RFX_Distortion, RFX_DistortionWidget>("RFX_Distortion");
