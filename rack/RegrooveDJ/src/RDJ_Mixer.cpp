#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_fader.h"
	#include "fx_crossfader.h"
}

struct RDJ_Mixer : Module {
	enum ParamId {
		A_LEVEL_PARAM,
		B_LEVEL_PARAM,
		XFADE_PARAM,
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

	FXFader* fader_a;
	FXFader* fader_b;
	FXCrossfader* crossfader;
	int sampleRate;

	RDJ_Mixer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters (0.0 - 1.0 range)
		configParam(A_LEVEL_PARAM, 0.f, 1.f, 1.0f, "Channel A Level", "%", 0.f, 100.f);
		configParam(B_LEVEL_PARAM, 0.f, 1.f, 1.0f, "Channel B Level", "%", 0.f, 100.f);
		configParam(XFADE_PARAM, 0.f, 1.f, 0.5f, "Crossfader Position");
		configParam(CURVE_PARAM, 0.f, 1.f, 0.0f, "Crossfader Curve");

		// Configure ports
		configInput(AUDIO_A_L_INPUT, "Channel A Left");
		configInput(AUDIO_A_R_INPUT, "Channel A Right");
		configInput(AUDIO_B_L_INPUT, "Channel B Left");
		configInput(AUDIO_B_R_INPUT, "Channel B Right");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create effects
		fader_a = fx_fader_create();
		fader_b = fx_fader_create();
		crossfader = fx_crossfader_create();
		fx_fader_set_enabled(fader_a, 1);
		fx_fader_set_enabled(fader_b, 1);
		fx_crossfader_set_enabled(crossfader, 1);
		sampleRate = 44100;
	}

