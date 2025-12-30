#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_crossfader.h"
}

struct RDJ_XFader : Module {
	enum ParamId {
		POSITION_PARAM,
		CURVE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_A_L_INPUT,
		AUDIO_A_R_INPUT,
		AUDIO_B_L_INPUT,
		AUDIO_B_R_INPUT,
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

	FXCrossfader* crossfader;
	int sampleRate;

	RDJ_XFader() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters (0.0 - 1.0 range)
		configParam(POSITION_PARAM, 0.f, 1.f, 0.5f, "Position");
		configParam(CURVE_PARAM, 0.f, 1.f, 0.0f, "Curve");

		// Configure ports
		configInput(AUDIO_A_L_INPUT, "Channel A Left");
		configInput(AUDIO_A_R_INPUT, "Channel A Right");
		configInput(AUDIO_B_L_INPUT, "Channel B Left");
		configInput(AUDIO_B_R_INPUT, "Channel B Right");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create effect
		crossfader = fx_crossfader_create();
		fx_crossfader_set_enabled(crossfader, 1);
		sampleRate = 44100;
	}

	~RDJ_XFader() {
		if (crossfader) {
			fx_crossfader_destroy(crossfader);
		}
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_crossfader_set_position(crossfader, params[POSITION_PARAM].getValue());
		fx_crossfader_set_curve(crossfader, params[CURVE_PARAM].getValue());

		// Get inputs (normalize to unit scale)
		float in_a_left = inputs[AUDIO_A_L_INPUT].getVoltage() / 5.f;
		float in_a_right = inputs[AUDIO_A_R_INPUT].isConnected() ?
		                   inputs[AUDIO_A_R_INPUT].getVoltage() / 5.f : in_a_left;

		float in_b_left = inputs[AUDIO_B_L_INPUT].getVoltage() / 5.f;
		float in_b_right = inputs[AUDIO_B_R_INPUT].isConnected() ?
		                   inputs[AUDIO_B_R_INPUT].getVoltage() / 5.f : in_b_left;

		// Process crossfade
		float out_left, out_right;
		fx_crossfader_process_frame(crossfader,
		                             in_a_left, in_a_right,
		                             in_b_left, in_b_right,
		                             &out_left, &out_right,
		                             sampleRate);

		// Set output
		outputs[AUDIO_L_OUTPUT].setVoltage(out_left * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(out_right * 5.f);
	}
};

struct RDJ_XFaderWidget : ModuleWidget {
	RDJ_XFaderWidget(RDJ_XFader* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RDJ_XFader.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "XFade";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Crossfader position slider
		// Align top with regular fader at 19.5mm
		// When we change size after createParamCentered, the top-left stays fixed, not the center
		// Curve switch is at 90.5mm, need clearance
		// Top at 19.5mm, give 3mm clearance before switch at 90.5mm
		// Max end: 87.5mm → height = 87.5 - 19.5 = 68mm (use 65mm for safety)
		auto* slider = createParamCentered<RegrooveSlider>(mm2px(Vec(15.24, 59.5)), module, RDJ_XFader::POSITION_PARAM);
		slider->box.size = mm2px(Vec(12, 65));  // 65mm tall slider
		addParam(slider);

		// Curve switch label
		RegrooveLabel* curveLabel = new RegrooveLabel();
		curveLabel->box.pos = mm2px(Vec(0, 85.5));
		curveLabel->box.size = mm2px(Vec(30.48, 4));
		curveLabel->text = "Curve";
		curveLabel->fontSize = 9.0;
		addChild(curveLabel);

		// Curve switch (horizontal toggle) - moved up 1mm
		addParam(createParamCentered<RegrooveSwitch>(mm2px(Vec(15.24, 90.5)), module, RDJ_XFader::CURVE_PARAM));

		// A label
		RegrooveLabel* aLabel = new RegrooveLabel();
		aLabel->box.pos = mm2px(Vec(2, 97.5));
		aLabel->box.size = mm2px(Vec(8, 4));
		aLabel->text = "A";
		aLabel->fontSize = 8.0;
		aLabel->align = NVG_ALIGN_LEFT;
		addChild(aLabel);

		// Channel A Inputs
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(13.5, 100.0)), module, RDJ_XFader::AUDIO_A_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(23.5, 100.0)), module, RDJ_XFader::AUDIO_A_R_INPUT));

		// B label (aligns with standard IN position)
		RegrooveLabel* bLabel = new RegrooveLabel();
		bLabel->box.pos = mm2px(Vec(2, 106.5));
		bLabel->box.size = mm2px(Vec(8, 4));
		bLabel->text = "B";
		bLabel->fontSize = 8.0;
		bLabel->align = NVG_ALIGN_LEFT;
		addChild(bLabel);

		// Channel B Inputs (aligned with standard IN at 109mm)
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(13.5, 109.0)), module, RDJ_XFader::AUDIO_B_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(23.5, 109.0)), module, RDJ_XFader::AUDIO_B_R_INPUT));

		// OUT label
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 115.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 118.0)), module, RDJ_XFader::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 118.0)), module, RDJ_XFader::AUDIO_R_OUTPUT));
	}
};

Model* modelRDJ_XFader = createModel<RDJ_XFader, RDJ_XFaderWidget>("RDJ_XFader");
