/**
 * VU Meter Widget - VCV Rack Component
 *
 * Renders VU meter using core logic from common/audio_viz/vu_meter.h
 * This is the VCV Rack-specific rendering layer using NanoVG
 *
 * Usage in module:
 *   // In module struct:
 *   VUMeter vu_meter;
 *
 *   // In module constructor:
 *   vu_meter_init(&vu_meter, APP->engine->getSampleRate(), VU_MODE_PEAK);
 *
 *   // In module process():
 *   vu_meter_process(&vu_meter, left_sample, right_sample);
 *
 *   // In ModuleWidget:
 *   addChild(new VUMeterDisplay(&module->vu_meter));
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#pragma once
#include <rack.hpp>

extern "C" {
#include "../../common/audio_viz/vu_meter.h"
}

using namespace rack;

// ============================================================================
// VU Meter Display Styles
// ============================================================================

enum VUMeterStyle {
    VU_STYLE_VERTICAL,      // Traditional vertical bars
    VU_STYLE_HORIZONTAL,    // Horizontal bars
    VU_STYLE_COMBINED       // Compact retro (center-split)
};

enum VUMeterColorScheme {
    VU_COLOR_STANDARD,      // Green/yellow/red
    VU_COLOR_REGROOVE,      // Regroove red theme
    VU_COLOR_BLUE,          // Blue digital
    VU_COLOR_RETRO          // Amber/orange
};

// ============================================================================
// VU Meter Display Widget
// ============================================================================

/**
 * VUMeterDisplay - Renders VU meter in VCV Rack
 */
struct VUMeterDisplay : TransparentWidget {
    VUMeter* vu_meter;
    VUMeterStyle style;
    VUMeterColorScheme color_scheme;
    bool show_peak_hold;
    bool show_db_scale;

    VUMeterDisplay(VUMeter* vu, VUMeterStyle s = VU_STYLE_VERTICAL,
                   VUMeterColorScheme cs = VU_COLOR_STANDARD)
        : vu_meter(vu), style(s), color_scheme(cs),
          show_peak_hold(true), show_db_scale(false) {
    }

    /**
     * Get color for dB level based on scheme
     */
    NVGcolor getColorForLevel(float db) {
        switch (color_scheme) {
            case VU_COLOR_STANDARD:
                if (db >= -6.0f) return nvgRGB(255, 0, 0);      // Red (hot)
                if (db >= -18.0f) return nvgRGB(255, 200, 0);   // Yellow (warn)
                return nvgRGB(0, 255, 0);                       // Green (good)

            case VU_COLOR_REGROOVE:
                return nvgRGB(0xCF, 0x1A, 0x37);  // Regroove red

            case VU_COLOR_BLUE:
                return nvgRGB(50, 150, 255);

            case VU_COLOR_RETRO:
                return nvgRGB(255, 180, 0);  // Amber

            default:
                return nvgRGB(0, 255, 0);
        }
    }

    /**
     * Get gradient color (interpolate based on normalized level)
     */
    NVGcolor getGradientColor(float normalized) {
        if (color_scheme != VU_COLOR_STANDARD) {
            return getColorForLevel(-6.0f);  // Just use solid color
        }

        if (normalized >= 0.8f) return nvgRGB(255, 0, 0);  // Red

        if (normalized >= 0.6f) {
            // Interpolate yellow to red
            float t = (normalized - 0.6f) / 0.2f;
            return nvgRGB(255, (int)(200 * (1.0f - t)), 0);
        }

        if (normalized >= 0.4f) {
            // Interpolate green to yellow
            float t = (normalized - 0.4f) / 0.2f;
            return nvgRGB((int)(255 * t), 255, 0);
        }

        return nvgRGB(0, 255, 0);  // Green
    }

