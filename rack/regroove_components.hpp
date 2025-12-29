#pragma once
#include <rack.hpp>

/**
 * Regroove VCV Rack Components
 *
 * Shared UI components for all Regroove VCV Rack plugins
 * Matches the visual identity from web UI (svg-slider.js, pad-knob.js, fader-components.js)
 *
 * Brand colors:
 * - Primary Red: #CF1A37 (signature Regroove red)
 * - Knob Track: #2a2a2a (knob outer body)
 * - Knob Cap: #555555 (knob center cap)
 * - Background: #0a0a0a (BLACK panels)
 * - Secondary BG: #1a1a1a (secondary panels)
 * - Text: #aaa (light gray text)
 *
 * Documentation: ../../groovy/STYLE_DESIGN_SYSTEM.md
 *
 * Usage:
 *   #include "../../regroove_components.hpp"
 */

using namespace rack;

// Regroove Brand Colors - EXACT match to VST3 + Web UI
// VST3: /plugins/DearImGuiKnobs/imgui-knobs.cpp lines 235-257
// Web:  /web/index.html lines 17-27
#define REGROOVE_RED nvgRGB(0xCF, 0x1A, 0x37)      // #CF1A37 (207, 26, 55) - Signature red tick
#define REGROOVE_TRACK nvgRGB(0x2a, 0x2a, 0x2a)    // #2A2A2A (42, 42, 42)  - Knob outer body
#define REGROOVE_CAP nvgRGB(0x55, 0x55, 0x55)      // #555555 (85, 85, 85)  - Knob center cap
#define REGROOVE_BG nvgRGB(0x0a, 0x0a, 0x0a)       // #0A0A0A (10, 10, 10)  - Panel background (BLACK)
#define REGROOVE_BG_SECONDARY nvgRGB(0x1a, 0x1a, 0x1a)  // #1A1A1A - Secondary panels
#define REGROOVE_TEXT nvgRGB(0xaa, 0xaa, 0xaa)     // #AAAAAA - Text labels

/**
 * Regroove Knob - Matches web UI aesthetic
 * - Dark circular track (#2a2a2a)
 * - Red indicator line (#CF1A37)
 * - Minimalist DJ-style design
 * - 60x60 size
 */
struct RegrooveKnob : app::SvgKnob {
	widget::FramebufferWidget* fb;
	CircularShadow* shadow;
	widget::TransformWidget* tw;
	widget::SvgWidget* sw;

	RegrooveKnob() {
		minAngle = -0.75 * M_PI;
		maxAngle = 0.75 * M_PI;

		// Shadow
		shadow = new CircularShadow;
		shadow->box.size = math::Vec(60, 60);
		shadow->opacity = 0.15;
		addChild(shadow);

		// Background framebuffer for smooth rendering
		fb = new widget::FramebufferWidget;
		addChild(fb);

		// Transform for rotation
		tw = new widget::TransformWidget;
		fb->addChild(tw);

		box.size = math::Vec(60, 60);
	}

	void draw(const DrawArgs& args) override {
		// EXACT match to VST3 ImGuiKnobVariant_Tick (imgui-knobs.cpp lines 235-257)
		float radius = box.size.x / 2;
		float cx = box.size.x / 2;
		float cy = box.size.y / 2;

		// 1. OUTER BODY - Dark gray #2A2A2A (0.85 * radius)
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cx, cy, 0.85f * radius);
		nvgFillColor(args.vg, REGROOVE_TRACK);
		nvgFill(args.vg);

		// 2. CENTER CAP - Gray #555555 (0.40 * radius)
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cx, cy, 0.40f * radius);
		nvgFillColor(args.vg, REGROOVE_CAP);
		nvgFill(args.vg);

		// 3. RED TICK LINE - #CF1A37 (from 0.42 to 0.85 * radius)
		float angle = 0.0;
		if (getParamQuantity()) {
			angle = math::rescale(getParamQuantity()->getValue(),
			                      getParamQuantity()->getMinValue(),
			                      getParamQuantity()->getMaxValue(),
			                      minAngle, maxAngle);
		}

		float tick_start = 0.42f * radius;
		float tick_end = 0.85f * radius;
		float tick_width = 0.08f * radius;

		nvgSave(args.vg);
		nvgTranslate(args.vg, cx, cy);
		nvgRotate(args.vg, angle);

		nvgBeginPath(args.vg);
		nvgRect(args.vg, -tick_width / 2, -tick_end, tick_width, tick_end - tick_start);
		nvgFillColor(args.vg, REGROOVE_RED);
		nvgFill(args.vg);

		nvgRestore(args.vg);

		Widget::draw(args);
	}
};

