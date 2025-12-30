#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_model1_trim.h"
	#include "fx_model1_hpf.h"
	#include "fx_model1_lpf.h"
	#include "fx_model1_sculpt.h"
}

struct RM1 : Module {
	enum ParamId {
		TRIM_PARAM,
		CONTOUR_HI_PARAM,
		SCULPT_FREQ_PARAM,
		SCULPT_BOOST_PARAM,
		CONTOUR_LO_PARAM,
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
		DRIVE_LIGHT,
		LIGHTS_LEN
	};

	FXModel1Trim* trim;
	FXModel1HPF* hpf;
	FXModel1LPF* lpf;
	FXModel1Sculpt* sculpt;
	int sampleRate;

	RM1() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters
		configParam(TRIM_PARAM, 0.f, 1.f, 0.5f, "Trim");
		configParam(CONTOUR_HI_PARAM, 0.f, 1.f, 0.0f, "Contour (Hi)");
		configParam(SCULPT_FREQ_PARAM, 0.f, 1.f, 0.5f, "Sculpt Freq");
		configParam(SCULPT_BOOST_PARAM, 0.f, 1.f, 0.5f, "Boost");
		configParam(CONTOUR_LO_PARAM, 0.f, 1.f, 1.0f, "Contour (Lo)");

		// Configure ports
		configInput(AUDIO_L_INPUT, "Left audio");
		configInput(AUDIO_R_INPUT, "Right audio");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create effects
		trim = fx_model1_trim_create();
		fx_model1_trim_set_enabled(trim, 1);

		hpf = fx_model1_hpf_create();
		fx_model1_hpf_set_enabled(hpf, 1);

		lpf = fx_model1_lpf_create();
		fx_model1_lpf_set_enabled(lpf, 1);

		sculpt = fx_model1_sculpt_create();
		fx_model1_sculpt_set_enabled(sculpt, 1);

		sampleRate = 44100;
	}

	~RM1() {
		if (trim) fx_model1_trim_destroy(trim);
		if (hpf) fx_model1_hpf_destroy(hpf);
		if (lpf) fx_model1_lpf_destroy(lpf);
		if (sculpt) fx_model1_sculpt_destroy(sculpt);
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_model1_trim_set_drive(trim, params[TRIM_PARAM].getValue());
		fx_model1_hpf_set_cutoff(hpf, params[CONTOUR_HI_PARAM].getValue());
		fx_model1_lpf_set_cutoff(lpf, params[CONTOUR_LO_PARAM].getValue());
		fx_model1_sculpt_set_frequency(sculpt, params[SCULPT_FREQ_PARAM].getValue());
		fx_model1_sculpt_set_gain(sculpt, params[SCULPT_BOOST_PARAM].getValue());

		// Get input
		float left = inputs[AUDIO_L_INPUT].getVoltage() / 5.f;
		float right = inputs[AUDIO_R_INPUT].isConnected() ?
		              inputs[AUDIO_R_INPUT].getVoltage() / 5.f : left;

		// Process chain: Trim -> HPF -> LPF -> Sculpt
		fx_model1_trim_process_frame(trim, &left, &right, sampleRate);
		fx_model1_hpf_process_frame(hpf, &left, &right, sampleRate);
		fx_model1_lpf_process_frame(lpf, &left, &right, sampleRate);
		fx_model1_sculpt_process_frame(sculpt, &left, &right, sampleRate);

		// Set output
		outputs[AUDIO_L_OUTPUT].setVoltage(left * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(right * 5.f);

		// Update drive LED based on peak level (detect clipping)
		float peakLevel = fx_model1_trim_get_peak_level(trim);
		lights[DRIVE_LIGHT].setBrightness(peakLevel > 0.8f ? 1.0f : 0.0f);
	}
};