	~RDJ_Mixer() {
		if (fader_a) fx_fader_destroy(fader_a);
		if (fader_b) fx_fader_destroy(fader_b);
		if (crossfader) fx_crossfader_destroy(crossfader);
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_fader_set_level(fader_a, params[A_LEVEL_PARAM].getValue());
		fx_fader_set_level(fader_b, params[B_LEVEL_PARAM].getValue());
		fx_crossfader_set_position(crossfader, params[XFADE_PARAM].getValue());
		fx_crossfader_set_curve(crossfader, params[CURVE_PARAM].getValue());

		// Get inputs (normalize to unit scale)
		float in_a_left = inputs[AUDIO_A_L_INPUT].getVoltage() / 5.f;
		float in_a_right = inputs[AUDIO_A_R_INPUT].isConnected() ?
		                   inputs[AUDIO_A_R_INPUT].getVoltage() / 5.f : in_a_left;

		float in_b_left = inputs[AUDIO_B_L_INPUT].getVoltage() / 5.f;
		float in_b_right = inputs[AUDIO_B_R_INPUT].isConnected() ?
		                   inputs[AUDIO_B_R_INPUT].getVoltage() / 5.f : in_b_left;

		// Apply channel faders
		fx_fader_process_frame(fader_a, &in_a_left, &in_a_right, sampleRate);
		fx_fader_process_frame(fader_b, &in_b_left, &in_b_right, sampleRate);

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

struct RDJ_MixerWidget : ModuleWidget {
	RDJ_MixerWidget(RDJ_Mixer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RDJ_Mixer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(60.96, 5));
		titleLabel->text = "Mixer";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// A label (above A fader at 10mm, centered over fader)
		RegrooveLabel* aLabel = new RegrooveLabel();
		aLabel->box.pos = mm2px(Vec(7, 16));
		aLabel->box.size = mm2px(Vec(6, 4));
		aLabel->text = "A";
		aLabel->fontSize = 10.0;
		aLabel->align = NVG_ALIGN_CENTER;
		addChild(aLabel);

		// B label (above B fader at 50.96mm, centered over fader)
		RegrooveLabel* bLabel = new RegrooveLabel();
		bLabel->box.pos = mm2px(Vec(47.96, 16));
		bLabel->box.size = mm2px(Vec(6, 4));
		bLabel->text = "B";
		bLabel->fontSize = 10.0;
		bLabel->align = NVG_ALIGN_CENTER;
		addChild(bLabel);

		// A Level fader (left side, starts at 19.5mm top, 73mm tall to end at 92.5mm)
		auto* sliderA = createParamCentered<RegrooveSlider>(mm2px(Vec(10, 56)), module, RDJ_Mixer::A_LEVEL_PARAM);
		sliderA->box.size = mm2px(Vec(12, 73));
		addParam(sliderA);

		// B Level fader (right side, starts at 19.5mm top, 73mm tall to end at 92.5mm)
		auto* sliderB = createParamCentered<RegrooveSlider>(mm2px(Vec(50.96, 56)), module, RDJ_Mixer::B_LEVEL_PARAM);
		sliderB->box.size = mm2px(Vec(12, 73));
		addParam(sliderB);

		// Curve switch label (at bottom of faders - moved UP)
		RegrooveLabel* curveLabel = new RegrooveLabel();
		curveLabel->box.pos = mm2px(Vec(20, 80));
		curveLabel->box.size = mm2px(Vec(20.96, 4));
		curveLabel->text = "Curve";
		curveLabel->fontSize = 9.0;
		addChild(curveLabel);

		// Curve switch (at bottom of faders - moved UP to 85mm)
		addParam(createParamCentered<RegrooveSwitch>(mm2px(Vec(30.48, 85)), module, RDJ_Mixer::CURVE_PARAM));

		// Horizontal crossfader (below curve switch, width 48mm, height 10mm)
		// Moved UP to 93mm (top edge), centered at 98mm
		// Centered horizontally at 30.48mm, which means left edge at 30.48 - 24 = 6.48mm
		auto* xfader = createParam<RegrooveSlider>(mm2px(Vec(6.48, 93)), module, RDJ_Mixer::XFADE_PARAM);
		xfader->box.size = mm2px(Vec(48, 10));
		xfader->horizontal = true;
		addParam(xfader);

		// Socket labels row (at standard position, more inward)
		RegrooveLabel* aInLabel = new RegrooveLabel();
		aInLabel->box.pos = mm2px(Vec(7.5, 110));
		aInLabel->box.size = mm2px(Vec(9, 3));
		aInLabel->text = "A";
		aInLabel->fontSize = 7.0;
		aInLabel->align = NVG_ALIGN_CENTER;
		addChild(aInLabel);

		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(25.5, 110));
		outLabel->box.size = mm2px(Vec(10, 3));
		outLabel->text = "Out";
		outLabel->fontSize = 7.0;
		aInLabel->align = NVG_ALIGN_CENTER;
		addChild(outLabel);

		RegrooveLabel* bInLabel = new RegrooveLabel();
		bInLabel->box.pos = mm2px(Vec(44.5, 110));
		bInLabel->box.size = mm2px(Vec(9, 3));
		bInLabel->text = "B";
		bInLabel->fontSize = 7.0;
		bInLabel->align = NVG_ALIGN_CENTER;
		addChild(bInLabel);

		// Sockets row at 118mm (standard OUT position): [AL] [AR] [OL] [OR] [BL] [BR]
		// A inputs (left)
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(7.5, 118.0)), module, RDJ_Mixer::AUDIO_A_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(16.5, 118.0)), module, RDJ_Mixer::AUDIO_A_R_INPUT));

		// Outputs (center)
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(25.5, 118.0)), module, RDJ_Mixer::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(35.5, 118.0)), module, RDJ_Mixer::AUDIO_R_OUTPUT));

		// B inputs (right)
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(44.5, 118.0)), module, RDJ_Mixer::AUDIO_B_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(53.5, 118.0)), module, RDJ_Mixer::AUDIO_B_R_INPUT));
	}
};

Model* modelRDJ_Mixer = createModel<RDJ_Mixer, RDJ_MixerWidget>("RDJ_Mixer");