/**
 * Regroove Vertical Slider - Matches svg-slider.js
 * - Dark track (#2a2a2a)
 * - Red thumb (#CF1A37)
 * - Touch-friendly design
 * - Default: 60x380
 */
struct RegrooveSlider : app::SliderKnob {
	RegrooveSlider() {
		box.size = math::Vec(60, 380);
		horizontal = false;
	}

	void draw(const DrawArgs& args) override {
		// Draw track
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 4);
		nvgFillColor(args.vg, REGROOVE_TRACK);
		nvgFill(args.vg);

		// Calculate thumb position
		float value = 0.5;
		if (getParamQuantity()) {
			value = getParamQuantity()->getScaledValue();
		}
		float thumbHeight = 100;
		float thumbY = (1.0 - value) * (box.size.y - thumbHeight);

		// Draw thumb
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, thumbY, box.size.x, thumbHeight, 2);
		nvgFillColor(args.vg, REGROOVE_RED);
		nvgFill(args.vg);

		Widget::draw(args);
	}
};

/**
 * Regroove Small Knob - For compact controls
 * Smaller version of RegrooveKnob (40x40)
 */
struct RegrooveSmallKnob : RegrooveKnob {
	RegrooveSmallKnob() {
		box.size = math::Vec(40, 40);
		shadow->box.size = box.size;
	}

	void draw(const DrawArgs& args) override {
		// EXACT match to VST3 ImGuiKnobVariant_Tick (scaled to 40x40)
		float radius = box.size.x / 2;
		float cx = box.size.x / 2;
		float cy = box.size.y / 2;

		// 1. OUTER BODY - Dark gray #2A2A2A
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cx, cy, 0.85f * radius);
		nvgFillColor(args.vg, REGROOVE_TRACK);
		nvgFill(args.vg);

		// 2. CENTER CAP - Gray #555555
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cx, cy, 0.40f * radius);
		nvgFillColor(args.vg, REGROOVE_CAP);
		nvgFill(args.vg);

		// 3. RED TICK LINE - #CF1A37
		float angle = 0.0;
		if (getParamQuantity()) {
			angle = math::rescale(getParamQuantity()->getValue(),
			                      getParamQuantity()->getMinValue(),
			                      getParamQuantity()->getMaxValue(),
			                      minAngle, maxAngle);
		}

		float tick_start = 0.42f * radius;
		float tick_end = 0.85f * radius;
		float tick_width = 0.08f * radius;

		nvgSave(args.vg);
		nvgTranslate(args.vg, cx, cy);
		nvgRotate(args.vg, angle);

		nvgBeginPath(args.vg);
		nvgRect(args.vg, -tick_width / 2, -tick_end, tick_width, tick_end - tick_start);
		nvgFillColor(args.vg, REGROOVE_RED);
		nvgFill(args.vg);

		nvgRestore(args.vg);

		Widget::draw(args);
	}
};

/**
 * Regroove Port - Custom jack with red accent
 */
struct RegroovePort : app::SvgPort {
	RegroovePort() {
		setSvg(Svg::load(asset::system("res/ComponentLibrary/PJ301M.svg")));
	}

	void draw(const DrawArgs& args) override {
		// Draw port circle
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, 12);
		nvgFillColor(args.vg, REGROOVE_TRACK);
		nvgFill(args.vg);

		// Draw inner ring
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, 8);
		nvgStrokeColor(args.vg, REGROOVE_RED);
		nvgStrokeWidth(args.vg, 1.5);
		nvgStroke(args.vg);

		SvgPort::draw(args);
	}
};

/**
 * Regroove Label - Styled text matching web UI
 */
struct RegrooveLabel : widget::Widget {
	std::string text;
	float fontSize;
	NVGcolor color;
	bool bold;

	RegrooveLabel() {
		fontSize = 12.0;
		color = REGROOVE_TEXT;
		bold = false;
	}

	void draw(const DrawArgs& args) override {
		nvgFontSize(args.vg, fontSize);
		nvgFontFaceId(args.vg, APP->window->uiFont->handle);
		nvgTextLetterSpacing(args.vg, 0.5);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, color);

		if (bold) {
			// Simulate bold by rendering text multiple times with slight offsets
			nvgText(args.vg, box.size.x / 2 - 0.5, box.size.y / 2, text.c_str(), NULL);
			nvgText(args.vg, box.size.x / 2 + 0.5, box.size.y / 2, text.c_str(), NULL);
			nvgText(args.vg, box.size.x / 2, box.size.y / 2 - 0.5, text.c_str(), NULL);
			nvgText(args.vg, box.size.x / 2, box.size.y / 2 + 0.5, text.c_str(), NULL);
		}
		nvgText(args.vg, box.size.x / 2, box.size.y / 2, text.c_str(), NULL);

		Widget::draw(args);
	}
};
