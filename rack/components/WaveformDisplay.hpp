/**
 * Waveform Display Widget - VCV Rack Component
 *
 * Renders waveform using core logic from common/audio_viz/waveform.h
 * This is the VCV Rack-specific rendering layer using NanoVG
 *
 * Usage in module:
 *   // In module struct:
 *   WaveformDisplay waveform;
 *
 *   // In module constructor:
 *   waveform_init(&waveform, 48000 * 5, WAVEFORM_CHANNEL_STEREO, APP->engine->getSampleRate());
 *
 *   // In module process():
 *   waveform_write_stereo(&waveform, buffer, frames);
 *
 *   // In ModuleWidget:
 *   auto* wf_display = new WaveformDisplayWidget(&module->waveform);
 *   wf_display->box.pos = mm2px(Vec(5, 20));
 *   wf_display->box.size = mm2px(Vec(60, 30));
 *   addChild(wf_display);
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#pragma once
#include <rack.hpp>

extern "C" {
#include "../../common/audio_viz/waveform.h"
}

using namespace rack;

// ============================================================================
// Waveform Display Styles
// ============================================================================

enum WaveformDisplayStyle {
    WAVEFORM_STYLE_OSCILLOSCOPE,    // Classic scope trace
    WAVEFORM_STYLE_FILLED,          // Filled waveform
    WAVEFORM_STYLE_STEREO           // Split L/R
};

enum WaveformColorScheme {
    WAVEFORM_COLOR_GREEN_SCOPE,     // Classic green oscilloscope
    WAVEFORM_COLOR_BLUE_DIGITAL,    // Modern blue DAW
    WAVEFORM_COLOR_REGROOVE,        // Regroove red
    WAVEFORM_COLOR_RETRO_AMBER      // Amber CRT
};

// ============================================================================
// Waveform Display Widget
// ============================================================================

/**
 * WaveformDisplayWidget - Renders waveform in VCV Rack
 */
struct WaveformDisplayWidget : TransparentWidget {
    WaveformDisplay* waveform;
    WaveformDisplayStyle style;
    WaveformColorScheme color_scheme;
    bool show_grid;

    WaveformDisplayWidget(WaveformDisplay* wf,
                         WaveformDisplayStyle s = WAVEFORM_STYLE_OSCILLOSCOPE,
                         WaveformColorScheme cs = WAVEFORM_COLOR_GREEN_SCOPE)
        : waveform(wf), style(s), color_scheme(cs), show_grid(true) {
    }

    /**
     * Get waveform color based on scheme
     */
    NVGcolor getWaveformColor(int channel = 0) {
        switch (color_scheme) {
            case WAVEFORM_COLOR_GREEN_SCOPE:
                return nvgRGB(0, 255, 0);

            case WAVEFORM_COLOR_BLUE_DIGITAL:
                return nvgRGB(50, 150, 255);

            case WAVEFORM_COLOR_REGROOVE:
                return nvgRGB(0xCF, 0x1A, 0x37);  // Regroove red

            case WAVEFORM_COLOR_RETRO_AMBER:
                return nvgRGB(255, 180, 0);

            default:
                return nvgRGB(0, 255, 0);
        }
    }

    /**
     * Get background color
     */
    NVGcolor getBackgroundColor() {
        switch (color_scheme) {
            case WAVEFORM_COLOR_RETRO_AMBER:
                return nvgRGB(20, 15, 10);  // Dark brownish
            default:
                return nvgRGB(10, 10, 10);  // Very dark gray
        }
    }