    /**
     * Draw vertical VU meter
     */
    void drawVertical(const DrawArgs& args) {
        if (!vu_meter) return;

        const float w = box.size.x;
        const float h = box.size.y;
        const float channel_width = (w - 4.0f) / 2.0f;
        const float bar_height = h - 20.0f;

        // Background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);

        // === LEFT CHANNEL ===
        float left_db = vu_meter->peak_left_db;
        float left_norm = vu_meter_get_normalized(left_db);
        float left_height = left_norm * bar_height;

        // Draw gradient bar
        if (left_height > 1.0f) {
            for (float y = 0; y < left_height; y += 2.0f) {
                float segment_norm = (bar_height - y) / bar_height;
                NVGcolor color = getGradientColor(segment_norm);

                nvgBeginPath(args.vg);
                nvgRect(args.vg, 2, bar_height - y - 2.0f, channel_width, 2.0f);
                nvgFillColor(args.vg, color);
                nvgFill(args.vg);
            }
        }

        // Peak hold line (left)
        if (show_peak_hold && vu_meter->peak_hold_left > 0.0f) {
            float hold_norm = vu_meter_get_normalized(vu_meter->peak_hold_left_db);
            float hold_y = bar_height - (hold_norm * bar_height);

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 2, hold_y);
            nvgLineTo(args.vg, 2 + channel_width, hold_y);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(args.vg, 2.0f);
            nvgStroke(args.vg);
        }

        // === RIGHT CHANNEL ===
        float right_db = vu_meter->peak_right_db;
        float right_norm = vu_meter_get_normalized(right_db);
        float right_height = right_norm * bar_height;

        if (right_height > 1.0f) {
            for (float y = 0; y < right_height; y += 2.0f) {
                float segment_norm = (bar_height - y) / bar_height;
                NVGcolor color = getGradientColor(segment_norm);

                nvgBeginPath(args.vg);
                nvgRect(args.vg, channel_width + 4, bar_height - y - 2.0f, channel_width, 2.0f);
                nvgFillColor(args.vg, color);
                nvgFill(args.vg);
            }
        }

