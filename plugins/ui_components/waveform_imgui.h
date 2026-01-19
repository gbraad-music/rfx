/**
 * Waveform Display - ImGui Rendering Wrapper
 *
 * Renders waveform using core logic from common/audio_viz/waveform.h
 * This is the ImGui-specific rendering layer
 *
 * Usage in plugin:
 *   // In plugin state:
 *   WaveformDisplay waveform;
 *
 *   // In init:
 *   waveform_init(&waveform, 48000, WAVEFORM_CHANNEL_STEREO, 48000);
 *
 *   // In audio process:
 *   waveform_write_stereo(&waveform, buffer, frames);
 *
 *   // In UI render:
 *   Waveform::renderWaveform(&waveform, ImVec2(600, 200), Waveform::STYLE_OSCILLOSCOPE);
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#ifndef WAVEFORM_IMGUI_H
#define WAVEFORM_IMGUI_H

#include "../../common/audio_viz/waveform.h"
#include "imgui.h"

namespace Waveform {

// ============================================================================
// Rendering Styles
// ============================================================================

enum Style {
    STYLE_OSCILLOSCOPE,     // Traditional scope trace (line)
    STYLE_FILLED,           // Filled waveform
    STYLE_ENVELOPE,         // Min/max envelope (efficient for zoomed out)
    STYLE_BARS              // Vertical bars (sample-accurate)
};

enum ColorScheme {
    COLOR_GREEN_SCOPE,      // Classic green oscilloscope
    COLOR_BLUE_DIGITAL,     // Modern blue DAW style
    COLOR_RETRO_AMBER,      // Amber CRT monitor
    COLOR_STEREO_LR         // Different colors for L/R
};

// ============================================================================
// Color Utilities
// ============================================================================

/**
 * Get waveform color for channel
 */
static inline ImU32 getWaveformColor(int channel, ColorScheme scheme = COLOR_GREEN_SCOPE) {
    switch (scheme) {
        case COLOR_GREEN_SCOPE:
            return IM_COL32(0, 255, 0, 255);

        case COLOR_BLUE_DIGITAL:
            return IM_COL32(50, 150, 255, 255);

        case COLOR_RETRO_AMBER:
            return IM_COL32(255, 180, 0, 255);

        case COLOR_STEREO_LR:
            return (channel == 0) ?
                   IM_COL32(100, 200, 255, 255) :  // Blue for left
                   IM_COL32(255, 150, 100, 255);   // Orange for right

        default:
            return IM_COL32(0, 255, 0, 255);
    }
}

/**
 * Get background color for scheme
 */
static inline ImU32 getBackgroundColor(ColorScheme scheme) {
    switch (scheme) {
        case COLOR_RETRO_AMBER:
            return IM_COL32(20, 15, 10, 255);  // Dark brownish
        default:
            return IM_COL32(10, 10, 10, 255);  // Very dark gray
    }
}

// ============================================================================
// Oscilloscope Style (Line Trace)
// ============================================================================

/**
 * Render traditional oscilloscope-style waveform
 *
 * @param wf Waveform core state
 * @param size Width and height of display
 * @param scheme Color scheme
 * @param show_grid Show time/amplitude grid
 */