    /**
     * Draw oscilloscope style
     */
    void drawOscilloscope(const DrawArgs& args) {
        if (!waveform) return;

        const float w = box.size.x;
        const float h = box.size.y;
        const float centerY = h * 0.5f;
        const float amp = (h * 0.45f) * waveform->amplitude_scale;

        // Background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgFillColor(args.vg, getBackgroundColor());
        nvgFill(args.vg);

        // Grid lines
        if (show_grid) {
            // Horizontal center line (0dB)
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, centerY);
            nvgLineTo(args.vg, w, centerY);
            nvgStrokeColor(args.vg, nvgRGBA(50, 50, 50, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);

            // +/-1.0 lines
            float line_top = h * 0.05f;
            float line_bottom = h * 0.95f;

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, line_top);
            nvgLineTo(args.vg, w, line_top);
            nvgMoveTo(args.vg, 0, line_bottom);
            nvgLineTo(args.vg, w, line_bottom);
            nvgStrokeColor(args.vg, nvgRGBA(40, 40, 40, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);

            // Vertical time divisions (10 divisions)
            nvgBeginPath(args.vg);
            for (int i = 1; i < 10; i++) {
                float x = (w / 10.0f) * i;
                nvgMoveTo(args.vg, x, 0);
                nvgLineTo(args.vg, x, h);
            }
            nvgStrokeColor(args.vg, nvgRGBA(30, 30, 30, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);
        }

        // Get visible range
        uint32_t start_sample, visible_samples;
        waveform_get_visible_range(waveform, &start_sample, &visible_samples);

        if (visible_samples == 0 || waveform->sample_count == 0) {
            // No data - show flatline
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, centerY);
            nvgLineTo(args.vg, w, centerY);
            nvgStrokeColor(args.vg, getWaveformColor(0));
            nvgStrokeWidth(args.vg, 1.5f);
            nvgStroke(args.vg);
            return;
        }

        // Determine rendering mode
        uint32_t pixels = (uint32_t)w;
        bool use_envelope = (visible_samples / pixels) > 4;

        NVGcolor trace_color = getWaveformColor(0);

        if (use_envelope) {
            // Envelope mode: draw min/max for each pixel column
            nvgBeginPath(args.vg);
            for (uint32_t x = 0; x < pixels; x++) {
                uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
                uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

                if (idx2 > start_sample + visible_samples) {
                    idx2 = start_sample + visible_samples;
                }

                float min_val, max_val;
                waveform_get_envelope(waveform, idx1, idx2, 0, &min_val, &max_val);

                // Clamp
                if (min_val < -1.0f) min_val = -1.0f;
                if (max_val > 1.0f) max_val = 1.0f;

                float y_min = centerY - (max_val * amp);
                float y_max = centerY - (min_val * amp);

                nvgMoveTo(args.vg, x, y_min);
                nvgLineTo(args.vg, x, y_max);
            }
            nvgStrokeColor(args.vg, trace_color);
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);

        } else {
            // Sample-accurate mode: connect samples
            nvgBeginPath(args.vg);
            bool first_point = true;

            for (uint32_t x = 0; x < pixels; x++) {
                uint32_t idx = start_sample + (x * visible_samples) / pixels;

                if (idx >= waveform->sample_count) {
                    idx = waveform->sample_count - 1;
                }

                float sample = waveform_get_sample(waveform, idx, 0);

                // Clamp
                if (sample < -1.0f) sample = -1.0f;
                if (sample > 1.0f) sample = 1.0f;

                float y = centerY - (sample * amp);

                if (first_point) {
                    nvgMoveTo(args.vg, x, y);
                    first_point = false;
                } else {
                    nvgLineTo(args.vg, x, y);
                }
            }

            nvgStrokeColor(args.vg, trace_color);
            nvgStrokeWidth(args.vg, 1.5f);
            nvgStroke(args.vg);
        }