        // Peak hold line (right)
        if (show_peak_hold && vu_meter->peak_hold_right > 0.0f) {
            float hold_norm = vu_meter_get_normalized(vu_meter->peak_hold_right_db);
            float hold_y = bar_height - (hold_norm * bar_height);

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, channel_width + 4, hold_y);
            nvgLineTo(args.vg, w - 2, hold_y);
            nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(args.vg, 2.0f);
            nvgStroke(args.vg);
        }

        // Channel labels
        nvgFontSize(args.vg, 10);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFillColor(args.vg, nvgRGB(200, 200, 200));
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgText(args.vg, 5, bar_height + 5, "L", NULL);
        nvgText(args.vg, channel_width + 10, bar_height + 5, "R", NULL);
    }

    /**
     * Draw horizontal VU meter
     */
    void drawHorizontal(const DrawArgs& args) {
        if (!vu_meter) return;

        const float w = box.size.x;
        const float h = box.size.y;
        const float channel_height = (h - 4.0f) / 2.0f;
        const float bar_width = w - 40.0f;

        // Background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);

        // === LEFT CHANNEL (top) ===
        float left_norm = vu_meter_get_normalized(vu_meter->peak_left_db);
        float left_width = left_norm * bar_width;

        if (left_width > 1.0f) {
            for (float x = 0; x < left_width; x += 2.0f) {
                float segment_norm = x / bar_width;
                NVGcolor color = getGradientColor(segment_norm);

                nvgBeginPath(args.vg);
                nvgRect(args.vg, 20 + x, 2, 2.0f, channel_height);
                nvgFillColor(args.vg, color);
                nvgFill(args.vg);
            }
        }

        // === RIGHT CHANNEL (bottom) ===
        float right_norm = vu_meter_get_normalized(vu_meter->peak_right_db);
        float right_width = right_norm * bar_width;

        if (right_width > 1.0f) {
            for (float x = 0; x < right_width; x += 2.0f) {
                float segment_norm = x / bar_width;
                NVGcolor color = getGradientColor(segment_norm);

                nvgBeginPath(args.vg);
                nvgRect(args.vg, 20 + x, channel_height + 4, 2.0f, channel_height);
                nvgFillColor(args.vg, color);
                nvgFill(args.vg);
            }
        }

        // Labels
        nvgFontSize(args.vg, 10);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFillColor(args.vg, nvgRGB(200, 200, 200));
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(args.vg, 2, channel_height * 0.5f, "L", NULL);
        nvgText(args.vg, 2, channel_height * 1.5f + 4, "R", NULL);
    }

    /**
     * Draw combined (retro compact) VU meter
     */
    void drawCombined(const DrawArgs& args) {
        if (!vu_meter) return;

        const float w = box.size.x;
        const float h = box.size.y;
        const float bar_height = h - 15.0f;
        const float center_gap = 4.0f;
        const float half_width = (w - center_gap) / 2.0f;

        // Background (hardware chassis look)
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, w, h, 3.0f);
        nvgFillColor(args.vg, nvgRGB(25, 25, 25));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, w, h, 3.0f);
        nvgStrokeColor(args.vg, nvgRGB(60, 60, 60));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);

        // === LEFT CHANNEL (grows from center to left) ===
        float left_norm = vu_meter_get_normalized(vu_meter->peak_left_db);
        float left_width = left_norm * half_width;

        if (left_width > 1.0f) {
            const int num_segments = 20;
            const float segment_height = bar_height / num_segments;

            for (int i = 0; i < num_segments; i++) {
                float segment_norm = (float)i / num_segments;
                if (segment_norm <= left_norm) {
                    NVGcolor color = getGradientColor(segment_norm);

                    nvgBeginPath(args.vg);
                    nvgRoundedRect(args.vg,
                                  half_width - left_width + 2,
                                  5 + (num_segments - i - 1) * segment_height + 1,
                                  left_width - 3,
                                  segment_height - 2,
                                  1.0f);
                    nvgFillColor(args.vg, color);
                    nvgFill(args.vg);
                }
            }
        }

        // === RIGHT CHANNEL (grows from center to right) ===
        float right_norm = vu_meter_get_normalized(vu_meter->peak_right_db);
        float right_width = right_norm * half_width;

        if (right_width > 1.0f) {
            const int num_segments = 20;
            const float segment_height = bar_height / num_segments;

            for (int i = 0; i < num_segments; i++) {
                float segment_norm = (float)i / num_segments;
                if (segment_norm <= right_norm) {
                    NVGcolor color = getGradientColor(segment_norm);

                    nvgBeginPath(args.vg);
                    nvgRoundedRect(args.vg,
                                  half_width + center_gap + 1,
                                  5 + (num_segments - i - 1) * segment_height + 1,
                                  right_width - 2,
                                  segment_height - 2,
                                  1.0f);
                    nvgFillColor(args.vg, color);
                    nvgFill(args.vg);
                }
            }
        }

        // Center divider
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, half_width, 5);
        nvgLineTo(args.vg, half_width, 5 + bar_height);
        nvgStrokeColor(args.vg, nvgRGB(80, 80, 80));
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStroke(args.vg);

        // Peak hold indicators
        if (show_peak_hold) {
            if (vu_meter->peak_hold_left > 0.0f) {
                float hold_norm = vu_meter_get_normalized(vu_meter->peak_hold_left_db);
                float hold_width = hold_norm * half_width;
                float hold_x = half_width - hold_width;

                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, hold_x, 5);
                nvgLineTo(args.vg, hold_x, 5 + bar_height);
                nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
                nvgStrokeWidth(args.vg, 1.5f);
                nvgStroke(args.vg);
            }

            if (vu_meter->peak_hold_right > 0.0f) {
                float hold_norm = vu_meter_get_normalized(vu_meter->peak_hold_right_db);
                float hold_width = hold_norm * half_width;
                float hold_x = half_width + center_gap + hold_width;

                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, hold_x, 5);
                nvgLineTo(args.vg, hold_x, 5 + bar_height);
                nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
                nvgStrokeWidth(args.vg, 1.5f);
                nvgStroke(args.vg);
            }
        }

        // Channel labels
        nvgFontSize(args.vg, 9);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFillColor(args.vg, nvgRGB(180, 180, 180));
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgText(args.vg, 5, bar_height + 8, "L", NULL);
        nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
        nvgText(args.vg, w - 5, bar_height + 8, "R", NULL);
    }

    void draw(const DrawArgs& args) override {
        if (!vu_meter) {
            // No meter - draw placeholder
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            nvgFillColor(args.vg, nvgRGB(20, 20, 20));
            nvgFill(args.vg);
            return;
        }

        switch (style) {
            case VU_STYLE_VERTICAL:
                drawVertical(args);
                break;
            case VU_STYLE_HORIZONTAL:
                drawHorizontal(args);
                break;
            case VU_STYLE_COMBINED:
                drawCombined(args);
                break;
        }
    }
};
