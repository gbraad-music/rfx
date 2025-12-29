#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_eq.h"
}

struct RFX_EQ : Module {
	enum ParamId {
		LOW_PARAM,
		MID_PARAM,
		HIGH_PARAM,
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

	FXEqualizer* eq;
	int sampleRate;

	RFX_EQ() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters (0.0 - 1.0 range, 0.5 = neutral)
		configParam(LOW_PARAM, 0.f, 1.f, 0.5f, "Low", " dB", 0, 24.f, -12.f);
		configParam(MID_PARAM, 0.f, 1.f, 0.5f, "Mid", " dB", 0, 24.f, -12.f);
		configParam(HIGH_PARAM, 0.f, 1.f, 0.5f, "High", " dB", 0, 24.f, -12.f);

		// Configure ports
		configInput(AUDIO_L_INPUT, "Left audio");
		configInput(AUDIO_R_INPUT, "Right audio");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create EQ effect
		eq = fx_eq_create();
		fx_eq_set_enabled(eq, 1);
		sampleRate = 44100;
	}

	~RFX_EQ() {
		if (eq) {
			fx_eq_destroy(eq);
		}
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_eq_set_low(eq, params[LOW_PARAM].getValue());
		fx_eq_set_mid(eq, params[MID_PARAM].getValue());
		fx_eq_set_high(eq, params[HIGH_PARAM].getValue());

		// Get input
		float left = inputs[AUDIO_L_INPUT].getVoltage() / 5.f;
		float right = inputs[AUDIO_R_INPUT].isConnected() ?
		              inputs[AUDIO_R_INPUT].getVoltage() / 5.f : left;

		// Process
		fx_eq_process_frame(eq, &left, &right, sampleRate);

		// Set output
		outputs[AUDIO_L_OUTPUT].setVoltage(left * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(right * 5.f);
	}
};

struct RFX_EQWidget : ModuleWidget {
	RFX_EQWidget(RFX_EQ* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RFX_EQ.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Regroove knobs (Hi, Mid, Low order to match SVG)
		addParam(createParamCentered<RegrooveSmallKnob>(mm2px(Vec(15.24, 28.0)), module, RFX_EQ::HIGH_PARAM));
		addParam(createParamCentered<RegrooveSmallKnob>(mm2px(Vec(15.24, 48.0)), module, RFX_EQ::MID_PARAM));
		addParam(createParamCentered<RegrooveSmallKnob>(mm2px(Vec(15.24, 68.0)), module, RFX_EQ::LOW_PARAM));

		// Inputs
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(15.24, 85.0)), module, RFX_EQ::AUDIO_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(15.24, 97.0)), module, RFX_EQ::AUDIO_R_INPUT));

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(15.24, 112.0)), module, RFX_EQ::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(15.24, 124.0)), module, RFX_EQ::AUDIO_R_OUTPUT));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 8));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "EQ";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Hi label
		RegrooveLabel* hiLabel = new RegrooveLabel();
		hiLabel->box.pos = mm2px(Vec(0, 19.5));
		hiLabel->box.size = mm2px(Vec(30.48, 4));
		hiLabel->text = "Hi";
		hiLabel->fontSize = 9.0;
		addChild(hiLabel);

		// Mid label
		RegrooveLabel* midLabel = new RegrooveLabel();
		midLabel->box.pos = mm2px(Vec(0, 39.5));
		midLabel->box.size = mm2px(Vec(30.48, 4));
		midLabel->text = "Mid";
		midLabel->fontSize = 9.0;
		addChild(midLabel);

		// Low label
		RegrooveLabel* lowLabel = new RegrooveLabel();
		lowLabel->box.pos = mm2px(Vec(0, 59.5));
		lowLabel->box.size = mm2px(Vec(30.48, 4));
		lowLabel->text = "Low";
		lowLabel->fontSize = 9.0;
		addChild(lowLabel);

		// In L label
		RegrooveLabel* inLLabel = new RegrooveLabel();
		inLLabel->box.pos = mm2px(Vec(0, 77));
		inLLabel->box.size = mm2px(Vec(30.48, 4));
		inLLabel->text = "In L";
		inLLabel->fontSize = 8.0;
		addChild(inLLabel);

		// In R label
		RegrooveLabel* inRLabel = new RegrooveLabel();
		inRLabel->box.pos = mm2px(Vec(0, 89));
		inRLabel->box.size = mm2px(Vec(30.48, 4));
		inRLabel->text = "In R";
		inRLabel->fontSize = 8.0;
		addChild(inRLabel);

		// Out L label
		RegrooveLabel* outLLabel = new RegrooveLabel();
		outLLabel->box.pos = mm2px(Vec(0, 104));
		outLLabel->box.size = mm2px(Vec(30.48, 4));
		outLLabel->text = "Out L";
		outLLabel->fontSize = 8.0;
		addChild(outLLabel);

		// Out R label
		RegrooveLabel* outRLabel = new RegrooveLabel();
		outRLabel->box.pos = mm2px(Vec(0, 116));
		outRLabel->box.size = mm2px(Vec(30.48, 4));
		outRLabel->text = "Out R";
		outRLabel->fontSize = 8.0;
		addChild(outRLabel);
	}
};

Model* modelRFX_EQ = createModel<RFX_EQ, RFX_EQWidget>("RFX_EQ");
