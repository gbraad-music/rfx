#include "plugin.hpp"
#include "../../regroove_components.hpp"

extern "C" {
	#include "fx_fader.h"
}

struct RDJ_Fader : Module {
	enum ParamId {
		LEVEL_PARAM,
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

	FXFader* fader;
	int sampleRate;

	RDJ_Fader() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		// Configure parameters (0.0 - 1.0 range)
		configParam(LEVEL_PARAM, 0.f, 1.f, 1.0f, "Level", " dB", -10, 40, -40);

		// Configure ports
		configInput(AUDIO_L_INPUT, "Left audio");
		configInput(AUDIO_R_INPUT, "Right audio");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		// Create effect
		fader = fx_fader_create();
		fx_fader_set_enabled(fader, 1);
		sampleRate = 44100;
	}

	~RDJ_Fader() {
		if (fader) {
			fx_fader_destroy(fader);
		}
	}

	void onSampleRateChange() override {
		sampleRate = (int)APP->engine->getSampleRate();
	}

	void process(const ProcessArgs& args) override {
		// Update parameters
		fx_fader_set_level(fader, params[LEVEL_PARAM].getValue());

		// Get input
		float left = inputs[AUDIO_L_INPUT].getVoltage() / 5.f;
		float right = inputs[AUDIO_R_INPUT].isConnected() ?
		              inputs[AUDIO_R_INPUT].getVoltage() / 5.f : left;

		// Process
		fx_fader_process_frame(fader, &left, &right, sampleRate);

		// Set output
		outputs[AUDIO_L_OUTPUT].setVoltage(left * 5.f);
		outputs[AUDIO_R_OUTPUT].setVoltage(right * 5.f);
	}
};

struct RDJ_FaderWidget : ModuleWidget {
	RDJ_FaderWidget(RDJ_Fader* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RDJ_Fader.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title label
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(30.48, 5));
		titleLabel->text = "Fader";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Fader slider (centered vertically between title and I/O section)
		// Available space: from 13mm (top bar end) to 106mm (separator) = 93mm
		// Default slider: 12mm wide x 80mm tall
		// Center vertically: 13 + (93/2) = 59.5mm
		addParam(createParamCentered<RegrooveSlider>(mm2px(Vec(15.24, 59.5)), module, RDJ_Fader::LEVEL_PARAM));

		// IN label
		RegrooveLabel* inLabel = new RegrooveLabel();
		inLabel->box.pos = mm2px(Vec(2, 106.5));
		inLabel->box.size = mm2px(Vec(8, 4));
		inLabel->text = "In";
		inLabel->fontSize = 8.0;
		inLabel->align = NVG_ALIGN_LEFT;
		addChild(inLabel);

		// Inputs
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(13.5, 109.0)), module, RDJ_Fader::AUDIO_L_INPUT));
		addInput(createInputCentered<RegroovePort>(mm2px(Vec(23.5, 109.0)), module, RDJ_Fader::AUDIO_R_INPUT));

		// OUT label
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(2, 115.5));
		outLabel->box.size = mm2px(Vec(8, 4));
		outLabel->text = "Out";
		outLabel->fontSize = 8.0;
		outLabel->align = NVG_ALIGN_LEFT;
		addChild(outLabel);

		// Outputs
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(13.5, 118.0)), module, RDJ_Fader::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(23.5, 118.0)), module, RDJ_Fader::AUDIO_R_OUTPUT));
	}
};

Model* modelRDJ_Fader = createModel<RDJ_Fader, RDJ_FaderWidget>("RDJ_Fader");
