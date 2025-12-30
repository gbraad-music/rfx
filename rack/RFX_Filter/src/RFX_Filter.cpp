#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_filter.h"
}

struct RFX_Filter : Module {
	enum ParamId {
		CUTOFF_PARAM,
		RESONANCE_PARAM,
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

	FXFilter* filter;
	int sampleRate;

	RFX_Filter() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters (0.0 - 1.0 range)
		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff");
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.0f, "Resonance");

		// Configure ports
		configInput(AUDIO_L_INPUT, "Left audio");
		configInput(AUDIO_R_INPUT, "Right audio");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create effect
		filter = fx_filter_create();
		fx_filter_set_enabled(filter, 1);
		sampleRate = 44100;
	}

	~RFX_Filter() {
		if (filter) {
			fx_filter_destroy(filter);
		}
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_filter_set_cutoff(filter, params[CUTOFF_PARAM].getValue());
		fx_filter_set_resonance(filter, params[RESONANCE_PARAM].getValue());

		// Get input
		float left = inputs[AUDIO_L_INPUT].getVoltage() / 5.f;
		float right = inputs[AUDIO_R_INPUT].isConnected() ?
		              inputs[AUDIO_R_INPUT].getVoltage() / 5.f : left;

		// Process
		fx_filter_process_frame(filter, &left, &right, sampleRate);

		// Set output
		outputs[AUDIO_L_OUTPUT].setVoltage(left * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(right * 5.f);
	}
};

struct RFX_FilterWidget : ModuleWidget {
	RFX_FilterWidget(RFX_Filter* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RFX_Filter.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "Filter";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Cutoff label (position 2)
		RegrooveLabel* cutoffLabel = new RegrooveLabel();
		cutoffLabel->box.pos = mm2px(Vec(0, 32.5));
		cutoffLabel->box.size = mm2px(Vec(30.48, 4));
		cutoffLabel->text = "Cutoff";
		cutoffLabel->fontSize = 9.0;
		addChild(cutoffLabel);

		// Cutoff knob (position 2: 43mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 43.0)), module, RFX_Filter::CUTOFF_PARAM));

		// Resonance label (position 4)
		RegrooveLabel* resonanceLabel = new RegrooveLabel();
		resonanceLabel->box.pos = mm2px(Vec(0, 68.5));
		resonanceLabel->box.size = mm2px(Vec(30.48, 4));
		resonanceLabel->text = "Res";
		resonanceLabel->fontSize = 9.0;
		addChild(resonanceLabel);

		// Resonance knob (position 4: 79mm)
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 79.0)), module, RFX_Filter::RESONANCE_PARAM));

		// IN label
		RegrooveLabel* inLabel = new RegrooveLabel();
		inLabel->box.pos = mm2px(Vec(2, 106.5));
		inLabel->box.size = mm2px(Vec(8, 4));
		inLabel->text = "In";
		inLabel->fontSize = 8.0;
		inLabel->align = NVG_ALIGN_LEFT;
		addChild(inLabel);

		// Inputs
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(13.5, 109.0)), module, RFX_Filter::AUDIO_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(23.5, 109.0)), module, RFX_Filter::AUDIO_R_INPUT));

		// OUT label
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 115.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 118.0)), module, RFX_Filter::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 118.0)), module, RFX_Filter::AUDIO_R_OUTPUT));
	}
};

Model* modelRFX_Filter = createModel<RFX_Filter, RFX_FilterWidget>("RFX_Filter");