static inline void renderOscilloscope(WaveformDisplay* wf, ImVec2 size,
                                     ColorScheme scheme = COLOR_GREEN_SCOPE,
                                     bool show_grid = true) {
    if (!wf) return;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    const float centerY = pos.y + size.y * 0.5f;
    const float amp = (size.y * 0.45f) * wf->amplitude_scale;

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                       getBackgroundColor(scheme));

    // Grid lines (if enabled)
    if (show_grid) {
        // Horizontal center line (0dB)
        draw->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY),
                     IM_COL32(50, 50, 50, 255), 1.0f);

        // +/-1.0 lines
        float line_pos_top = pos.y + size.y * 0.05f;
        float line_pos_bottom = pos.y + size.y * 0.95f;
        draw->AddLine(ImVec2(pos.x, line_pos_top), ImVec2(pos.x + size.x, line_pos_top),
                     IM_COL32(40, 40, 40, 255), 1.0f);
        draw->AddLine(ImVec2(pos.x, line_pos_bottom), ImVec2(pos.x + size.x, line_pos_bottom),
                     IM_COL32(40, 40, 40, 255), 1.0f);

        // Vertical time divisions (10 divisions)
        for (int i = 1; i < 10; i++) {
            float x = pos.x + (size.x / 10.0f) * i;
            draw->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y),
                         IM_COL32(30, 30, 30, 255), 1.0f);
        }
    }

    // Get visible range
    uint32_t start_sample, visible_samples;
    waveform_get_visible_range(wf, &start_sample, &visible_samples);

    if (visible_samples == 0 || wf->sample_count == 0) {
        // No data - show flatline
        draw->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY),
                     getWaveformColor(0, scheme), 1.5f);
        ImGui::Dummy(size);
        return;
    }

    // Determine rendering mode based on zoom
    uint32_t pixels = (uint32_t)size.x;
    bool use_envelope = (visible_samples / pixels) > 4;  // Use envelope if >4 samples per pixel

    // === RENDER LEFT/MONO CHANNEL ===
    ImU32 leftColor = getWaveformColor(0, scheme);

    if (use_envelope) {
        // Envelope mode: draw min/max for each pixel column
        for (uint32_t x = 0; x < pixels - 1; x++) {
            uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
            uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

            if (idx2 > start_sample + visible_samples) {
                idx2 = start_sample + visible_samples;
            }

            float min_val, max_val;
            waveform_get_envelope(wf, idx1, idx2, 0, &min_val, &max_val);

            // Clamp for safety
            if (min_val < -1.0f) min_val = -1.0f;
            if (max_val > 1.0f) max_val = 1.0f;

            float y_min = centerY - (max_val * amp);
            float y_max = centerY - (min_val * amp);

            // Draw vertical line from min to max
            draw->AddLine(ImVec2(pos.x + x, y_min), ImVec2(pos.x + x, y_max),
                         leftColor, 1.0f);
        }
    } else {
        // Sample-accurate mode: connect individual samples
        for (uint32_t x = 0; x < pixels - 1; x++) {
            uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
            uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

            if (idx1 >= wf->sample_count) idx1 = wf->sample_count - 1;
            if (idx2 >= wf->sample_count) idx2 = wf->sample_count - 1;

            float sample1 = waveform_get_sample(wf, idx1, 0);
            float sample2 = waveform_get_sample(wf, idx2, 0);

            // Clamp
            if (sample1 < -1.0f) sample1 = -1.0f;
            if (sample1 > 1.0f) sample1 = 1.0f;
            if (sample2 < -1.0f) sample2 = -1.0f;
            if (sample2 > 1.0f) sample2 = 1.0f;

            float y1 = centerY - (sample1 * amp);
            float y2 = centerY - (sample2 * amp);

            draw->AddLine(ImVec2(pos.x + x, y1), ImVec2(pos.x + x + 1, y2),
                         leftColor, 1.5f);
        }
    }

    // Border
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                 IM_COL32(80, 80, 80, 255), 0.0f, 0, 1.5f);

    // Reserve space
    ImGui::Dummy(size);
}

// ============================================================================
// Filled Style
// ============================================================================

/**
 * Render filled waveform (DAW-style)
 */
static inline void renderFilled(WaveformDisplay* wf, ImVec2 size,
                               ColorScheme scheme = COLOR_BLUE_DIGITAL,
                               bool show_grid = false) {
    if (!wf) return;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    const float centerY = pos.y + size.y * 0.5f;
    const float amp = (size.y * 0.45f) * wf->amplitude_scale;

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                       getBackgroundColor(scheme));

    // Grid
    if (show_grid) {
        draw->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY),
                     IM_COL32(50, 50, 50, 255), 1.0f);
    }

    // Get visible range
    uint32_t start_sample, visible_samples;
    waveform_get_visible_range(wf, &start_sample, &visible_samples);

    if (visible_samples == 0 || wf->sample_count == 0) {
        ImGui::Dummy(size);
        return;
    }

    uint32_t pixels = (uint32_t)size.x;
    ImU32 fillColor = getWaveformColor(0, scheme);

    // Draw filled waveform using envelope
    for (uint32_t x = 0; x < pixels; x++) {
        uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
        uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

        if (idx2 > start_sample + visible_samples) {
            idx2 = start_sample + visible_samples;
        }

        float min_val, max_val;
        waveform_get_envelope(wf, idx1, idx2, 0, &min_val, &max_val);

        // Clamp
        if (min_val < -1.0f) min_val = -1.0f;
        if (max_val > 1.0f) max_val = 1.0f;

        float y_top = centerY - (max_val * amp);
        float y_bottom = centerY - (min_val * amp);

        // Fill from center to edges with gradient alpha
        draw->AddRectFilled(
            ImVec2(pos.x + x, y_top),
            ImVec2(pos.x + x + 1, y_bottom),
            fillColor
        );
    }

    // Border
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                 IM_COL32(80, 80, 80, 255), 0.0f, 0, 1.5f);

    ImGui::Dummy(size);
}

// ============================================================================
// Stereo Style (Split L/R)
// ============================================================================

/**
 * Render stereo waveform (split top/bottom)
 */