        // Border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgStrokeColor(args.vg, nvgRGB(80, 80, 80));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);
    }

    /**
     * Draw filled style
     */
    void drawFilled(const DrawArgs& args) {
        if (!waveform) return;

        const float w = box.size.x;
        const float h = box.size.y;
        const float centerY = h * 0.5f;
        const float amp = (h * 0.45f) * waveform->amplitude_scale;

        // Background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgFillColor(args.vg, getBackgroundColor());
        nvgFill(args.vg);

        // Center line
        if (show_grid) {
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, centerY);
            nvgLineTo(args.vg, w, centerY);
            nvgStrokeColor(args.vg, nvgRGBA(50, 50, 50, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);
        }

        // Get visible range
        uint32_t start_sample, visible_samples;
        waveform_get_visible_range(waveform, &start_sample, &visible_samples);

        if (visible_samples == 0 || waveform->sample_count == 0) {
            return;
        }

        uint32_t pixels = (uint32_t)w;
        NVGcolor fill_color = getWaveformColor(0);

        // Draw filled waveform
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, centerY);

        for (uint32_t x = 0; x < pixels; x++) {
            uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
            uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

            if (idx2 > start_sample + visible_samples) {
                idx2 = start_sample + visible_samples;
            }

            float min_val, max_val;
            waveform_get_envelope(waveform, idx1, idx2, 0, &min_val, &max_val);

            // Clamp
            if (min_val < -1.0f) min_val = -1.0f;
            if (max_val > 1.0f) max_val = 1.0f;

            float y_top = centerY - (max_val * amp);
            float y_bottom = centerY - (min_val * amp);

            // Create filled rectangle for this column
            nvgRect(args.vg, x, y_top, 1, y_bottom - y_top);
        }

        nvgFillColor(args.vg, fill_color);
        nvgFill(args.vg);

        // Border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgStrokeColor(args.vg, nvgRGB(80, 80, 80));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);
    }

    /**
     * Draw stereo split style
     */
    void drawStereo(const DrawArgs& args) {
        if (!waveform || waveform->channel_mode != WAVEFORM_CHANNEL_STEREO) {
            drawOscilloscope(args);
            return;
        }

        const float w = box.size.x;
        const float h = box.size.y;
        const float halfH = h * 0.5f;
        const float amp = (halfH * 0.8f) * waveform->amplitude_scale;

        // Background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgFillColor(args.vg, getBackgroundColor());
        nvgFill(args.vg);

        // Center divider
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, halfH);
        nvgLineTo(args.vg, w, halfH);
        nvgStrokeColor(args.vg, nvgRGB(80, 80, 80));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);

        // Grid
        if (show_grid) {
            nvgBeginPath(args.vg);
            for (int i = 1; i < 10; i++) {
                float x = (w / 10.0f) * i;
                nvgMoveTo(args.vg, x, 0);
                nvgLineTo(args.vg, x, h);
            }
            nvgStrokeColor(args.vg, nvgRGBA(30, 30, 30, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);
        }

        // Get visible range
        uint32_t start_sample, visible_samples;
        waveform_get_visible_range(waveform, &start_sample, &visible_samples);

        if (visible_samples == 0 || waveform->sample_count == 0) {
            return;
        }

        uint32_t pixels = (uint32_t)w;
        bool use_envelope = (visible_samples / pixels) > 4;

        // === LEFT CHANNEL (top half) ===
        const float leftCenterY = halfH * 0.5f;
        NVGcolor leftColor = getWaveformColor(0);

        nvgBeginPath(args.vg);
        bool first_left = true;

        for (uint32_t x = 0; x < pixels; x++) {
            uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
            uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

            if (use_envelope) {
                float min_val, max_val;
                waveform_get_envelope(waveform, idx1, idx2, 0, &min_val, &max_val);
                if (min_val < -1.0f) min_val = -1.0f;
                if (max_val > 1.0f) max_val = 1.0f;

                float y_min = leftCenterY - (max_val * amp);
                float y_max = leftCenterY - (min_val * amp);

                nvgMoveTo(args.vg, x, y_min);
                nvgLineTo(args.vg, x, y_max);
            } else {
                float sample = waveform_get_sample(waveform, idx1, 0);
                float y = leftCenterY - (sample * amp);

                if (first_left) {
                    nvgMoveTo(args.vg, x, y);
                    first_left = false;
                } else {
                    nvgLineTo(args.vg, x, y);
                }
            }
        }

        nvgStrokeColor(args.vg, leftColor);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);

        // === RIGHT CHANNEL (bottom half) ===
        const float rightCenterY = halfH + halfH * 0.5f;
        NVGcolor rightColor = nvgRGB(255, 150, 100);  // Orange for right

        nvgBeginPath(args.vg);
        bool first_right = true;

        for (uint32_t x = 0; x < pixels; x++) {
            uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
            uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

            if (use_envelope) {
                float min_val, max_val;
                waveform_get_envelope(waveform, idx1, idx2, 1, &min_val, &max_val);
                if (min_val < -1.0f) min_val = -1.0f;
                if (max_val > 1.0f) max_val = 1.0f;

                float y_min = rightCenterY - (max_val * amp);
                float y_max = rightCenterY - (min_val * amp);

                nvgMoveTo(args.vg, x, y_min);
                nvgLineTo(args.vg, x, y_max);
            } else {
                float sample = waveform_get_sample(waveform, idx1, 1);
                float y = rightCenterY - (sample * amp);

                if (first_right) {
                    nvgMoveTo(args.vg, x, y);
                    first_right = false;
                } else {
                    nvgLineTo(args.vg, x, y);
                }
            }
        }

        nvgStrokeColor(args.vg, rightColor);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);

        // Labels
        nvgFontSize(args.vg, 10);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFillColor(args.vg, nvgRGB(150, 150, 150));
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgText(args.vg, 5, 5, "L", NULL);
        nvgText(args.vg, 5, halfH + 5, "R", NULL);

        // Border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgStrokeColor(args.vg, nvgRGB(80, 80, 80));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);
    }

    void draw(const DrawArgs& args) override {
        if (!waveform) {
            // No waveform - draw placeholder
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            nvgFillColor(args.vg, nvgRGB(20, 20, 20));
            nvgFill(args.vg);
            return;
        }

        switch (style) {
            case WAVEFORM_STYLE_OSCILLOSCOPE:
                drawOscilloscope(args);
                break;
            case WAVEFORM_STYLE_FILLED:
                drawFilled(args);
                break;
            case WAVEFORM_STYLE_STEREO:
                drawStereo(args);
                break;
        }
    }
};