struct RM1Widget : ModuleWidget {
	RM1Widget(RM1* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RM1.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 8));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "M1";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Trim label - positioned WELL ABOVE the knob
		RegrooveLabel* trimLabel = new RegrooveLabel();
		trimLabel->box.pos = mm2px(Vec(0, 16.5));
		trimLabel->box.size = mm2px(Vec(30.48, 4));
		trimLabel->text = "Trim";
		trimLabel->fontSize = 8.0;
		addChild(trimLabel);

		// Trim knob
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 27.0)), module, RM1::TRIM_PARAM));

		// Drive LED (next to Trim knob)
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(25.0, 23.0)), module, RM1::DRIVE_LIGHT));

		// Drive label (next to LED)
		RegrooveLabel* driveLabel = new RegrooveLabel();
		driveLabel->box.pos = mm2px(Vec(19.5, 18.5));
		driveLabel->box.size = mm2px(Vec(12, 3));
		driveLabel->text = "Drive";
		driveLabel->fontSize = 6.0;
		addChild(driveLabel);

		// Contour Hi label - positioned WELL ABOVE the knob
		RegrooveLabel* contHiLabel = new RegrooveLabel();
		contHiLabel->box.pos = mm2px(Vec(0, 34.5));
		contHiLabel->box.size = mm2px(Vec(30.48, 4));
		contHiLabel->text = "Contour (Hi)";
		contHiLabel->fontSize = 7.0;
		addChild(contHiLabel);

		// Contour Hi knob
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 45.0)), module, RM1::CONTOUR_HI_PARAM));

		// Sculpt Freq label - positioned WELL ABOVE the knob
		RegrooveLabel* freqLabel = new RegrooveLabel();
		freqLabel->box.pos = mm2px(Vec(0, 52.5));
		freqLabel->box.size = mm2px(Vec(30.48, 4));
		freqLabel->text = "Sculpt Freq";
		freqLabel->fontSize = 7.5;
		addChild(freqLabel);

		// Sculpt Freq knob
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 63.0)), module, RM1::SCULPT_FREQ_PARAM));

		// Boost label - positioned WELL ABOVE the knob
		RegrooveLabel* boostLabel = new RegrooveLabel();
		boostLabel->box.pos = mm2px(Vec(0, 70.5));
		boostLabel->box.size = mm2px(Vec(30.48, 4));
		boostLabel->text = "Boost";
		boostLabel->fontSize = 8.0;
		addChild(boostLabel);

		// Boost knob
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 81.0)), module, RM1::SCULPT_BOOST_PARAM));

		// Contour Lo label - positioned WELL ABOVE the knob
		RegrooveLabel* contLoLabel = new RegrooveLabel();
		contLoLabel->box.pos = mm2px(Vec(0, 88.5));
		contLoLabel->box.size = mm2px(Vec(30.48, 4));
		contLoLabel->text = "Contour (Lo)";
		contLoLabel->fontSize = 7.0;
		addChild(contLoLabel);

		// Contour Lo knob
		addParam(createParamCentered<RegrooveMediumKnob>(mm2px(Vec(15.24, 99.0)), module, RM1::CONTOUR_LO_PARAM));

		// STANDARD POSITION: IN label (left of ports)
		RegrooveLabel* inLabel = new RegrooveLabel();
		inLabel->box.pos = mm2px(Vec(2, 108.5));
		inLabel->box.size = mm2px(Vec(8, 4));
		inLabel->text = "In";
		inLabel->fontSize = 8.0;
		inLabel->align = NVG_ALIGN_LEFT;
		addChild(inLabel);

		// STANDARD POSITION: Inputs (horizontal pair with label on left)
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(13.5, 111.0)), module, RM1::AUDIO_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(23.5, 111.0)), module, RM1::AUDIO_R_INPUT));

		// STANDARD POSITION: OUT label (left of ports)
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 116.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// STANDARD POSITION: Outputs (horizontal pair with label on left)
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 119.0)), module, RM1::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 119.0)), module, RM1::AUDIO_R_OUTPUT));
	}
};

Model* modelRM1 = createModel<RM1, RM1Widget>("RM1");