static inline void renderStereo(WaveformDisplay* wf, ImVec2 size,
                               ColorScheme scheme = COLOR_STEREO_LR,
                               bool show_grid = true) {
    if (!wf || wf->channel_mode != WAVEFORM_CHANNEL_STEREO) {
        renderOscilloscope(wf, size, scheme, show_grid);
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    const float halfH = size.y * 0.5f;
    const float amp = (halfH * 0.8f) * wf->amplitude_scale;

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                       getBackgroundColor(scheme));

    // Center divider
    draw->AddLine(ImVec2(pos.x, pos.y + halfH), ImVec2(pos.x + size.x, pos.y + halfH),
                 IM_COL32(80, 80, 80, 255), 1.5f);

    // Grid
    if (show_grid) {
        for (int i = 1; i < 10; i++) {
            float x = pos.x + (size.x / 10.0f) * i;
            draw->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y),
                         IM_COL32(30, 30, 30, 255), 1.0f);
        }
    }

    // Get visible range
    uint32_t start_sample, visible_samples;
    waveform_get_visible_range(wf, &start_sample, &visible_samples);

    if (visible_samples == 0 || wf->sample_count == 0) {
        ImGui::Dummy(size);
        return;
    }

    uint32_t pixels = (uint32_t)size.x;
    bool use_envelope = (visible_samples / pixels) > 4;

    // === LEFT CHANNEL (top half) ===
    const float leftCenterY = pos.y + halfH * 0.5f;
    ImU32 leftColor = getWaveformColor(0, scheme);

    for (uint32_t x = 0; x < pixels - 1; x++) {
        uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
        uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

        if (use_envelope) {
            float min_val, max_val;
            waveform_get_envelope(wf, idx1, idx2, 0, &min_val, &max_val);
            if (min_val < -1.0f) min_val = -1.0f;
            if (max_val > 1.0f) max_val = 1.0f;

            float y_min = leftCenterY - (max_val * amp);
            float y_max = leftCenterY - (min_val * amp);
            draw->AddLine(ImVec2(pos.x + x, y_min), ImVec2(pos.x + x, y_max),
                         leftColor, 1.0f);
        } else {
            float sample1 = waveform_get_sample(wf, idx1, 0);
            float sample2 = waveform_get_sample(wf, idx2, 0);
            float y1 = leftCenterY - (sample1 * amp);
            float y2 = leftCenterY - (sample2 * amp);
            draw->AddLine(ImVec2(pos.x + x, y1), ImVec2(pos.x + x + 1, y2),
                         leftColor, 1.5f);
        }
    }

    // === RIGHT CHANNEL (bottom half) ===
    const float rightCenterY = pos.y + halfH + halfH * 0.5f;
    ImU32 rightColor = getWaveformColor(1, scheme);

    for (uint32_t x = 0; x < pixels - 1; x++) {
        uint32_t idx1 = start_sample + (x * visible_samples) / pixels;
        uint32_t idx2 = start_sample + ((x + 1) * visible_samples) / pixels;

        if (use_envelope) {
            float min_val, max_val;
            waveform_get_envelope(wf, idx1, idx2, 1, &min_val, &max_val);
            if (min_val < -1.0f) min_val = -1.0f;
            if (max_val > 1.0f) max_val = 1.0f;

            float y_min = rightCenterY - (max_val * amp);
            float y_max = rightCenterY - (min_val * amp);
            draw->AddLine(ImVec2(pos.x + x, y_min), ImVec2(pos.x + x, y_max),
                         rightColor, 1.0f);
        } else {
            float sample1 = waveform_get_sample(wf, idx1, 1);
            float sample2 = waveform_get_sample(wf, idx2, 1);
            float y1 = rightCenterY - (sample1 * amp);
            float y2 = rightCenterY - (sample2 * amp);
            draw->AddLine(ImVec2(pos.x + x, y1), ImVec2(pos.x + x + 1, y2),
                         rightColor, 1.5f);
        }
    }

    // Labels
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    draw->AddText(ImVec2(pos.x + 5, pos.y + 5), IM_COL32(150, 150, 150, 255), "L");
    draw->AddText(ImVec2(pos.x + 5, pos.y + halfH + 5), IM_COL32(150, 150, 150, 255), "R");
    ImGui::PopStyleColor();

    // Border
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                 IM_COL32(80, 80, 80, 255), 0.0f, 0, 1.5f);

    ImGui::Dummy(size);
}

// ============================================================================
// Main Render Function
// ============================================================================

/**
 * Render waveform display with auto-selected style
 */
static inline void renderWaveform(WaveformDisplay* wf, ImVec2 size,
                                 Style style = STYLE_OSCILLOSCOPE,
                                 ColorScheme scheme = COLOR_GREEN_SCOPE,
                                 bool show_grid = true) {
    if (!wf) {
        ImGui::Dummy(size);
        return;
    }

    switch (style) {
        case STYLE_OSCILLOSCOPE:
            if (wf->channel_mode == WAVEFORM_CHANNEL_STEREO) {
                renderStereo(wf, size, scheme, show_grid);
            } else {
                renderOscilloscope(wf, size, scheme, show_grid);
            }
            break;

        case STYLE_FILLED:
            renderFilled(wf, size, scheme, show_grid);
            break;

        case STYLE_ENVELOPE:
        case STYLE_BARS:
            // TODO: Implement additional styles
            renderOscilloscope(wf, size, scheme, show_grid);
            break;
    }
}

} // namespace Waveform

#endif // WAVEFORM_IMGUI_H
